/***************************************************************************
    Copyright (C) 2004-2020 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>

#include "amazonfetcher.h"
#include "amazonrequest.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../collection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../utils/string_utils.h"
#include "../utils/isbnvalidator.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KSeparator>
#include <KComboBox>
#include <KAcceleratorManager>
#include <KConfigGroup>
#include <KJobWidgets/KJobWidgets>

#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QGridLayout>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {
  static const int AMAZON_RETURNS_PER_REQUEST = 10;
  static const int AMAZON_MAX_RETURNS_TOTAL = 20;
  static const char* AMAZON_ASSOC_TOKEN = "tellico-20";
}

using namespace Tellico;
using Tellico::Fetch::AmazonFetcher;

// static
// see https://webservices.amazon.com/paapi5/documentation/common-request-parameters.html#host-and-region
const AmazonFetcher::SiteData& AmazonFetcher::siteData(int site_) {
  Q_ASSERT(site_ >= 0);
  Q_ASSERT(site_ < XX);
  static SiteData dataVector[16] = {
    {
      i18n("Amazon (US)"),
      "www.amazon.com",
      "us-east-1",
      QLatin1String("us"),
      i18n("United States")
    }, {
      i18n("Amazon (UK)"),
      "www.amazon.co.uk",
      "eu-west-1",
      QLatin1String("gb"),
      i18n("United Kingdom")
    }, {
      i18n("Amazon (Germany)"),
      "www.amazon.de",
      "eu-west-1",
      QLatin1String("de"),
      i18n("Germany")
    }, {
      i18n("Amazon (Japan)"),
      "www.amazon.co.jp",
      "us-west-2",
      QLatin1String("jp"),
      i18n("Japan")
    }, {
      i18n("Amazon (France)"),
      "www.amazon.fr",
      "eu-west-1",
      QLatin1String("fr"),
      i18n("France")
    }, {
      i18n("Amazon (Canada)"),
      "www.amazon.ca",
      "us-east-1",
      QLatin1String("ca"),
      i18n("Canada")
    }, {
      // TODO: no chinese in PAAPI-5 yet?
      i18n("Amazon (China)"),
      "http://webservices.amazon.cn/onca/xml",
      "us-west-2",
      QLatin1String("ch"),
      i18n("China")
    }, {
      i18n("Amazon (Spain)"),
      "www.amazon.es",
      "eu-west-1",
      QLatin1String("es"),
      i18n("Spain")
    }, {
      i18n("Amazon (Italy)"),
      "www.amazon.it",
      "eu-west-1",
      QLatin1String("it"),
      i18n("Italy")
    }, {
      i18n("Amazon (Brazil)"),
      "www.amazon.com.br",
      "us-east-1",
      QLatin1String("br"),
      i18n("Brazil")
    }, {
      i18n("Amazon (Australia)"),
      "www.amazon.com.au",
      "us-west-2",
      QLatin1String("au"),
      i18n("Australia")
    }, {
      i18n("Amazon (India)"),
      "www.amazon.in",
      "eu-west-1",
      QLatin1String("in"),
      i18n("India")
    }, {
      i18n("Amazon (Mexico)"),
      "www.amazon.com.mx",
      "us-east-1",
      QLatin1String("mx"),
      i18n("Mexico")
    }, {
      i18n("Amazon (Turkey)"),
      "www.amazon.com.tr",
      "eu-west-1",
      QLatin1String("tr"),
      i18n("Turkey")
    }, {
      i18n("Amazon (Singapore)"),
      "www.amazon.sg",
      "us-west-2",
      QLatin1String("sg"),
      i18n("Singapore")
    }, {
      i18n("Amazon (UAE)"),
      "www.amazon.ae",
      "eu-west-1",
      QLatin1String("ae"),
      i18n("United Arab Emirates")
    }
  };

  return dataVector[qBound(0, site_, static_cast<int>(sizeof(dataVector)/sizeof(SiteData)))];
}

AmazonFetcher::AmazonFetcher(QObject* parent_)
    : Fetcher(parent_), m_site(Unknown), m_imageSize(MediumImage),
      m_assoc(QLatin1String(AMAZON_ASSOC_TOKEN)), m_limit(AMAZON_MAX_RETURNS_TOTAL),
      m_countOffset(0), m_page(1), m_total(-1), m_numResults(0), m_job(nullptr), m_started(false) {
}

AmazonFetcher::~AmazonFetcher() {
}

QString AmazonFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString AmazonFetcher::attribution() const {
  return i18n("This data is licensed under <a href=""%1"">specific terms</a>.",
              QLatin1String("https://affiliate-program.amazon.com/gp/advertising/api/detail/agreement.html"));
}

bool AmazonFetcher::canFetch(int type) const {
  return type == Data::Collection::Book
         || type == Data::Collection::ComicBook
         || type == Data::Collection::Bibtex
         || type == Data::Collection::Album
         || type == Data::Collection::Video
         || type == Data::Collection::Game
         || type == Data::Collection::BoardGame;
}

bool AmazonFetcher::canSearch(FetchKey k) const {
  // no UPC in Canada
  return k == Title
      || k == Person
      || k == ISBN
      || k == UPC
      || k == Keyword;
}

void AmazonFetcher::readConfigHook(const KConfigGroup& config_) {
  const int site = config_.readEntry("Site", int(Unknown));
  Q_ASSERT(site != Unknown);
  m_site = static_cast<Site>(site);
  if(m_name.isEmpty()) {
    m_name = siteData(m_site).title;
  }
  QString s = config_.readEntry("AccessKey");
  if(!s.isEmpty()) {
    m_accessKey = s;
  } else {
    myWarning() << "No Amazon access key";
  }
  s = config_.readEntry("AssocToken");
  if(!s.isEmpty()) {
    m_assoc = s;
  }
  s = config_.readEntry("SecretKey");
  if(!s.isEmpty()) {
    m_secretKey = s;
  } else {
    myWarning() << "No Amazon secret key";
  }
  int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
}

void AmazonFetcher::search() {
  m_started = true;
  m_page = 1;
  m_total = -1;
  m_countOffset = 0;
  m_numResults = 0;
  doSearch();
}

void AmazonFetcher::continueSearch() {
  m_started = true;
  m_limit += AMAZON_MAX_RETURNS_TOTAL;
  doSearch();
}

void AmazonFetcher::doSearch() {
  if(m_secretKey.isEmpty() || m_accessKey.isEmpty()) {
    // this message is split in two since the first half is reused later
    message(i18n("Access to data from Amazon.com requires an AWS Access Key ID and a Secret Key.") +
            QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  const QByteArray payload = requestPayload(request());
  if(payload.isEmpty()) {
    stop();
    return;
  }

  AmazonRequest request(m_accessKey, m_secretKey);
  request.setHost(siteData(m_site).host);
  request.setRegion(siteData(m_site).region);

  // debugging check
  if(m_testResultsFile.isEmpty()) {
    QUrl u;
    u.setHost(QString::fromUtf8(siteData(m_site).host));
    m_job = KIO::storedHttpPost(payload, u, KIO::HideProgressInfo);
    QMapIterator<QByteArray, QByteArray> i(request.headers(payload));
    while(i.hasNext()) {
      i.next();
      m_job->addMetaData(QStringLiteral("customHTTPHeader"), QString::fromUtf8(i.key() + ": " + i.value()));
    }
  } else {
    myDebug() << "Reading" << m_testResultsFile;
    m_job = KIO::storedGet(QUrl::fromLocalFile(m_testResultsFile), KIO::NoReload, KIO::HideProgressInfo);
  }
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &AmazonFetcher::slotComplete);
}

void AmazonFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug();
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  emit signalDone(this);
}

void AmazonFetcher::slotComplete(KJob*) {
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

#if 0
  myWarning() << "Remove debug from amazonfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test%1.xml").arg(m_page));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonParseError jsonError;
  QJsonObject databject = QJsonDocument::fromJson(data, &jsonError).object();
  if(jsonError.error != QJsonParseError::NoError) {
    myDebug() << "AmazonFetcher: JSON error -" << jsonError.errorString();
    message(jsonError.errorString(), MessageHandler::Error);
    stop();
    return;
  }
  QJsonObject resultObject = databject.value(QStringLiteral("SearchResult")).toObject();

  if(m_total == -1) {
    int totalResults = resultObject.value(QStringLiteral("TotalResultCount")).toInt();
    if(totalResults > 0) {
      m_total = totalResults;
//      myDebug() << "Total results is" << totalResults;
    }
  }

  QStringList errors;
  QJsonValue errorValue = databject.value(QLatin1String("Errors"));
  if(!errorValue.isNull()) {
    foreach(const QJsonValue& error, errorValue.toArray()) {
      errors += error.toObject().value(QLatin1String("Message")).toString();
    }
  }
  if(!errors.isEmpty()) {
    for(QStringList::ConstIterator it = errors.constBegin(); it != errors.constEnd(); ++it) {
      myDebug() << "AmazonFetcher::" << *it;
    }
    message(errors[0], MessageHandler::Error);
    stop();
    return;
  }

  Data::CollPtr coll = createCollection();
  if(!coll) {
    myDebug() << "no collection pointer";
    stop();
    return;
  }

  int count = -1;
  foreach(const QJsonValue& item, resultObject.value(QLatin1String("Items")).toArray()) {
    ++count;
    if(m_numResults >= m_limit) {
      break;
    }
    if(!m_started) {
      // might get aborted
      break;
    }
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, item.toObject());

    // special case book author
    // amazon is really bad about not putting spaces after periods
    if(coll->type() == Data::Collection::Book) {
      QRegExp rx(QLatin1String("\\.([^\\s])"));
      QStringList values = FieldFormat::splitValue(entry->field(QStringLiteral("author")));
      for(QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
        (*it).replace(rx, QStringLiteral(". \\1"));
      }
      entry->setField(QStringLiteral("author"), values.join(FieldFormat::delimiterString()));
    }

    // UK puts the year in the title for some reason
    if(m_site == UK && coll->type() == Data::Collection::Video) {
      QRegExp rx(QLatin1String("\\[(\\d{4})\\]"));
      QString t = entry->title();
      if(rx.indexIn(t) > -1) {
        QString y = rx.cap(1);
        t = t.remove(rx).simplified();
        entry->setField(QStringLiteral("title"), t);
        if(entry->field(QStringLiteral("year")).isEmpty()) {
          entry->setField(QStringLiteral("year"), y);
        }
      }
    }

    // strip HTML from comments, or plot in movies
    // tentatively don't do this, looks like ECS 4 cleaned everything up
/*
    if(coll->type() == Data::Collection::Video) {
      QString plot = entry->field(QLatin1String("plot"));
      plot.remove(stripHTML);
      entry->setField(QLatin1String("plot"), plot);
    } else if(coll->type() == Data::Collection::Game) {
      QString desc = entry->field(QLatin1String("description"));
      desc.remove(stripHTML);
      entry->setField(QLatin1String("description"), desc);
    } else {
      QString comments = entry->field(QLatin1String("comments"));
      comments.remove(stripHTML);
      entry->setField(QLatin1String("comments"), comments);
    }
*/
//    myDebug() << entry->title();
    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++m_numResults;
  }

  // we might have gotten aborted
  if(!m_started) {
    return;
  }

  // are there any additional results to get?
  m_hasMoreResults = m_page * AMAZON_RETURNS_PER_REQUEST < m_total;

  const int currentTotal = qMin(m_total, m_limit);
  if(m_page * AMAZON_RETURNS_PER_REQUEST < currentTotal) {
    int foundCount = (m_page-1) * AMAZON_RETURNS_PER_REQUEST + coll->entryCount();
    message(i18n("Results from %1: %2/%3", source(), foundCount, m_total), MessageHandler::Status);
    ++m_page;
    m_countOffset = 0;
    doSearch();
  } else if(request().value.count(QLatin1Char(';')) > 9) {
    // start new request after cutting off first 10 isbn values
    FetchRequest newRequest = request();
    newRequest.value = request().value.section(QLatin1Char(';'), 10);
    startSearch(newRequest);
  } else {
    m_countOffset = m_entries.count() % AMAZON_RETURNS_PER_REQUEST;
    if(m_countOffset == 0) {
      ++m_page; // need to go to next page
    }
    stop();
  }
}

Tellico::Data::EntryPtr AmazonFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    myWarning() << "no entry in dict";
    return entry;
  }

  // do what we can to remove useless keywords
  const int type = collectionType();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      if(optionalFields().contains(QStringLiteral("keyword"))) {
        StringSet newWords;
        const QStringList keywords = FieldFormat::splitValue(entry->field(QStringLiteral("keyword")));
        foreach(const QString& keyword, keywords) {
          if(keyword == QLatin1String("General") ||
             keyword == QLatin1String("Subjects") ||
             keyword == QLatin1String("Par prix") || // french stuff
             keyword == QLatin1String("Divers") || // french stuff
             keyword.startsWith(QLatin1Char('(')) ||
             keyword.startsWith(QLatin1String("Authors"))) {
            continue;
          }
          newWords.add(keyword);
        }
        entry->setField(QStringLiteral("keyword"), newWords.toList().join(FieldFormat::delimiterString()));
      }
      entry->setField(QStringLiteral("comments"), Tellico::decodeHTML(entry->field(QStringLiteral("comments"))));
      break;

    case Data::Collection::Video:
      {
        const QString genres = QStringLiteral("genre");
        QStringList oldWords = FieldFormat::splitValue(entry->field(genres));
        StringSet words;
        // only care about genres that have "Genres" in the amazon response
        // and take the first word after that
        for(QStringList::Iterator it = oldWords.begin(); it != oldWords.end(); ++it) {
          if((*it).indexOf(QLatin1String("Genres")) == -1) {
            continue;
          }

          // the amazon2tellico stylesheet separates words with '/'
          QStringList nodes = (*it).split(QLatin1Char('/'));
          for(QStringList::Iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            if(*it2 != QLatin1String("Genres")) {
              continue;
            }
            ++it2;
            if(it2 != nodes.end() && *it2 != QLatin1String("General")) {
              words.add(*it2);
            }
            break; // we're done
          }
        }
        entry->setField(genres, words.toList().join(FieldFormat::delimiterString()));
        // language tracks get duplicated, too
        words.clear();
        words.add(FieldFormat::splitValue(entry->field(QStringLiteral("language"))));
        entry->setField(QStringLiteral("language"), words.toList().join(FieldFormat::delimiterString()));
      }
      entry->setField(QStringLiteral("plot"), Tellico::decodeHTML(entry->field(QStringLiteral("plot"))));
      break;

    case Data::Collection::Album:
      {
        const QString genres = QStringLiteral("genre");
        QStringList oldWords = FieldFormat::splitValue(entry->field(genres));
        StringSet words;
        // only care about genres that have "Styles" in the amazon response
        // and take the first word after that
        for(QStringList::Iterator it = oldWords.begin(); it != oldWords.end(); ++it) {
          if((*it).indexOf(QLatin1String("Styles")) == -1) {
            continue;
          }

          // the amazon2tellico stylesheet separates words with '/'
          QStringList nodes = (*it).split(QLatin1Char('/'));
          bool isStyle = false;
          for(QStringList::Iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            if(!isStyle) {
              if(*it2 == QLatin1String("Styles")) {
                isStyle = true;
              }
              continue;
            }
            if(*it2 != QLatin1String("General")) {
              words.add(*it2);
            }
          }
        }
        entry->setField(genres, words.toList().join(FieldFormat::delimiterString()));
      }
      entry->setField(QStringLiteral("comments"), Tellico::decodeHTML(entry->field(QStringLiteral("comments"))));
      break;

    case Data::Collection::Game:
      entry->setField(QStringLiteral("description"), Tellico::decodeHTML(entry->field(QStringLiteral("description"))));
      break;
  }

  // clean up the title
  parseTitle(entry);

  // also sometimes table fields have rows but no values
  Data::FieldList fields = entry->collection()->fields();
  QRegExp blank(QLatin1String("[\\s") +
                FieldFormat::columnDelimiterString() +
                FieldFormat::delimiterString() +
                QLatin1String("]+")); // only white space, column separators and value separators
  foreach(Data::FieldPtr fIt, fields) {
    if(fIt->type() != Data::Field::Table) {
      continue;
    }
    if(blank.exactMatch(entry->field(fIt))) {
      entry->setField(fIt, QString());
    }
  }

  // don't want to show image urls in the fetch dialog
  // so clear them after reading the URL
  QString imageURL;
  switch(m_imageSize) {
    case SmallImage:
      imageURL = entry->field(QStringLiteral("small-image"));
      entry->setField(QStringLiteral("small-image"),  QString());
      break;
    case MediumImage:
      imageURL = entry->field(QStringLiteral("medium-image"));
      entry->setField(QStringLiteral("medium-image"),  QString());
      break;
    case LargeImage:
      imageURL = entry->field(QStringLiteral("large-image"));
      entry->setField(QStringLiteral("large-image"),  QString());
      break;
    case NoImage:
    default:
      break;
  }
//  myDebug() << "grabbing " << imageURL;
  if(!imageURL.isEmpty()) {
    QString id = ImageFactory::addImage(QUrl::fromUserInput(imageURL), true);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    } else { // amazon serves up 1x1 gifs occasionally, but that's caught in the image constructor
      // all relevant collection types have cover fields
      entry->setField(QStringLiteral("cover"), id);
    }
  }

  return entry;
}

Tellico::Fetch::FetchRequest AmazonFetcher::updateRequest(Data::EntryPtr entry_) {
  const int type = entry_->collection()->type();
  const QString t = entry_->field(QStringLiteral("title"));
  if(type == Data::Collection::Book || type == Data::Collection::ComicBook || type == Data::Collection::Bibtex) {
    const QString isbn = entry_->field(QStringLiteral("isbn"));
    if(!isbn.isEmpty()) {
      return FetchRequest(Fetch::ISBN, isbn);
    }
    const QString a = entry_->field(QStringLiteral("author"));
    if(!a.isEmpty()) {
      return t.isEmpty() ? FetchRequest(Fetch::Person, a)
                         : FetchRequest(Fetch::Keyword, t + QLatin1Char('-') + a);
    }
  } else if(type == Data::Collection::Album) {
    const QString a = entry_->field(QStringLiteral("artist"));
    if(!a.isEmpty()) {
      return t.isEmpty() ? FetchRequest(Fetch::Person, a)
                         : FetchRequest(Fetch::Keyword, t + QLatin1Char('-') + a);
    }
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }

  return FetchRequest();
}

QByteArray AmazonFetcher::requestPayload(FetchRequest request_) {
  QJsonObject payload;
  payload.insert(QLatin1String("PartnerTag"), m_assoc);
  payload.insert(QLatin1String("PartnerType"), QLatin1String("Associates"));
  payload.insert(QLatin1String("Operation"), QLatin1String("SearchItems"));
  payload.insert(QLatin1String("SortBy"), QLatin1String("Relevance"));
  // not mandatory
//  payload.insert(QLatin1String("Marketplace"), QLatin1String(siteData(m_site).host));
  if(m_page > 1) {
    payload.insert(QLatin1String("ItemPage"), m_page);
  }

  QJsonArray resources;
  resources.append(QLatin1String("ItemInfo.Title"));

  const int type = request_.collectionType;
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      payload.insert(QLatin1String("SearchIndex"), QLatin1String("Books"));
      resources.append(QLatin1String("ItemInfo.ExternalIds"));
      break;

    case Data::Collection::Album:
      payload.insert(QLatin1String("SearchIndex"), QLatin1String("Music"));
      break;

    case Data::Collection::Video:
      // CA and JP appear to have a bug where Video only returns VHS or Music results
      // DVD will return DVD, Blu-ray, etc. so just ignore VHS for those users
      payload.insert(QLatin1String("SearchIndex"), QLatin1String("MoviesAndTV"));
      if(m_site == CA || m_site == JP || m_site == IT || m_site == ES) {
        payload.insert(QStringLiteral("SearchIndex"), QStringLiteral("DVD"));
      } else {
        payload.insert(QStringLiteral("SearchIndex"), QStringLiteral("Video"));
      }
//      params.insert(QStringLiteral("SortIndex"), QStringLiteral("relevancerank"));
      break;

    case Data::Collection::Game:
      payload.insert(QLatin1String("SearchIndex"), QLatin1String("VideoGames"));
      break;

    case Data::Collection::BoardGame:
      payload.insert(QLatin1String("SearchIndex"), QLatin1String("ToysAndGames"));
//      params.insert(QStringLiteral("SortIndex"), QStringLiteral("relevancerank"));
      break;

    case Data::Collection::Coin:
    case Data::Collection::Stamp:
    case Data::Collection::Wine:
    case Data::Collection::Base:
    case Data::Collection::Card:
      myDebug() << "can't fetch this type:" << collectionType();
      return QByteArray();
  }

  switch(request_.key) {
    case Title:
      payload.insert(QLatin1String("Title"), request_.value);
      break;

    case Person:
      if(type == Data::Collection::Video) {
        payload.insert(QStringLiteral("Actor"), request_.value);
//        payload.insert(QStringLiteral("Director"), request_.value);
      } else if(type == Data::Collection::Album) {
        payload.insert(QStringLiteral("Artist"), request_.value);
      } else if(type == Data::Collection::Book) {
        payload.insert(QLatin1String("Author"), request_.value);
      } else {
        payload.insert(QLatin1String("Keywords"), request_.value);
      }
      break;

    case ISBN:
      {
        QString cleanValue = request_.value;
        cleanValue.remove(QLatin1Char('-'));
        // ISBN only get digits or 'X'
        QStringList isbns = FieldFormat::splitValue(cleanValue);
        // Amazon isbn13 search is still very flaky, so if possible, we're going to convert
        // all of them to isbn10. If we run into a 979 isbn13, then we're forced to do an
        // isbn13 search
        bool isbn13 = false;
        for(QStringList::Iterator it = isbns.begin(); it != isbns.end(); ) {
          if((*it).startsWith(QLatin1String("979"))) {
            isbn13 = true;
            break;
          }
          ++it;
        }
        // if we want isbn10, then convert all
        if(!isbn13) {
          for(QStringList::Iterator it = isbns.begin(); it != isbns.end(); ++it) {
            if((*it).length() > 12) {
              (*it) = ISBNValidator::isbn10(*it);
              (*it).remove(QLatin1Char('-'));
            }
          }
        }
        // limit to first 10
        while(isbns.size() > 10) {
          isbns.pop_back();
        }
        payload.insert(QLatin1String("Keywords"), isbns.join(QLatin1String("|")));
        if(isbn13) {
//          params.insert(QStringLiteral("IdType"), QStringLiteral("EAN"));
        }
      }
      break;

    case UPC:
      {
        QString cleanValue = request_.value;
        cleanValue.remove(QLatin1Char('-'));
        // for EAN values, add 0 to beginning if not 13 characters
        // in order to assume US country code from UPC value
        QStringList values;
        foreach(const QString& splitValue, cleanValue.split(FieldFormat::delimiterString())) {
          QString tmpValue = splitValue;
          if(m_site != US && tmpValue.length() == 12) {
            tmpValue.prepend(QLatin1Char('0'));
          }
          values << tmpValue;
          // limit to first 10 values
          if(values.length() >= 10) {
            break;
          }
        }

        payload.insert(QLatin1String("Keywords"), values.join(QLatin1String("|")));
      }
      break;

    case Keyword:
      payload.insert(QLatin1String("Keywords"), request_.value);
      break;

    case Raw:
      {
        QString key = request_.value.section(QLatin1Char('='), 0, 0).trimmed();
        QString str = request_.value.section(QLatin1Char('='), 1).trimmed();
        payload.insert(key, str);
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return QByteArray();
  }

  switch(m_imageSize) {
    case SmallImage:  resources.append(QLatin1String("Images.Primary.Small")); break;
    case MediumImage: resources.append(QLatin1String("Images.Primary.Medium")); break;
    case LargeImage:  resources.append(QLatin1String("Images.Primary.Large")); break;
    case NoImage: break;
  }

  payload.insert(QLatin1String("Resources"), resources);
  return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

Tellico::Data::CollPtr AmazonFetcher::createCollection() {
  Data::CollPtr coll = CollectionFactory::collection(collectionType(), true);
  if(!coll) {
    return coll;
  }

  QString imageFieldName;
  switch(m_imageSize) {
    case SmallImage:  imageFieldName = QStringLiteral("small-image"); break;
    case MediumImage: imageFieldName = QStringLiteral("medium-image"); break;
    case LargeImage:  imageFieldName = QStringLiteral("large-image"); break;
    case NoImage: break;
  }

  if(!imageFieldName.isEmpty()) {
    Data::FieldPtr field(new Data::Field(imageFieldName, QString(), Data::Field::URL));
    coll->addField(field);
  }

  if(optionalFields().contains(QStringLiteral("amazon"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("amazon"), i18n("Amazon Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  return coll;
}

void AmazonFetcher::populateEntry(Data::EntryPtr entry_, const QJsonObject& info_) {
  QVariantMap infoMap = info_.value(QLatin1String("ItemInfo")).toObject().toVariantMap();
  entry_->setField(QStringLiteral("title"), mapValue(infoMap, "Title", "DisplayValue"));
  const QString isbn = mapValue(infoMap, "ExternalIds", "ISBNs", "DisplayValues");
  if(!isbn.isEmpty()) {
    entry_->setField(QStringLiteral("isbn"), isbn);
  }

  QVariantMap imagesMap = info_.value(QLatin1String("Images")).toObject().toVariantMap();
  switch(m_imageSize) {
    case SmallImage:
      entry_->setField(QStringLiteral("small-image"), mapValue(imagesMap, "Primary", "Small", "URL"));
      break;
    case MediumImage:
      entry_->setField(QStringLiteral("medium-image"), mapValue(imagesMap, "Primary", "Medium", "URL"));
      break;
    case LargeImage:
      entry_->setField(QStringLiteral("large-image"), mapValue(imagesMap, "Primary", "Large", "URL"));
      break;
    case NoImage:
      break;
  }

  if(optionalFields().contains(QStringLiteral("amazon"))) {
    entry_->setField(QStringLiteral("amazon"), mapValue(info_.toVariantMap(), "DetailPageURL"));
  }
}

void AmazonFetcher::parseTitle(Tellico::Data::EntryPtr entry_) {
  // assume that everything in brackets or parentheses is extra
  QRegExp rx(QLatin1String("[\\(\\[](.*)[\\)\\]]"));
  rx.setMinimal(true);
  QString title = entry_->field(QStringLiteral("title"));
  int pos = rx.indexIn(title);
  while(pos > -1) {
    if(parseTitleToken(entry_, rx.cap(1))) {
      title.remove(pos, rx.matchedLength());
      --pos; // search again there
    }
    pos = rx.indexIn(title, pos+1);
  }
  entry_->setField(QStringLiteral("title"), title.trimmed());
}

bool AmazonFetcher::parseTitleToken(Tellico::Data::EntryPtr entry_, const QString& token_) {
//  myDebug() << "title token:" << token_;
  // if res = true, then the token gets removed from the title
  bool res = false;
  if(token_.indexOf(QLatin1String("widescreen"), 0, Qt::CaseInsensitive) > -1 ||
     token_.indexOf(i18n("Widescreen"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("widescreen"), QStringLiteral("true"));
    // res = true; leave it in the title
  } else if(token_.indexOf(QLatin1String("full screen"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  } else if(token_.indexOf(QLatin1String("import"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  }
  if(token_.indexOf(QLatin1String("blu-ray"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("medium"), i18n("Blu-ray"));
    res = true;
  } else if(token_.indexOf(QLatin1String("hd dvd"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("medium"), i18n("HD DVD"));
    res = true;
  } else if(token_.indexOf(QLatin1String("vhs"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("medium"), i18n("VHS"));
    res = true;
  }
  if(token_.indexOf(QLatin1String("director's cut"), 0, Qt::CaseInsensitive) > -1 ||
     token_.indexOf(i18n("Director's Cut"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("directors-cut"), QStringLiteral("true"));
    // res = true; leave it in the title
  }
  if(token_.toLower() == QLatin1String("ntsc")) {
    entry_->setField(QStringLiteral("format"), i18n("NTSC"));
    res = true;
  }
  if(token_.toLower() == QLatin1String("dvd")) {
    entry_->setField(QStringLiteral("medium"), i18n("DVD"));
    res = true;
  }
  static QRegExp regionRx(QLatin1String("Region [1-9]"));
  if(regionRx.indexIn(token_) > -1) {
    entry_->setField(QStringLiteral("region"), i18n(regionRx.cap(0).toUtf8().constData()));
    res = true;
  }
  if(entry_->collection()->type() == Data::Collection::Game) {
    Data::FieldPtr f = entry_->collection()->fieldByName(QStringLiteral("platform"));
    if(f && f->allowed().contains(token_)) {
      res = true;
    }
  }
  return res;
}

//static
QString AmazonFetcher::defaultName() {
  return i18n("Amazon.com Web Services");
}

QString AmazonFetcher::defaultIcon() {
  return favIcon("http://www.amazon.com");
}

Tellico::StringHash AmazonFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("keyword")] = i18n("Keywords");
  hash[QStringLiteral("amazon")] = i18n("Amazon Link");
  return hash;
}

Tellico::Fetch::ConfigWidget* AmazonFetcher::configWidget(QWidget* parent_) const {
  return new AmazonFetcher::ConfigWidget(parent_, this);
}

AmazonFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AmazonFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                AmazonFetcher::defaultName(),
                                QLatin1String("https://affiliate-program.amazon.com/gp/flex/advertising/api/sign-in.html")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_accessEdit = new QLineEdit(optionsWidget());
  connect(m_accessEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_accessEdit, row, 1);
  QString w = i18n("Access to data from Amazon.com requires an AWS Access Key ID and a Secret Key.");
  label->setWhatsThis(w);
  m_accessEdit->setWhatsThis(w);
  label->setBuddy(m_accessEdit);

  label = new QLabel(i18n("Secret key: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_secretKeyEdit = new QLineEdit(optionsWidget());
//  m_secretKeyEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  connect(m_secretKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_secretKeyEdit, row, 1);
  label->setWhatsThis(w);
  m_secretKeyEdit->setWhatsThis(w);
  label->setBuddy(m_secretKeyEdit);

  label = new QLabel(i18n("Country: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_siteCombo = new GUI::ComboBox(optionsWidget());
  for(int i = 0; i < XX; ++i) {
    const AmazonFetcher::SiteData& siteData = AmazonFetcher::siteData(i);
    QIcon icon(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                      QStringLiteral("kf5/locale/countries/%1/flag.png").arg(siteData.country)));
    m_siteCombo->addItem(icon, siteData.countryName, i);
    m_siteCombo->model()->sort(0);
  }

  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_siteCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  connect(m_siteCombo, activatedInt, this, &ConfigWidget::slotSiteChanged);
  l->addWidget(m_siteCombo, row, 1);
  w = i18n("Amazon.com provides data from several different localized sites. Choose the one "
           "you wish to use for this data source.");
  label->setWhatsThis(w);
  m_siteCombo->setWhatsThis(w);
  label->setBuddy(m_siteCombo);

  label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
  m_imageCombo->addItem(i18n("Medium Image"), MediumImage);
  m_imageCombo->addItem(i18n("Large Image"), LargeImage);
  m_imageCombo->addItem(i18n("No Image"), NoImage);
  connect(m_imageCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_imageCombo, row, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  label->setWhatsThis(w);
  m_imageCombo->setWhatsThis(w);
  label->setBuddy(m_imageCombo);

  label = new QLabel(i18n("&Associate's ID: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_assocEdit = new QLineEdit(optionsWidget());
  void (QLineEdit::* textChanged)(const QString&) = &QLineEdit::textChanged;
  connect(m_assocEdit, textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_assocEdit, row, 1);
  w = i18n("The associate's id identifies the person accessing the Amazon.com Web Services, and is included "
           "in any links to the Amazon.com site.");
  label->setWhatsThis(w);
  m_assocEdit->setWhatsThis(w);
  label->setBuddy(m_assocEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_siteCombo->setCurrentData(fetcher_->m_site);
    m_accessEdit->setText(fetcher_->m_accessKey);
    m_secretKeyEdit->setText(fetcher_->m_secretKey);
    m_assocEdit->setText(fetcher_->m_assoc);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_assocEdit->setText(QLatin1String(AMAZON_ASSOC_TOKEN));
    m_imageCombo->setCurrentData(MediumImage);
  }

  addFieldsWidget(AmazonFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  KAcceleratorManager::manage(optionsWidget());
}

void AmazonFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  int n = m_siteCombo->currentData().toInt();
  config_.writeEntry("Site", n);
  QString s = m_accessEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("AccessKey", s);
  }
  s = m_secretKeyEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("SecretKey", s);
  }
  s = m_assocEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("AssocToken", s);
  }
  n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}

QString AmazonFetcher::ConfigWidget::preferredName() const {
  return AmazonFetcher::siteData(m_siteCombo->currentData().toInt()).title;
}

void AmazonFetcher::ConfigWidget::slotSiteChanged() {
  emit signalName(preferredName());
}
