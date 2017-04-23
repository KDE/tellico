/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "doubanfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/musiccollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/isbnvalidator.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const int DOUBAN_MAX_RETURNS_TOTAL = 20;
  static const char* DOUBAN_API_URL = "https://api.douban.com/v2/";
  // old and unused
  //static const char* DOUBAN_API_KEY = "0bd1672394eb1ebf2374356abec15c3d";
}

using namespace Tellico;
using Tellico::Fetch::DoubanFetcher;

DoubanFetcher::DoubanFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

DoubanFetcher::~DoubanFetcher() {
}

QString DoubanFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DoubanFetcher::canSearch(FetchKey k) const {
  return k == Keyword || k == ISBN;
}

bool DoubanFetcher::canFetch(int type) const {
  return type == Data::Collection::Book
      || type == Data::Collection::Bibtex
      || type == Data::Collection::Video
      || type == Data::Collection::Album;
}

void DoubanFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void DoubanFetcher::search() {
  m_started = true;
  QUrl u(QString::fromLatin1(DOUBAN_API_URL));

  switch(request().collectionType) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      u.setPath(u.path() + QLatin1String("book/"));
      break;

    case Data::Collection::Video:
      u.setPath(u.path() + QLatin1String("movie/"));
      break;

    case Data::Collection::Album:
      u.setPath(u.path() + QLatin1String("music/"));
      break;

    default:
      myWarning() << "bad collection type:" << request().collectionType;
  }

  QUrlQuery q;
  switch(request().key) {
    case ISBN:
      u.setPath(u.path() + QLatin1String("isbn/"));
      {
        QStringList isbns = FieldFormat::splitValue(request().value);
        if(!isbns.isEmpty()) {
          u.setPath(u.path() + ISBNValidator::cleanValue(isbns.front()));
        }
      }
      break;

    case Keyword:
      u.setPath(u.path() + QLatin1String("search"));
      q.addQueryItem(QLatin1String("q"), request().value);
      q.addQueryItem(QLatin1String("count"), QString::number(DOUBAN_MAX_RETURNS_TOTAL));
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
  }

//  q.addQueryItem(QLatin1String("start"), QString::number(0));
  u.setQuery(q);
//  myDebug() << "url:" << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  if(request().key == ISBN) {
    connect(m_job, SIGNAL(result(KJob*)), SLOT(slotCompleteISBN(KJob*)));
  } else {
    connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
  }
}

void DoubanFetcher::stop() {
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

void DoubanFetcher::slotCompleteISBN(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QVariantMap resultMap = doc.object().toVariantMap();

  // code == 6000 for no result
  if(value(resultMap, "code") == QLatin1String("6000")) {
    message(value(resultMap, "msg"), MessageHandler::Error);
  } else {
    Data::EntryPtr entry = createEntry(resultMap);
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    m_matches.insert(r->uid, QUrl(value(resultMap, "url")));
    emit signalResultFound(r);
  }

  stop();
}

void DoubanFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from doubanfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-douban1.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QVariantMap resultsMap = doc.object().toVariantMap();

  switch(request().collectionType) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      foreach(const QVariant& v, resultsMap.value(QLatin1String("books")).toList()) {
        const QVariantMap resultMap = v.toMap();
        FetchResult* r = new FetchResult(Fetcher::Ptr(this), value(resultMap, "title"),
                                         value(resultMap, "author") + QLatin1Char('/') +
                                         value(resultMap, "publisher") + QLatin1Char('/') +
                                         value(resultMap, "pubdate").left(4));
        m_matches.insert(r->uid, QUrl(value(resultMap, "url")));
        emit signalResultFound(r);
      }
      break;

    case Data::Collection::Video:
      foreach(const QVariant& v, resultsMap.value(QLatin1String("subjects")).toList()) {
        const QVariantMap resultMap = v.toMap();
        FetchResult* r = new FetchResult(Fetcher::Ptr(this), value(resultMap, "title"),
                                         value(resultMap, "directors", "name") + QLatin1Char('/') +
                                         value(resultMap, "year"));
        // movie results don't appear to have a url field
        m_matches.insert(r->uid, QUrl(QLatin1String(DOUBAN_API_URL) +
                                      QLatin1String("movie/subject/") +
                                      value(resultMap, "id")));
        emit signalResultFound(r);
      }
      break;

    case Data::Collection::Album:
      foreach(const QVariant& v, resultsMap.value(QLatin1String("musics")).toList()) {
        const QVariantMap resultMap = v.toMap();
        FetchResult* r = new FetchResult(Fetcher::Ptr(this), value(resultMap, "title"),
                                         value(resultMap, "attrs", "singer") + QLatin1Char('/') +
                                         value(resultMap, "attrs", "publisher") + QLatin1Char('/') +
                                         value(resultMap, "attrs", "pubdate").left(4));
        // movie results don't appear to have a url field
        m_matches.insert(r->uid, QUrl(QLatin1String(DOUBAN_API_URL) +
                                      QLatin1String("music/") +
                                      value(resultMap, "id")));
        emit signalResultFound(r);
      }
      break;

    default:
      break;
  }

  stop();
}

Tellico::Data::EntryPtr DoubanFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(entry) {
    return entry;
  }

  QUrl url = m_matches.value(uid_);
  QByteArray data = FileHandler::readDataFile(url, true);
#if 0
  myWarning() << "Remove output debug from doubanfetcher.cpp";
  QFile f(QLatin1String("/tmp/test-douban2.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  entry = createEntry(doc.object().toVariantMap());

  if(entry) {
    m_entries.insert(uid_, entry);
  }
  return entry;
}

Tellico::Data::EntryPtr DoubanFetcher::createEntry(const QVariantMap& resultMap_) {
  Data::CollPtr coll;
  Data::EntryPtr entry;
  switch(request().collectionType) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      coll = new Data::BookCollection(true);
      if(optionalFields().contains(QLatin1String("origtitle")) &&
        !value(resultMap_, "origin_title").isEmpty() &&
        !coll->hasField(QLatin1String("origtitle"))) {
        Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
        f->setFormatType(FieldFormat::FormatTitle);
        coll->addField(f);
      }
      if(optionalFields().contains(QLatin1String("douban")) &&
        !value(resultMap_, "alt").isEmpty() &&
        !coll->hasField(QLatin1String("douban"))) {
        Data::FieldPtr f(new Data::Field(QLatin1String("douban"), i18n("Douban Link"), Data::Field::URL));
        f->setCategory(i18n("General"));
        coll->addField(f);
      }
      // always add a plot field. TODO: Eventually, should make a default field?
      if(!coll->hasField(QLatin1String("plot"))) {
        Data::FieldPtr f(new Data::Field(QLatin1String("plot"), i18n("Plot Summary"), Data::Field::Para));
        coll->addField(f);
      }
      entry = new Data::Entry(coll);
      populateBookEntry(entry, resultMap_);
      break;
    case Data::Collection::Video:
      coll = new Data::VideoCollection(true);
      if(optionalFields().contains(QLatin1String("origtitle")) &&
        !value(resultMap_, "original_title").isEmpty() &&
        !coll->hasField(QLatin1String("origtitle"))) {
        Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
        f->setFormatType(FieldFormat::FormatTitle);
        coll->addField(f);
      }
      if(optionalFields().contains(QLatin1String("douban")) &&
        !value(resultMap_, "alt").isEmpty() &&
        !coll->hasField(QLatin1String("douban"))) {
        Data::FieldPtr f(new Data::Field(QLatin1String("douban"), i18n("Douban Link"), Data::Field::URL));
        f->setCategory(i18n("General"));
        coll->addField(f);
      }
      entry = new Data::Entry(coll);
      populateVideoEntry(entry, resultMap_);
      break;
    case Data::Collection::Album:
      coll = new Data::MusicCollection(true);
      if(optionalFields().contains(QLatin1String("origtitle")) &&
        !value(resultMap_, "original_title").isEmpty() &&
        !coll->hasField(QLatin1String("origtitle"))) {
        Data::FieldPtr f(new Data::Field(QLatin1String("origtitle"), i18n("Original Title")));
        f->setFormatType(FieldFormat::FormatTitle);
        coll->addField(f);
      }
      if(optionalFields().contains(QLatin1String("douban")) &&
        !value(resultMap_, "alt").isEmpty() &&
        !coll->hasField(QLatin1String("douban"))) {
        Data::FieldPtr f(new Data::Field(QLatin1String("douban"), i18n("Douban Link"), Data::Field::URL));
        f->setCategory(i18n("General"));
        coll->addField(f);
      }
      entry = new Data::Entry(coll);
      populateMusicEntry(entry, resultMap_);
      break;
    default:
      break;
  }

  return entry;
}

void DoubanFetcher::populateBookEntry(Data::EntryPtr entry, const QVariantMap& resultMap_) {
  entry->setField(QLatin1String("title"), value(resultMap_, "title"));
  entry->setField(QLatin1String("subtitle"), value(resultMap_, "subtitle"));
  entry->setField(QLatin1String("author"), value(resultMap_, "author"));
  entry->setField(QLatin1String("translator"), value(resultMap_, "translator"));
  entry->setField(QLatin1String("publisher"), value(resultMap_, "publisher"));

  const QString binding = value(resultMap_, "binding");
  if(binding == QString::fromUtf8("精装")) {
    entry->setField(QLatin1String("binding"), i18n("Hardback"));
  } else if(binding == QString::fromUtf8("平装")) {
    entry->setField(QLatin1String("binding"), i18n("Paperback"));
  }

  entry->setField(QLatin1String("pub_year"), value(resultMap_, "pubdate").left(4));
  entry->setField(QLatin1String("isbn"), value(resultMap_, "isbn10"));
  entry->setField(QLatin1String("pages"), value(resultMap_, "pages"));
  entry->setField(QLatin1String("cover"), value(resultMap_, "image"));
  entry->setField(QLatin1String("keyword"), value(resultMap_, "tags", "title"));

  if(optionalFields().contains(QLatin1String("origtitle")) &&
     !value(resultMap_, "origin_title").isEmpty()) {
    entry->setField(QLatin1String("origtitle"), value(resultMap_, "origin_title"));
  }
  if(optionalFields().contains(QLatin1String("douban"))) {
    entry->setField(QLatin1String("douban"), value(resultMap_, "alt"));
  }
  entry->setField(QLatin1String("plot"), value(resultMap_, "summary"));
}

void DoubanFetcher::populateVideoEntry(Data::EntryPtr entry, const QVariantMap& resultMap_) {
  entry->setField(QLatin1String("title"), value(resultMap_, "title"));
  entry->setField(QLatin1String("genre"), value(resultMap_, "genres"));
  entry->setField(QLatin1String("director"), value(resultMap_, "directors", "name"));
  entry->setField(QLatin1String("writer"), value(resultMap_, "writers", "name"));
  entry->setField(QLatin1String("year"), value(resultMap_, "year"));
  entry->setField(QLatin1String("cover"), value(resultMap_, "images", "medium"));
  entry->setField(QLatin1String("plot"), value(resultMap_, "summary"));

  QStringList actors;
  foreach(const QVariant& v, resultMap_.value(QLatin1String("casts")).toList()) {
    actors << v.toMap().value(QLatin1String("name")).toString();
  }
  entry->setField(QLatin1String("cast"), actors.join(FieldFormat::rowDelimiterString()));

  if(optionalFields().contains(QLatin1String("origtitle")) &&
     !value(resultMap_, "original_title").isEmpty()) {
    entry->setField(QLatin1String("origtitle"), value(resultMap_, "original_title"));
  }
  if(optionalFields().contains(QLatin1String("douban"))) {
    entry->setField(QLatin1String("douban"), value(resultMap_, "alt"));
  }
}

void DoubanFetcher::populateMusicEntry(Data::EntryPtr entry, const QVariantMap& resultMap_) {
  entry->setField(QLatin1String("title"), value(resultMap_, "title"));
  entry->setField(QLatin1String("cover"), value(resultMap_, "image"));
  entry->setField(QLatin1String("artist"), value(resultMap_, "attrs", "singer"));
  entry->setField(QLatin1String("label"), value(resultMap_, "attrs", "publisher"));
  entry->setField(QLatin1String("year"), value(resultMap_, "attrs", "pubdate").left(4));

  if(value(resultMap_, "attrs", "media") == QLatin1String("Audio CD") ||
     value(resultMap_, "attrs", "media") == QLatin1String("CD")) {
    entry->setField(QLatin1String("medium"), i18n("Compact Disc"));
  }

  QStringList values, tracks;
  foreach(const QVariant& v, resultMap_.value(QLatin1String("attrs"))
                               .toMap().value(QLatin1String("tracks")).toList()) {
    // some cases have all the tracks in one item, separated by "\n" and using 01. track numbers
    if(v.toString().contains(QLatin1Char('\n'))) {
      values << v.toString().split(QLatin1String("\n"));
    } else {
      values << v.toString();
    }
  }
  QRegExp trackNumRx(QLatin1String("^\\d+[.\\s]{2}"));
  QRegExp trackDurRx(QLatin1String("\\d+:\\d{2}"));
  foreach(QString value, values) { // can't be const
    // might starts with track number
    QStringList l = value.remove(trackNumRx).split(QLatin1String(" - "));
    if(l.size() == 1) {
      // might be split by tab characters and have track length at end
      l = value.remove(trackNumRx).split(QRegExp(QLatin1String("[\t\n]+")));
      if(trackDurRx.exactMatch(l.last())) {
        tracks << l.first() + FieldFormat::columnDelimiterString() +
                  entry->field(QLatin1String("artist")) + FieldFormat::columnDelimiterString() +
                  l.last();
      } else {
        tracks << l.first();
      }
    } else if(l.size() > 1) {
      const QString last = l.takeLast();
      tracks << l.join(QLatin1String(" - ")) + FieldFormat::columnDelimiterString() + last;
    }
  }
  entry->setField(QLatin1String("track"), tracks.join(FieldFormat::rowDelimiterString()));

  if(optionalFields().contains(QLatin1String("origtitle")) &&
     !value(resultMap_, "original_title").isEmpty()) {
    entry->setField(QLatin1String("origtitle"), value(resultMap_, "original_title"));
  }
  if(optionalFields().contains(QLatin1String("douban"))) {
    entry->setField(QLatin1String("douban"), value(resultMap_, "alt"));
  }
}

Tellico::Fetch::FetchRequest DoubanFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }

  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DoubanFetcher::configWidget(QWidget* parent_) const {
  return new DoubanFetcher::ConfigWidget(parent_, this);
}

QString DoubanFetcher::defaultName() {
  return QLatin1String("Douban.com");
}

QString DoubanFetcher::defaultIcon() {
  return favIcon("http://www.douban.com");
}

Tellico::StringHash DoubanFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("origtitle")] = i18n("Original Title");
  hash[QLatin1String("douban")] = i18n("Douban Link");
  return hash;
}

DoubanFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DoubanFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(DoubanFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DoubanFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  Q_UNUSED(config_);
}

QString DoubanFetcher::ConfigWidget::preferredName() const {
  return DoubanFetcher::defaultName();
}

// static
QString DoubanFetcher::value(const QVariantMap& map, const char* name) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    return v.toStringList().join(Tellico::FieldFormat::delimiterString());
  } else {
    return QString();
  }
}

QString DoubanFetcher::value(const QVariantMap& map, const char* object, const char* name) {
  const QVariant v = map.value(QLatin1String(object));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::Map)) {
    return value(v.toMap(), name);
  } else if(v.canConvert(QVariant::List)) {
    QVariantList list = v.toList();
    return list.isEmpty() ? QString() : value(list.at(0).toMap(), name);
  } else {
    return QString();
  }
}
