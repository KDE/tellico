/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "kinofetcher.h"
#include "../utils/guiproxy.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../utils/string_utils.h"
#include "../utils/objvalue.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfig>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const char* KINO_BASE_URL = "https://www.kino.de/se/";
}

using namespace Tellico;
using Tellico::Fetch::KinoFetcher;

KinoFetcher::KinoFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

KinoFetcher::~KinoFetcher() {
}

QString KinoFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool KinoFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

void KinoFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void KinoFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(KINO_BASE_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("types"), QStringLiteral("movie"));

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("searchterm"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url:" << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &KinoFetcher::slotComplete);
}

void KinoFetcher::stop() {
  if(!m_started) {
    return;
  }

  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  Q_EMIT signalDone(this);
}

void KinoFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;

  const QString pageText = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from kinofetcher.cpp";
  QFile f(QStringLiteral("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << pageText;
  }
  f.close();
#endif

  QRegularExpression linkRx(QStringLiteral("<div class=\"poster__title\">.*?<a .+?poster__link.+?href=\"(.+?)\".*?>(.+?)</"),
                            QRegularExpression::DotMatchesEverythingOption);
  QRegularExpression dateSpanRx(QStringLiteral("<span .+?movie-startdate.+?>(.+?)</span"));
  QRegularExpression dateRx(QStringLiteral("\\d{2}\\.\\d{2}\\.(\\d{4})"));
  QRegularExpression yearEndRx(QStringLiteral("(\\d{4})/?$"));

  auto i = linkRx.globalMatch(pageText);
  while(i.hasNext()) {
    auto match = i.next();
    QString u = match.captured(1);
    if(u.isEmpty() || u.contains(QLatin1String("news")) || !u.contains(QLatin1String("film"))) {
      continue;
    }
    if(u.startsWith(QLatin1String("//"))) {
      u.prepend(QLatin1String("https:"));
    }
    Data::CollPtr coll(new Data::VideoCollection(true));
    Data::EntryPtr entry(new Data::Entry(coll));
    coll->addEntries(entry);

    entry->setField(QStringLiteral("title"), match.captured(2));

    QString y;
    auto dateMatch = dateSpanRx.match(pageText, match.capturedEnd());
    if(dateMatch.hasMatch()) {
      y = dateRx.match(dateMatch.captured(1)).captured(1);
    } else {
      // see if year is embedded in url
      y = yearEndRx.match(u).captured(1);
    }
    entry->setField(QStringLiteral("year"), y);

    FetchResult* r = new FetchResult(this, entry);
    QUrl url = QUrl(QString::fromLatin1(KINO_BASE_URL)).resolved(QUrl(u));
    m_matches.insert(r->uid, url);
    m_entries.insert(r->uid, entry);
    // don't emit signal until after putting url in matches hash
    Q_EMIT signalResultFound(r);
  }

  stop();
}

Tellico::Data::EntryPtr KinoFetcher::fetchEntryHook(uint uid_) {
  if(!m_entries.contains(uid_)) {
    myWarning() << "no entry in hash";
    return Data::EntryPtr();
  }

  Data::EntryPtr entry = m_entries[uid_];
  // if the url is not in the hash, the entry has already been fully populated
  if(!m_matches.contains(uid_)) {
    return entry;
  }

  QString results = Tellico::decodeHTML(FileHandler::readTextFile(m_matches[uid_], true, true));
  if(results.isEmpty()) {
    myDebug() << "No text results from" << m_matches[uid_];
    return entry;
  }

#if 0
  myWarning() << "Remove debug2 from kinofetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << results;
  }
  f.close();
#endif

  parseEntry(entry, results);
  // remove url to signal the entry is fully populated
  m_matches.remove(uid_);
  return entry;
}

void KinoFetcher::parseEntry(Data::EntryPtr entry, const QString& str_) {
  bool hasDirector = false;
  bool hasCast = false;
  static const QRegularExpression jsonRx(QStringLiteral("<script type=\"application/ld\\+json\">(.*?)</script"),
                                         QRegularExpression::DotMatchesEverythingOption);
  auto i = jsonRx.globalMatch(str_);
  while(i.hasNext()) {
    QJsonDocument doc = QJsonDocument::fromJson(i.next().captured(1).toUtf8());
    const auto obj = doc.object();
    if(objValue(obj, "@type") != QStringLiteral("Movie")) {
      continue;
    }
    const auto dirName = objValue(obj, "director", "name");
    entry->setField(QStringLiteral("director"), dirName);
    hasDirector = !dirName.isEmpty();

    QStringList actors;
    const auto actorArray = obj[QLatin1StringView("actor")].toArray();
    for(const auto&v : actorArray) {
      const QString actor = objValue(v.toObject(), "name");
      if(!actor.isEmpty()) actors += actor;
    }
    if(!actors.isEmpty()) {
      entry->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));
      hasCast = true;
    }
    // cover could be a relative link
    QString coverLink = objValue(obj, "image");
    if(coverLink.startsWith(QLatin1String("//"))) {
      coverLink.prepend(QLatin1String("https:"));
    }
    entry->setField(QStringLiteral("cover"), coverLink);

    const QString genreString = objValue(obj, "genre");
    if(!genreString.isEmpty()) {
      static const QRegularExpression commaRx(QStringLiteral(",\\s+"));
      QStringList genres = genreString.split(commaRx);
      entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
    }
  }

  static const QRegularExpression tagRx(QStringLiteral("<.+?>"));
  static const QRegularExpression liRx(QStringLiteral("<li>(.+?)</li>"));

  QRegularExpression nationalityRx(QStringLiteral(">Produktionsland:(.*?)</a>"));
  auto nationalityMatch = nationalityRx.match(str_);
  if(nationalityMatch.hasMatch()) {
    const QString n = nationalityMatch.captured(1).remove(tagRx).trimmed();
    entry->setField(QStringLiteral("nationality"), n);
  } else {
    nationalityRx.setPattern(QStringLiteral("href=\".+?/laender/.+?\" title=\"(.+?)\""));
    nationalityMatch = nationalityRx.match(str_);
    if(nationalityMatch.hasMatch()) {
      const QString n = nationalityMatch.captured(1);
      entry->setField(QStringLiteral("nationality"), n);
    }
  }

  QString lengthStr;
  QRegularExpression lengthRx(QStringLiteral(">Dauer:(.*?)</li"),
                              QRegularExpression::DotMatchesEverythingOption);
  auto lengthMatch = lengthRx.match(str_);
  if(lengthMatch.hasMatch()) {
    lengthStr = lengthMatch.captured(1).remove(tagRx);
  } else {
    lengthRx.setPattern(QStringLiteral(">(\\d+) Min</span"));
    lengthMatch = lengthRx.match(str_);
    if(lengthMatch.hasMatch()) {
      lengthStr = lengthMatch.captured(1);
    }
  }
  if(!lengthStr.isEmpty()) {
    const QString l = lengthStr.remove(QStringLiteral(" Min")).trimmed();
    entry->setField(QStringLiteral("running-time"), l);
  }

  QRegularExpression genreRx(QStringLiteral("<dt.*?>Genre</dt><dd.*?>(.*?)</dd>"));
  auto genreMatch = genreRx.match(str_);
  if(genreMatch.hasMatch()) {
    QRegularExpression anchorRx(QStringLiteral("<a.*?>(.*?)</a>"));
    auto i = anchorRx.globalMatch(genreMatch.captured(1));
    QStringList genres;
    while(i.hasNext()) {
      genres += i.next().captured(1).trimmed();
    }
    entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }

  QRegularExpression certRx(QStringLiteral(">FSK:?(.*?)</"),
                            QRegularExpression::DotMatchesEverythingOption);
  auto certMatch = certRx.match(str_);
  if(certMatch.hasMatch()) {
    // need to translate? Let's just add FSK ratings to the allowed values
    QStringList allowed = entry->collection()->hasField(QStringLiteral("certification")) ?
                          entry->collection()->fieldByName(QStringLiteral("certification"))->allowed() :
                          QStringList();
    if(!allowed.contains(QStringLiteral("FSK 0 (DE)"))) {
      allowed << QStringLiteral("FSK 0 (DE)")
              << QStringLiteral("FSK 6 (DE)")
              << QStringLiteral("FSK 12 (DE)")
              << QStringLiteral("FSK 16 (DE)")
              << QStringLiteral("FSK 18 (DE)");
      entry->collection()->fieldByName(QStringLiteral("certification"))->setAllowed(allowed);
    }
    QString c = certMatch.captured(1).remove(tagRx).trimmed();
    if(c == QLatin1String("ab 0") || c == QLatin1String("0")) {
      c = QStringLiteral("FSK 0 (DE)");
    } else if(c == QLatin1String("ab 6") || c == QLatin1String("6")) {
      c = QStringLiteral("FSK 6 (DE)");
    } else if(c == QLatin1String("ab 12") || c == QLatin1String("12")) {
      c = QStringLiteral("FSK 12 (DE)");
    } else if(c == QLatin1String("ab 16") || c == QLatin1String("16")) {
      c = QStringLiteral("FSK 16 (DE)");
    } else if(c == QLatin1String("ab 18") || c == QLatin1String("18")) {
      c = QStringLiteral("FSK 18 (DE)");
    }
    entry->setField(QStringLiteral("certification"), c);
  }

  QRegularExpression studioRx(QStringLiteral(">Filmverleih(.*?)</li"));
  auto studioMatch = studioRx.match(str_);
  if(studioMatch.hasMatch()) {
    QString s = studioMatch.captured(1).remove(tagRx).trimmed();
    entry->setField(QStringLiteral("studio"), s);
  }

  if(!hasDirector) {
    QRegularExpression directorRx(QStringLiteral(">Regisseur(.*?)</ul"));
    auto directorMatch = directorRx.match(str_);
    if(directorMatch.hasMatch()) {
      auto i = liRx.globalMatch(directorMatch.captured(1));
      QStringList directors;
      while(i.hasNext()) {
        QString d = i.next().captured(1).trimmed();
        if(d.endsWith(QLatin1Char(','))) d.chop(1);
        if(!d.isEmpty()) directors += d;
      }
      entry->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
    }
  }

  QRegularExpression producerRx(QStringLiteral(">Produzent(.*?)</ul"));
  auto producerMatch = producerRx.match(str_);
  if(producerMatch.hasMatch()) {
    auto i = liRx.globalMatch(producerMatch.captured(1));
    QStringList producers;
    while(i.hasNext()) {
      QString p = i.next().captured(1).trimmed();
      if(p.endsWith(QLatin1Char(','))) p.chop(1);
      if(!p.isEmpty()) producers += p;
    }
    entry->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));
  }

  if(!hasCast) {
    QRegularExpression castRx(QStringLiteral(">Darsteller(.*?)</ul"));
    auto castMatch = castRx.match(str_);
    if(castMatch.hasMatch()) {
      auto i = liRx.globalMatch(castMatch.captured(1));
      QStringList cast;
      while(i.hasNext()) {
        QString c = i.next().captured(1).trimmed();
        if(c.endsWith(QLatin1Char(','))) c.chop(1);
        if(!c.isEmpty()) cast += c;
      }
      entry->setField(QStringLiteral("cast"), cast.join(FieldFormat::rowDelimiterString()));
    }
  }

  QRegularExpression plotRx(QStringLiteral("(<p class=\"movie-plot-synopsis\">.+?</p>)<(div|h2)"),
                            QRegularExpression::DotMatchesEverythingOption);
  auto plotMatch = plotRx.match(str_);
  if(!plotMatch.hasMatch()) {
    QRegularExpression plot2Rx(QStringLiteral("(</h2><p>.+?</p>)<(div|h2)"),
                               QRegularExpression::DotMatchesEverythingOption);
    plotMatch = plot2Rx.match(str_);
  }
  if(plotMatch.hasMatch()) {
    QString plot;
    // sometimes the plot starts with double <p>
    QRegularExpression pRx(QStringLiteral("<p.*?>(?!<p.*?>).*?</p>"));
    auto i = pRx.globalMatch(plotMatch.captured(1));
    while(i.hasNext()) {
      plot += i.next().captured(0);
    }
    plot = plot.remove(tagRx).trimmed();
    entry->setField(QStringLiteral("plot"), plot);
  }

  QString cover = entry->field(QStringLiteral("cover"));
  if(!cover.isEmpty()) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(cover), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }
}

Tellico::Fetch::FetchRequest KinoFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* KinoFetcher::configWidget(QWidget* parent_) const {
  return new KinoFetcher::ConfigWidget(parent_, this);
}

QString KinoFetcher::defaultName() {
  return QStringLiteral("Kino.de");
}

QString KinoFetcher::defaultIcon() {
  return favIcon("https://www.kino.de");
}

//static
Tellico::StringHash KinoFetcher::allOptionalFields() {
  StringHash hash;
  // TODO: add link
//  hash[QStringLiteral("kino")] = i18n("Kino.de Link");
  return hash;
}

KinoFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const KinoFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(KinoFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString KinoFetcher::ConfigWidget::preferredName() const {
  return KinoFetcher::defaultName();
}
