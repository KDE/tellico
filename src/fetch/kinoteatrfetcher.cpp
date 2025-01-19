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

#include "kinoteatrfetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* KINOTEATR_SEARCH_URL = "https://kino-teatr.ua/uk/main/films.phtml";
}

using namespace Tellico;
using Tellico::Fetch::KinoTeatrFetcher;

KinoTeatrFetcher::KinoTeatrFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

KinoTeatrFetcher::~KinoTeatrFetcher() {
}

QString KinoTeatrFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool KinoTeatrFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

bool KinoTeatrFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title;
}

void KinoTeatrFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void KinoTeatrFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(KINOTEATR_SEARCH_URL));
  QUrlQuery q;

  switch(request().key()) {
    case Title:
      // TODO: allow year in search query and parse it out?
      //q.addQueryItem(QStringLiteral("year"), QStringLiteral("yes"));
      q.addQueryItem(QStringLiteral("title"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &KinoTeatrFetcher::slotComplete);
}

void KinoTeatrFetcher::stop() {
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

void KinoTeatrFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  const QString output = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from kinoteatrfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test1.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif

  // look for a specific div, with an href and title, sometime uses single-quote, sometimes double-quotes
  QRegularExpression resultRx(QStringLiteral("<a class=\"uk-margin-small-bottom\" href=\"(.+?)\".+?</a>"),
                              QRegularExpression::DotMatchesEverythingOption);
  QRegularExpression titleRx(QStringLiteral("<h2 class=\"uk-h4\">(.+?)</"));
  // the year is within the searchItemText as a 4-digit number, starting with 1 or 2
  QRegularExpression yearRx(QStringLiteral(" ([12]\\d\\d\\d)[ \"]"));

  QString href, title, year;
  QRegularExpressionMatchIterator i = resultRx.globalMatch(output);
  while(i.hasNext() && m_started) {
    QRegularExpressionMatch topMatch = i.next();
    const QString resultText = topMatch.captured();
    href = topMatch.captured(1);
    QRegularExpressionMatch match = titleRx.match(resultText);
    if(match.hasMatch()) {
      title = match.captured(1);
    }
    // there can be multiple
    match = yearRx.match(resultText);
    if(match.hasMatch()) {
      year = match.captured(1);
    }
    if(!href.isEmpty()) {
      QUrl url(QString::fromLatin1(KINOTEATR_SEARCH_URL));
      url = url.resolved(QUrl(href));
//      myDebug() << url << title << year;
      FetchResult* r = new FetchResult(this, title, year);
      m_matches.insert(r->uid, url);
      Q_EMIT signalResultFound(r);
    }
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;
  stop();
}

Tellico::Data::EntryPtr KinoTeatrFetcher::fetchEntryHook(uint uid_) {
  // if we already grabbed this one, then just pull it out of the dict
  Data::EntryPtr entry = m_entries[uid_];
  if(entry) {
    return entry;
  }

  QUrl url = m_matches[uid_];
  if(url.isEmpty()) {
    myWarning() << "no url in map";
    return Data::EntryPtr();
  }

  const QString results = Tellico::decodeHTML(FileHandler::readDataFile(url, true));
  if(results.isEmpty()) {
    myDebug() << "no text results";
    return Data::EntryPtr();
  }

#if 0
  myDebug() << url.url();
  myWarning() << "Remove debug2 from kinoteatrfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-kinoteatr.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    myDebug() << "error in processing entry";
    return Data::EntryPtr();
  }

  QString newPath(url.path());
  newPath.replace(QLatin1String("/film/"), QLatin1String("/film-persons/"));
  QUrl personUrl(url);
  personUrl.setPath(newPath);

  const QString personsText = Tellico::decodeHTML(FileHandler::readDataFile(personUrl, true));
  if(!personsText.isEmpty()) {
    parsePeople(entry, personsText);
#if 0
    myWarning() << "Remove persons debug from kinoteatrfetcher.cpp";
    myDebug() << personUrl.url();
    QFile f2(QStringLiteral("/tmp/test-kinoteatr-persons.html"));
    if(f2.open(QIODevice::WriteOnly)) {
      QTextStream t(&f2);
      t << personsText;
    }
    f2.close();
#endif
  }

  if(optionalFields().contains(QStringLiteral("kinoteatr"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("kinoteatr"), i18n("Kino-Teatr Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    entry->collection()->addField(field);
    entry->setField(QStringLiteral("kinoteatr"), url.url());
  }

  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr KinoTeatrFetcher::parseEntry(const QString& str_) {
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  static const QRegularExpression tagRx(QLatin1String("<.*?>"));
  const QRegularExpression anchorRx(QStringLiteral("<a.+?href=[\"'].+?[\"'].*?>(.*?)</"));

  QRegularExpression titleRx(QStringLiteral("<span itemprop=[\"']name[\"']>(.+?)</span"));
  QRegularExpressionMatch match = titleRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("title"), match.captured(1).simplified());
  }

  if(optionalFields().contains(QStringLiteral("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);

    QRegularExpression origTitleRx(QStringLiteral("itemprop=\"alternativeHeadline\".*?>(.+?)</"));
    match = origTitleRx.match(str_);
    if(match.hasMatch()) {
      entry->setField(QStringLiteral("origtitle"), match.captured(1).simplified());
    }
  }

  QRegularExpression yearRx(QStringLiteral("Рік:.*?([12]\\d\\d\\d).*?</a"),
                            QRegularExpression::DotMatchesEverythingOption);
  match = yearRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("year"), match.captured(1));
  }

  QRegularExpression countryRx(QStringLiteral("Країна:(.*?)<br"),
                               QRegularExpression::DotMatchesEverythingOption);
  match = countryRx.match(str_);
  if(match.hasMatch()) {
    const QString innerText = match.captured(1);
    QStringList countries;
    QRegularExpressionMatchIterator i = anchorRx.globalMatch(innerText);
    while(i.hasNext()) {
      match = i.next();
      const QString s = match.captured(1).simplified();
      if(!s.isEmpty()) {
        countries += s;
      }
    }
    if(!countries.isEmpty()) {
      countries.removeDuplicates();
      entry->setField(QStringLiteral("nationality"), countries.join(Tellico::FieldFormat::delimiterString()));
    }
  }

  QRegularExpression genreRx(QStringLiteral("itemprop=\"genre\">(.*?)<br"),
                             QRegularExpression::DotMatchesEverythingOption);
  match = genreRx.match(str_);
  if(match.hasMatch()) {
    const QString innerText = match.captured(1);
    QStringList genres;
    QRegularExpressionMatchIterator i = anchorRx.globalMatch(innerText);
    while(i.hasNext()) {
      match = i.next();
      const QString s = match.captured(1).simplified();
      if(!s.isEmpty()) {
        genres += s;
      }
    }
    if(!genres.isEmpty()) {
      genres.removeDuplicates();
      entry->setField(QStringLiteral("genre"), genres.join(Tellico::FieldFormat::delimiterString()));
    }
  }

  QRegularExpression directorRx(QStringLiteral("itemprop=\"director\".*?>(.*?)<br"),
                                QRegularExpression::DotMatchesEverythingOption);
  match = directorRx.match(str_);
  if(match.hasMatch()) {
    const QString innerText = match.captured(1);
    QStringList directors;
    QRegularExpressionMatchIterator i = anchorRx.globalMatch(innerText);
    while(i.hasNext()) {
      match = i.next();
      QString s = match.captured(1).simplified();
      if(!s.isEmpty()) {
        directors += s.remove(tagRx);
      }
    }
    if(!directors.isEmpty()) {
      entry->setField(QStringLiteral("director"), directors.join(Tellico::FieldFormat::delimiterString()));
    }
  }

  QRegularExpression runtimeRx(QStringLiteral("Тривалість:.*?(\\d+).*?хв<br>"),
                               QRegularExpression::DotMatchesEverythingOption);
  match = runtimeRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("running-time"), match.captured(1));
  }

  QRegularExpression plotRx(QStringLiteral("itemprop=[\"']description[\"'].*?>(.+?)</div"),
                            QRegularExpression::DotMatchesEverythingOption);
  match = plotRx.match(str_);
  if(match.hasMatch()) {
    entry->setField(QStringLiteral("plot"), Tellico::decodeHTML(match.captured(1).simplified()));
  } else {
    plotRx.setPattern(QStringLiteral("<meta name=\"og:description\" content=\"(.+?)\""));
    match = plotRx.match(str_);
    if(match.hasMatch()) {
      entry->setField(QStringLiteral("plot"), Tellico::decodeHTML(match.captured(1)));
    }
  }

  QString cover;
  QRegularExpression coverRx(QStringLiteral("<img\\s.*?src=[\"'](.+?)[\"'].+?itemprop=[\"']image[\"']"));
  match = coverRx.match(str_);
  if(match.hasMatch()) {
    cover = match.captured(1);
  } else {
    coverRx.setPattern(QStringLiteral("<meta property=\"og:image\" content=\"(.+?)\""));
    match = coverRx.match(str_);
    if(match.hasMatch()) {
      cover = match.captured(1);
    }
  }
  if(!cover.isEmpty()) {
//    myDebug() << "cover:" << cover;
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(cover), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  return entry;
}

void KinoTeatrFetcher::parsePeople(Data::EntryPtr entry_, const QString& str_) {
  if(!entry_) {
    myDebug() << "no entry";
    return;
  }

  QRegularExpression nameDivRx(QStringLiteral("<div.*?>(.+?)</div"),
                               QRegularExpression::DotMatchesEverythingOption);
  QRegularExpression anchorRx(QStringLiteral("<a[^>]+?person[^>]+?>(.+?)</a"));
  QRegularExpression roleRx(QStringLiteral("<br>(.+?)$"));

  QRegularExpression castRx(QStringLiteral("Актори(.+?)<(header|/section)"),
                            QRegularExpression::DotMatchesEverythingOption);
  auto match = castRx.match(str_);
  if(match.hasMatch()) {
    const QString innerText = match.captured(1);
    QStringList actors, roles;
    auto i = nameDivRx.globalMatch(innerText);
    while(i.hasNext()) {
      match = i.next();
      QRegularExpressionMatch anchorMatch = anchorRx.match(match.captured(1));
      if(anchorMatch.hasMatch()) {
        actors += anchorMatch.captured(1).simplified();
        auto roleMatch = roleRx.match(match.captured(1));
        roles += roleMatch.hasMatch() ? roleMatch.captured(1).simplified() : QString();
      }
    }
    // interleave actors and roles
    QStringList cast;
    for(int i = 0; i< actors.length(); ++i) {
      QString row = actors.at(i);
      if(!roles.at(i).isEmpty()) {
        row += FieldFormat::columnDelimiterString() + roles.at(i);
      }
      cast += row;
    }
    if(!cast.isEmpty()) {
//      myDebug() << cast;
      entry_->setField(QStringLiteral("cast"), cast.join(FieldFormat::rowDelimiterString()));
    }
  }

  QRegularExpression writerRx(QStringLiteral("Сценаристи(.+?)<(header|/section)"),
                              QRegularExpression::DotMatchesEverythingOption);
  match = writerRx.match(str_);
  if(match.hasMatch()) {
    const QString innerText = match.captured(1);
    QStringList writers;
    auto i = nameDivRx.globalMatch(innerText);
    while(i.hasNext()) {
      match = i.next();
      auto anchorMatch = anchorRx.match(match.captured(1));
      if(anchorMatch.hasMatch()) {
        writers += anchorMatch.captured(1).simplified();
      }
    }
    if(!writers.isEmpty()) {
      entry_->setField(QStringLiteral("writer"), writers.join(FieldFormat::delimiterString()));
    }
  }

  QRegularExpression producerRx(QStringLiteral("Продюсери(.+?)<(header|/section)"),
                                QRegularExpression::DotMatchesEverythingOption);
  match = producerRx.match(str_);
  if(match.hasMatch()) {
    const QString innerText = match.captured(1);
    QStringList producers;
    auto i = nameDivRx.globalMatch(innerText);
    while(i.hasNext()) {
      match = i.next();
      auto anchorMatch = anchorRx.match(match.captured(1));
      if(anchorMatch.hasMatch()) {
        producers += anchorMatch.captured(1).simplified();
      }
    }
    if(!producers.isEmpty()) {
      entry_->setField(QStringLiteral("producer"), producers.join(FieldFormat::delimiterString()));
    }
  }

  QRegularExpression composerRx(QStringLiteral("Композитори(.+?)<(header|/section)"),
                                QRegularExpression::DotMatchesEverythingOption);
  match = composerRx.match(str_);
  if(match.hasMatch()) {
    const QString innerText = match.captured(1);
    QStringList composers;
    auto i = nameDivRx.globalMatch(innerText);
    while(i.hasNext()) {
      match = i.next();
      auto anchorMatch = anchorRx.match(match.captured(1));
      if(anchorMatch.hasMatch()) {
        composers += anchorMatch.captured(1).simplified();
      }
    }
    if(!composers.isEmpty()) {
      entry_->setField(QStringLiteral("composer"), composers.join(FieldFormat::delimiterString()));
    }
  }
}

Tellico::Fetch::FetchRequest KinoTeatrFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* KinoTeatrFetcher::configWidget(QWidget* parent_) const {
  return new KinoTeatrFetcher::ConfigWidget(parent_);
}

QString KinoTeatrFetcher::defaultName() {
  return QStringLiteral("Кіно-Театр (kino-teatr.ua)");
}

QString KinoTeatrFetcher::defaultIcon() {
  return favIcon("https://kino-teatr.ua");
}

Tellico::StringHash KinoTeatrFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("kinoteatr")] = i18n("Kino-Teatr Link");
  return hash;
}

KinoTeatrFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const KinoTeatrFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  addFieldsWidget(KinoTeatrFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString KinoTeatrFetcher::ConfigWidget::preferredName() const {
  return KinoTeatrFetcher::defaultName();
}
