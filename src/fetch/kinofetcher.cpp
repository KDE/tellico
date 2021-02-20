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
#include "../utils/string_utils.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfig>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KJobWidgets/KJobWidgets>

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
  q.addQueryItem(QStringLiteral("sp_search_filter"), QStringLiteral("movie"));

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("searchterm"), request().value());
      break;

    default:
      myWarning() << "key not recognized: " << request().key();
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
  emit signalDone(this);
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

  const QString s = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from kinofetcher.cpp";
  QFile f(QStringLiteral("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << s;
  }
  f.close();
#endif

  QRegularExpression linkRx(QStringLiteral("<span class=\"alice-teaser-label\\s*?\">.+?Film.+?<a .+?teaser-link.+?href=\"(.+?)\".*?>(.+?)</"));
  QRegularExpression dateSpanRx(QStringLiteral("<span .+?movie-startdate.+?>(.+?)</span"));
  QRegularExpression dateRx(QStringLiteral("\\d{2}\\.\\d{2}\\.(\\d{4})"));
  QRegularExpression yearEndRx(QStringLiteral("(\\d{4})/?$"));

  QRegularExpressionMatchIterator i = linkRx.globalMatch(s);
  while(i.hasNext()) {
    QRegularExpressionMatch match = i.next();
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
    QRegularExpressionMatch dateMatch = dateSpanRx.match(s, match.capturedEnd());
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
    emit signalResultFound(r);
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
    myDebug() << "no text results from" << m_matches[uid_];
    return entry;
  }

#if 0
  myWarning() << "Remove debug from kinofetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
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
  QRegularExpression jsonRx(QStringLiteral("<script type=\"application/ld\\+json\">(.*?)</script"),
                            QRegularExpression::DotMatchesEverythingOption);
  QRegularExpressionMatchIterator i = jsonRx.globalMatch(str_);
  while(i.hasNext()) {
    QJsonDocument doc = QJsonDocument::fromJson(i.next().captured(1).toUtf8());
    QVariantMap objectMap = doc.object().toVariantMap();
    if(mapValue(objectMap, "@type") != QStringLiteral("Movie")) {
      continue;
    }
    entry->setField(QStringLiteral("director"), mapValue(objectMap, "director", "name"));

    QStringList actors;
    foreach(QVariant v, objectMap.value(QLatin1String("actor")).toList()) {
      const QString actor = mapValue(v.toMap(), "name");
      if(!actor.isEmpty()) actors += actor;
    }
    if(!actors.isEmpty()) {
      entry->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));
    }
    // cover could be a relative link
    QString coverLink = mapValue(objectMap, "image");
    if(coverLink.startsWith(QLatin1String("//"))) {
      coverLink.prepend(QLatin1String("https:"));
    }
    entry->setField(QStringLiteral("cover"), coverLink);
  }

  QRegularExpression tagRx(QStringLiteral("<.+?>"));

  QRegularExpression nationalityRx(QStringLiteral("<dt.*?>Produktionsland</dt><dd.*?>(.*?)</dd>"));
  QRegularExpressionMatch nationalityMatch = nationalityRx.match(str_);
  if(nationalityMatch.hasMatch()) {
    const QString n = nationalityMatch.captured(1).remove(tagRx);
    entry->setField(QStringLiteral("nationality"), n);
  }

  QRegularExpression lengthRx(QStringLiteral("<dt.*?>Dauer</dt><dd.*?>(.*?)</dd>"));
  QRegularExpressionMatch lengthMatch = lengthRx.match(str_);
  if(lengthMatch.hasMatch()) {
    const QString l = lengthMatch.captured(1).remove(tagRx).remove(QStringLiteral(" Min"));
    entry->setField(QStringLiteral("running-time"), l);
  }

  QRegularExpression genreRx(QStringLiteral("<dt.*?>Genre</dt><dd.*?>(.*?)</dd>"));
  QRegularExpressionMatch genreMatch = genreRx.match(str_);
  if(genreMatch.hasMatch()) {
    QRegularExpression anchorRx(QStringLiteral("<a.*?>(.*?)</a>"));
    QRegularExpressionMatchIterator i = anchorRx.globalMatch(genreMatch.captured(1));
    QStringList genres;
    while(i.hasNext()) {
      genres += i.next().captured(1).trimmed();
    }
    entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }

  QRegularExpression certRx(QStringLiteral("<dt.*?>FSK</dt><dd.*?>(.*?)</dd>"));
  QRegularExpressionMatch certMatch = certRx.match(str_);
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
    QString c = certMatch.captured(1).remove(tagRx);
    if(c == QStringLiteral("ab 0")) {
      c = QStringLiteral("FSK 0 (DE)");
    } else if(c == QLatin1String("ab 6")) {
      c = QStringLiteral("FSK 6 (DE)");
    } else if(c == QLatin1String("ab 12")) {
      c = QStringLiteral("FSK 12 (DE)");
    } else if(c == QLatin1String("ab 16")) {
      c = QStringLiteral("FSK 16 (DE)");
    } else if(c == QLatin1String("ab 18")) {
      c = QStringLiteral("FSK 18 (DE)");
    }
    entry->setField(QStringLiteral("certification"), c);
  }

  QRegularExpression studioRx(QStringLiteral("<dt.*?>Filmverleih</dt><dd.*?>(.*?)</dd>"));
  QRegularExpressionMatch studioMatch = studioRx.match(str_);
  if(studioMatch.hasMatch()) {
    QString s = studioMatch.captured(1).remove(tagRx).remove(QStringLiteral(" Min"));
    entry->setField(QStringLiteral("studio"), s);
  }

  QRegularExpression plotRx(QStringLiteral("(<p class=\"movie-plot-synopsis\">.+?</p>)<(div|h2)"),
                                          QRegularExpression::DotMatchesEverythingOption);
  QRegularExpressionMatch plotMatch = plotRx.match(str_);
  if(plotMatch.hasMatch()) {
    QString plot;
    // sometimes the plot starts with double <p>
    QRegularExpression pRx(QStringLiteral("<p.*?>(?!<p.*?>).*?</p>"));
    QRegularExpressionMatchIterator i = pRx.globalMatch(plotMatch.captured(1));
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
