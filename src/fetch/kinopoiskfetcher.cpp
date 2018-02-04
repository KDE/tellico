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

#include "kinopoiskfetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/videocollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>

#include <QRegExp>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const char* KINOPOISK_SEARCH_URL = "http://www.kinopoisk.ru/index.php";
}

using namespace Tellico;
using Tellico::Fetch::KinoPoiskFetcher;

KinoPoiskFetcher::KinoPoiskFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

KinoPoiskFetcher::~KinoPoiskFetcher() {
}

QString KinoPoiskFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool KinoPoiskFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

bool KinoPoiskFetcher::canSearch(FetchKey k) const {
  return k == Title;
}

void KinoPoiskFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void KinoPoiskFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(KINOPOISK_SEARCH_URL));
  QUrlQuery q;

  switch(request().key) {
    case Title:
      // first means return first result only
      //q.addQueryItem(QLatin1String("first"), QLatin1String("yes"));
      q.addQueryItem(QStringLiteral("kp_query"), request().value);
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void KinoPoiskFetcher::stop() {
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

void KinoPoiskFetcher::slotComplete(KJob*) {
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
  myWarning() << "Remove debug from kinopoiskfetcher.cpp";
  QFile f(QLatin1String("/tmp/test1.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << output;
  }
  f.close();
#endif

  // look for a paragraph, class=",", with an internal ink to "/film..."
  QRegExp resultRx(QLatin1String("<p class=\"name\">\\s*"
                                 "<a href=\"/film[^\"]+\".* data-url=\"([^\"]*)\".*>(.*)</a>\\s*"
                                 "<span class=\"year\">(.*)</span"));
  resultRx.setMinimal(true);

  QString href, title, year;
  for(int pos = resultRx.indexIn(output); m_started && pos > -1;
          pos = resultRx.indexIn(output, pos+resultRx.matchedLength())) {
    href = resultRx.cap(1);
    title = resultRx.cap(2);
    year = resultRx.cap(3);
    if(!href.isEmpty()) {
      QUrl url(QString::fromLatin1(KINOPOISK_SEARCH_URL));
      url = url.resolved(QUrl(href));
//      myDebug() << url << title << year;
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), title, year);
      m_matches.insert(r->uid, url);
      emit signalResultFound(r);
    }
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;
  stop();
}

Tellico::Data::EntryPtr KinoPoiskFetcher::fetchEntryHook(uint uid_) {
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

//  myDebug() << url.url();
#if 0
  myWarning() << "Remove debug from kinopoiskfetcher.cpp";
  QFile f(QLatin1String("/tmp/test2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << results;
  }
  f.close();
#endif

  entry = parseEntry(results);
  if(!entry) {
    myDebug() << "error in processing entry";
    return Data::EntryPtr();
  }

  if(optionalFields().contains(QLatin1String("kinopoisk"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("kinopoisk"), i18n("KinoPoisk Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    entry->collection()->addField(field);
    entry->setField(QStringLiteral("kinopoisk"), url.url());
  }

  m_entries.insert(uid_, entry); // keep for later
  return entry;
}

Tellico::Data::EntryPtr KinoPoiskFetcher::parseEntry(const QString& str_) {
  Data::CollPtr coll(new Data::VideoCollection(true));
  Data::EntryPtr entry(new Data::Entry(coll));
  coll->addEntries(entry);

  QRegExp tagRx(QLatin1String("<.*>"));
  tagRx.setMinimal(true);

  QRegExp anchorRx(QLatin1String("<a\\s+href=\".*\"[^>]*>(.*)</"));
  anchorRx.setMinimal(true);

  QRegExp titleRx(QLatin1String("class=\"moviename-big\"[^>]*>([^<]+)</"));
  if(str_.contains(titleRx)) {
    entry->setField(QStringLiteral("title"), titleRx.cap(1));
  }

  if(optionalFields().contains(QLatin1String("origtitle"))) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("origtitle"), i18n("Original Title")));
    f->setFormatType(FieldFormat::FormatTitle);
    coll->addField(f);

    QRegExp origTitleRx(QLatin1String("itemprop=\"alternativeHeadline\"[^>]*>([^<]+)</"));
    if(str_.contains(origTitleRx)) {
      entry->setField(QStringLiteral("origtitle"), origTitleRx.cap(1));
    }
  }

  QRegExp yearRx(QLatin1String("<a href=\"/lists/m_act%5Byear[^\"]+\"[^>]*>([^<]+)</a"));
  if(str_.contains(yearRx)) {
    entry->setField(QStringLiteral("year"), yearRx.cap(1));
  }

  QRegExp countryRx(QLatin1String("<a href=\"/lists/m_act%5Bcountry[^\"]+\"[^>]*>([^<]+)</a"));
  countryRx.setMinimal(true);
  QStringList countries;
  for(int pos = countryRx.indexIn(str_); pos > -1;
          pos = countryRx.indexIn(str_, pos+countryRx.matchedLength())) {
    countries += countryRx.cap(1);
  }
  if(!countries.isEmpty()) {
    countries.removeDuplicates();
    entry->setField(QStringLiteral("nationality"), countries.join(Tellico::FieldFormat::delimiterString()));
  }

  QRegExp genreRx(QLatin1String("<a href=\"/lists/m_act%5Bgenre[^\"]+\"[^>]*>([^<]+)</a"));
  genreRx.setMinimal(true);
  QStringList genres;
  for(int pos = genreRx.indexIn(str_); pos > -1;
          pos = genreRx.indexIn(str_, pos+genreRx.matchedLength())) {
    genres += genreRx.cap(1);
  }
  if(!genres.isEmpty()) {
    genres.removeDuplicates();
    entry->setField(QStringLiteral("genre"), genres.join(Tellico::FieldFormat::delimiterString()));
  }

  QRegExp directorRx(QLatin1String("<td itemprop=\"director\">(.*)</td"));
  directorRx.setMinimal(true);
  if(str_.contains(directorRx)) {
    QString s = directorRx.cap(1);
    QStringList directors;
    for(int pos = anchorRx.indexIn(s); pos > -1;
            pos = anchorRx.indexIn(s, pos+anchorRx.matchedLength())) {
      QString value = anchorRx.cap(1);
      if(value != QLatin1String("...")) {
        directors += value;
      }
    }
    if(!directors.isEmpty()) {
      entry->setField(QStringLiteral("director"), directors.join(Tellico::FieldFormat::delimiterString()));
    }
  }

  QRegExp writerRx(QStringLiteral("<td class=\"type\">сценарий</td>(.*)</td"));
  writerRx.setMinimal(true);
  if(str_.contains(writerRx)) {
    QString s = writerRx.cap(1);
    QStringList writers;
    for(int pos = anchorRx.indexIn(s); pos > -1;
            pos = anchorRx.indexIn(s, pos+anchorRx.matchedLength())) {
      QString value = anchorRx.cap(1);
      if(value != QLatin1String("...")) {
        writers += value;
      }
    }
    if(!writers.isEmpty()) {
      entry->setField(QStringLiteral("writer"), writers.join(Tellico::FieldFormat::delimiterString()));
    }
  }

  QRegExp producerRx(QLatin1String("<td itemprop=\"producer\">(.*)</td"));
  producerRx.setMinimal(true);
  if(str_.contains(producerRx)) {
    QString s = producerRx.cap(1);
    QStringList producers;
    for(int pos = anchorRx.indexIn(s); pos > -1;
            pos = anchorRx.indexIn(s, pos+anchorRx.matchedLength())) {
      QString value = anchorRx.cap(1);
      if(value != QLatin1String("...")) {
        producers += value;
      }
    }
    if(!producers.isEmpty()) {
      entry->setField(QStringLiteral("producer"), producers.join(Tellico::FieldFormat::delimiterString()));
    }
  }

  QRegExp composerRx(QLatin1String("<td itemprop=\"musicBy\">(.*)</td"));
  composerRx.setMinimal(true);
  if(str_.contains(composerRx)) {
    QString s = composerRx.cap(1);
    QStringList composers;
    for(int pos = anchorRx.indexIn(s); pos > -1;
            pos = anchorRx.indexIn(s, pos+anchorRx.matchedLength())) {
      QString value = anchorRx.cap(1);
      if(value != QLatin1String("...")) {
        composers += value;
      }
    }
    if(!composers.isEmpty()) {
      entry->setField(QStringLiteral("composer"), composers.join(Tellico::FieldFormat::delimiterString()));
    }
  }

  QRegExp castRx(QStringLiteral("<h4>В главных ролях.*</h4>.*<ul>(.*)</ul>"));
  castRx.setMinimal(true);
  if(str_.contains(castRx)) {
    QString s = castRx.cap(1);
    QStringList actors;
    for(int pos = anchorRx.indexIn(s); pos > -1;
            pos = anchorRx.indexIn(s, pos+anchorRx.matchedLength())) {
      QString value = anchorRx.cap(1);
      if(value != QLatin1String("...")) {
        actors += value;
      }
    }
    if(!actors.isEmpty()) {
      entry->setField(QStringLiteral("cast"), actors.join(Tellico::FieldFormat::rowDelimiterString()));
    }
  }

  QRegExp runtimeRx(QLatin1String("id=\"runtime\">(\\d+)"));
  if(str_.contains(runtimeRx)) {
    entry->setField(QStringLiteral("running-time"), runtimeRx.cap(1));
  }

  QRegExp plotRx(QLatin1String("itemprop=\"description\"[^>]*>(.+)</div"));
  plotRx.setMinimal(true);
  if(str_.contains(plotRx)) {
    entry->setField(QStringLiteral("plot"), Tellico::decodeHTML(plotRx.cap(1)));
  }

  QRegExp mpaaRx(QLatin1String("itemprop=\"contentRating\"[^>]+content=\"MPAA ([^>]+)\""));
  mpaaRx.setMinimal(true);
  if(str_.contains(mpaaRx)) {
    QString value = mpaaRx.cap(1) + QLatin1String(" (USA)");
//    entry->setField(QLatin1String("certification"), i18n(value.toUtf8()));
    entry->setField(QStringLiteral("certification"), value);
  }

  QRegExp coverRx(QLatin1String("<a class=\"popupBigImage\"[^>]+>\\s*<img.*src=\"([^\"]+)\""));
  coverRx.setMinimal(true);
  if(str_.contains(coverRx)) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(coverRx.cap(1)), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }
  return entry;
}

Tellico::Fetch::FetchRequest KinoPoiskFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* KinoPoiskFetcher::configWidget(QWidget* parent_) const {
  return new KinoPoiskFetcher::ConfigWidget(parent_);
}

QString KinoPoiskFetcher::defaultName() {
  return QStringLiteral("КиноПоиск (KinoPoisk.ru)");
}

QString KinoPoiskFetcher::defaultIcon() {
  return favIcon("http://www.kinopoisk.ru");
}

Tellico::StringHash KinoPoiskFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("kinopoisk")] = i18n("KinoPoisk Link");
  return hash;
}

KinoPoiskFetcher::ConfigWidget::ConfigWidget(QWidget* parent_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString KinoPoiskFetcher::ConfigWidget::preferredName() const {
  return KinoPoiskFetcher::defaultName();
}
