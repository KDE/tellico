/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../images/imagefactory.h"
#include "../gui/guiproxy.h"
#include "../collection.h"
#include "../document.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../tellico_utils.h"
#include "../utils/isbnvalidator.h"
#include "../utils/wallet.h"
#include "../gui/combobox.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <klineedit.h>
#include <kseparator.h>
#include <kcombobox.h>
#include <kacceleratormanager.h>
#include <KConfigGroup>

#include <QDomDocument>
#include <QLabel>
#include <QCheckBox>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QGridLayout>

namespace {
  static const int AMAZON_RETURNS_PER_REQUEST = 10;
  static const int AMAZON_MAX_RETURNS_TOTAL = 20;
  static const char* AMAZON_ASSOC_TOKEN = "tellico-20";
  // need to have these in the translation file
  static const char* linkText = I18N_NOOP("Amazon Link");
}

using namespace Tellico;
using Tellico::Fetch::AmazonFetcher;

// static
const AmazonFetcher::SiteData& AmazonFetcher::siteData(int site_) {
  static SiteData dataVector[6] = {
    {
      i18n("Amazon (US)"),
      KUrl("http://webservices.amazon.com/onca/xml")
    }, {
      i18n("Amazon (UK)"),
      KUrl("http://webservices.amazon.co.uk/onca/xml")
    }, {
      i18n("Amazon (Germany)"),
      KUrl("http://webservices.amazon.de/onca/xml")
    }, {
      i18n("Amazon (Japan)"),
      KUrl("http://webservices.amazon.co.jp/onca/xml")
    }, {
      i18n("Amazon (France)"),
      KUrl("http://webservices.amazon.fr/onca/xml")
    }, {
      i18n("Amazon (Canada)"),
      KUrl("http://webservices.amazon.ca/onca/xml")
    }
  };

  return dataVector[site_];
}

AmazonFetcher::AmazonFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0), m_site(Unknown), m_imageSize(MediumImage),
      m_assoc(QLatin1String(AMAZON_ASSOC_TOKEN)), m_addLinkField(true), m_limit(AMAZON_MAX_RETURNS_TOTAL),
      m_countOffset(0), m_page(1), m_total(-1), m_numResults(0), m_job(0), m_started(false) {
  (void)linkText; // just to shut up the compiler
}

AmazonFetcher::~AmazonFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString AmazonFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString AmazonFetcher::attribution() const {
  return i18n("This data is licensed under <a href=""%1"">specific terms</a>.")
         .arg(QLatin1String("https://affiliate-program.amazon.com/gp/advertising/api/detail/agreement.html"));
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
         || (k == UPC && m_site != CA)
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
    m_access = s;
  }
  s = config_.readEntry("AssocToken");
  if(!s.isEmpty()) {
    m_assoc = s;
  }
  s = config_.readEntry("SecretKey");
  if(!s.isEmpty()) {
    m_amazonKey = s.toUtf8();
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
  // calling secretKey() ensures that we try to read it first
  if(secretKey().isEmpty() || m_access.isEmpty()) {
    // this message is split in two since the first half is reused later
    message(i18n("Access to data from Amazon.com requires an AWS Access Key ID and a Secret Key.") +
            QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

//  myDebug() << "value = " << request().value;
//  myDebug() << "getting page " << m_page;

  QMap<QString, QString> params;
  params.insert(QLatin1String("Service"),        QLatin1String("AWSECommerceService"));
  params.insert(QLatin1String("AssociateTag"),   m_assoc);
  params.insert(QLatin1String("AWSAccessKeyId"), m_access);
  params.insert(QLatin1String("Operation"),      QLatin1String("ItemSearch"));
  params.insert(QLatin1String("ResponseGroup"),  QLatin1String("Large"));
  params.insert(QLatin1String("ItemPage"),       QString::number(m_page));
  params.insert(QLatin1String("Version"),        QLatin1String("2009-11-02"));

  const int type = collectionType();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      params.insert(QLatin1String("SearchIndex"), QLatin1String("Books"));
      params.insert(QLatin1String("SortIndex"), QLatin1String("relevancerank"));
      break;

    case Data::Collection::Album:
      params.insert(QLatin1String("SearchIndex"), QLatin1String("Music"));
      break;

    case Data::Collection::Video:
      // CA and JP appear to have a bug where Video only returns VHS or Music results
      // DVD will return DVD, Blu-ray, etc. so just ignore VHS for those users
      if(m_site == CA || m_site == JP) {
        params.insert(QLatin1String("SearchIndex"), QLatin1String("DVD"));
      } else {
        params.insert(QLatin1String("SearchIndex"), QLatin1String("Video"));
      }
      params.insert(QLatin1String("SortIndex"), QLatin1String("relevancerank"));
      break;

    case Data::Collection::Game:
      params.insert(QLatin1String("SearchIndex"), QLatin1String("VideoGames"));
      break;

    case Data::Collection::BoardGame:
      params.insert(QLatin1String("SearchIndex"), QLatin1String("Toys"));
      params.insert(QLatin1String("SortIndex"), QLatin1String("relevancerank"));
      break;

    case Data::Collection::Coin:
    case Data::Collection::Stamp:
    case Data::Collection::Wine:
    case Data::Collection::Base:
    case Data::Collection::Card:
      myDebug() << "can't fetch this type:" << collectionType();
      stop();
      return;
  }

  QString value = request().value;

  switch(request().key) {
    case Title:
      params.insert(QLatin1String("Title"), value);
      break;

    case Person:
      if(type == Data::Collection::Video) {
        params.insert(QLatin1String("Actor"),        value);
        params.insert(QLatin1String("Director"),     value);
      } else if(type == Data::Collection::Album) {
        params.insert(QLatin1String("Artist"),       value);
      } else if(type == Data::Collection::Game) {
        params.insert(QLatin1String("Manufacturer"), value);
      } else { // books and bibtex
        QString s = QString::fromLatin1("author:%1 or publisher:%2").arg(value, value);
//        params.insert(QLatin1String("Author"),       value, mib);
//        params.insert(QLatin1String("Publisher"),    value, mib);
        params.insert(QLatin1String("Power"),    s);
      }
      break;

    case ISBN:
      {
        params.insert(QLatin1String("Operation"), QLatin1String("ItemLookup"));

        QString cleanValue = value;
        cleanValue.remove(QLatin1Char('-'));
        // ISBN only get digits or 'X'
        QStringList isbns = FieldFormat::splitValue(cleanValue);
        // Amazon isbn13 search is still very flaky, so if possible, we're going to convert
        // all of them to isbn10. If we run into a 979 isbn13, then we're forced to do an
        // isbn13 search
        bool isbn13 = false;
        for(QStringList::Iterator it = isbns.begin(); it != isbns.end(); ) {
          if((*it).startsWith(QLatin1String("979"))) {
            if(m_site == JP) { // never works for JP
              myWarning() << "ISBN-13 searching not implemented for Japan";
              it = isbns.erase(it);
              continue;
            }
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
          // the default search is by ASIN, which prohibits SearchIndex
          params.remove(QLatin1String("SearchIndex"));
        }
        // limit to first 10
        while(isbns.size() > 10) {
          isbns.pop_back();
        }
        params.insert(QLatin1String("ItemId"), isbns.join(QLatin1String(",")));
        if(isbn13) {
          params.insert(QLatin1String("IdType"), QLatin1String("EAN"));
        }
      }
      break;

    case UPC:
      {
        params.insert(QLatin1String("Operation"), QLatin1String("ItemLookup"));
        // US allows UPC, all others are EAN
        if(m_site == US) {
          params.insert(QLatin1String("IdType"), QLatin1String("UPC"));
        } else {
          params.insert(QLatin1String("IdType"), QLatin1String("EAN"));
        }
        QString cleanValue = value;
        cleanValue.remove(QLatin1Char('-'));
        // limit to first 10
        cleanValue.replace(FieldFormat::delimiterString(), QLatin1String(","));
        cleanValue = cleanValue.section(QLatin1Char(','), 0, 9);
        params.insert(QLatin1String("ItemId"), cleanValue);
      }
      break;

    case Keyword:
      params.insert(QLatin1String("Keywords"), value);
      break;

    case Raw:
      {
        QString key = value.section(QLatin1Char('='), 0, 0).trimmed();
        QString str = value.section(QLatin1Char('='), 1).trimmed();
        params.insert(key, str);
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }

  AmazonRequest request(siteData(m_site).url, m_amazonKey);
  KUrl newUrl = request.signedRequest(params);
//  myDebug() << newUrl;

  m_job = KIO::storedGet(newUrl, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void AmazonFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug();
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_started = false;
  emit signalDone(this);
}

void AmazonFetcher::slotComplete(KJob*) {
//  myDebug();

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

#if 0
  myWarning() << "Remove debug from amazonfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test%1.xml").arg(m_page));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec(QTextCodec::codecForName("UTF-8"));
    t << data;
  }
  f.close();
#endif

  QStringList errors;
  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data, false)) {
      myWarning() << "server did not return valid XML.";
      stop();
      return;
    }
    // find TotalResults element
    // it's in the first level under the root element
    //ItemSearchResponse/Items/TotalResults
    QDomNode n = dom.documentElement().namedItem(QLatin1String("Items"))
                                      .namedItem(QLatin1String("TotalResults"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.text().toInt();
    }
    n = dom.documentElement().namedItem(QLatin1String("Items"))
                             .namedItem(QLatin1String("Request"))
                             .namedItem(QLatin1String("Errors"));
    e = n.toElement();
    if(!e.isNull()) {
      QDomNodeList nodes = e.elementsByTagName(QLatin1String("Error"));
      for(int i = 0; i < nodes.count(); ++i) {
        e = nodes.item(i).toElement().namedItem(QLatin1String("Code")).toElement();
        if(!e.isNull() && e.text() == QLatin1String("AWS.ECommerceService.NoExactMatches")) {
          // no exact match, not a real error, so skip
          continue;
        }
        // for some reason, Amazon will return an error simply when a valid ISBN is not found
        // I really want to ignore that, so check the IsValid element in the Request element
        QDomNode isValidNode = n.parentNode().namedItem(QLatin1String("IsValid"));
        if(request().key == ISBN && isValidNode.toElement().text().toLower() == QLatin1String("true")) {
          continue;
        }
        e = nodes.item(i).toElement().namedItem(QLatin1String("Message")).toElement();
        if(!e.isNull()) {
          errors << e.text();
        }
      }
    }
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

//  QRegExp stripHTML(QLatin1String("<.*>"), true);
//  stripHTML.setMinimal(true);

  // assume amazon is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data, data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "no collection pointer";
    stop();
    return;
  }

  if(!m_addLinkField) {
    // remove amazon field if it's not to be added
    coll->removeField(QLatin1String("amazon"));
  }

  Data::EntryList entries = coll->entries();
  if(entries.isEmpty() && !errors.isEmpty()) {
    for(QStringList::ConstIterator it = errors.constBegin(); it != errors.constEnd(); ++it) {
      myDebug() << "AmazonFetcher::" << *it;
    }
    message(errors[0], MessageHandler::Error);
    stop();
    return;
  }

  int count = -1;
  foreach(Data::EntryPtr entry, entries) {
    ++count;
    if(m_numResults >= m_limit) {
      break;
    }
    if(count < m_countOffset) {
      continue;
    }
    if(!m_started) {
      // might get aborted
      break;
    }

    // special case book author
    // amazon is really bad about not putting spaces after periods
    if(coll->type() == Data::Collection::Book) {
      QRegExp rx(QLatin1String("\\.([^\\s])"));
      QStringList values = FieldFormat::splitValue(entry->field(QLatin1String("author")));
      for(QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
        (*it).replace(rx, QLatin1String(". \\1"));
      }
      entry->setField(QLatin1String("author"), values.join(FieldFormat::delimiterString()));
    }

    // UK puts the year in the title for some reason
    if(m_site == UK && coll->type() == Data::Collection::Video) {
      QRegExp rx(QLatin1String("\\[(\\d{4})\\]"));
      QString t = entry->title();
      if(rx.indexIn(t) > -1) {
        QString y = rx.cap(1);
        t.remove(rx).simplified();
        entry->setField(QLatin1String("title"), t);
        if(entry->field(QLatin1String("year")).isEmpty()) {
          entry->setField(QLatin1String("year"), y);
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
      {
        StringSet newWords;
        const QStringList keywords = FieldFormat::splitValue(entry->field(QLatin1String("keyword")));
        foreach(const QString& keyword, keywords) {
          // the amazon2tellico stylesheet separates keywords with '/'
          const QStringList words = keyword.split(QLatin1Char('/'));
          foreach(const QString& word, words) {
            if(word == QLatin1String("General") ||
               word == QLatin1String("Subjects") ||
               word == QLatin1String("Par prix") || // french stuff
               word == QLatin1String("Divers") || // french stuff
               word.startsWith(QLatin1Char('(')) ||
               word.startsWith(QLatin1String("Authors"))) {
              continue;
            }
            newWords.add(word);
          }
        }
        entry->setField(QLatin1String("keyword"), newWords.toList().join(FieldFormat::delimiterString()));
      }
      entry->setField(QLatin1String("comments"), Tellico::decodeHTML(entry->field(QLatin1String("comments"))));
      break;

    case Data::Collection::Video:
      {
        const QString genres = QLatin1String("genre");
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
        words.add(FieldFormat::splitValue(entry->field(QLatin1String("language"))));
        entry->setField(QLatin1String("language"), words.toList().join(FieldFormat::delimiterString()));
      }
      entry->setField(QLatin1String("plot"), Tellico::decodeHTML(entry->field(QLatin1String("plot"))));
      break;

    case Data::Collection::Album:
      {
        const QString genres = QLatin1String("genre");
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
      entry->setField(QLatin1String("comments"), Tellico::decodeHTML(entry->field(QLatin1String("comments"))));
      break;

    case Data::Collection::Game:
      entry->setField(QLatin1String("description"), Tellico::decodeHTML(entry->field(QLatin1String("description"))));
      break;
  }

  // clean up the title
  parseTitle(entry, type);

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

  KUrl imageURL;
  switch(m_imageSize) {
    case SmallImage:
      imageURL = entry->field(QLatin1String("small-image"));
      break;
    case MediumImage:
      imageURL = entry->field(QLatin1String("medium-image"));
      break;
    case LargeImage:
      imageURL = entry->field(QLatin1String("large-image"));
      break;
    case NoImage:
    default:
      break;
  }
//  myDebug() << "grabbing " << imageURL.prettyUrl();
  if(!imageURL.isEmpty()) {
    QString id = ImageFactory::addImage(imageURL, false);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    } else { // amazon serves up 1x1 gifs occasionally, but that's caught in the image constructor
      // all relevant collection types have cover fields
      entry->setField(QLatin1String("cover"), id);
    }
  }

  // don't want to show image urls in the fetch dialog
  entry->setField(QLatin1String("small-image"),  QString());
  entry->setField(QLatin1String("medium-image"), QString());
  entry->setField(QLatin1String("large-image"),  QString());
  return entry;
}

void AmazonFetcher::initXSLTHandler() {
  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("amazon2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate amazon2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in amazon2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::FetchRequest AmazonFetcher::updateRequest(Data::EntryPtr entry_) {
  const int type = entry_->collection()->type();
  const QString t = entry_->field(QLatin1String("title"));
  if(type == Data::Collection::Book || type == Data::Collection::ComicBook || type == Data::Collection::Bibtex) {
    const QString isbn = entry_->field(QLatin1String("isbn"));
    if(!isbn.isEmpty()) {
      return FetchRequest(Fetch::ISBN, isbn);
    }
    const QString a = entry_->field(QLatin1String("author"));
    if(!a.isEmpty()) {
      return t.isEmpty() ? FetchRequest(Fetch::Person, a)
                         : FetchRequest(Fetch::Keyword, t + QLatin1Char('-') + a);
    }
  } else if(type == Data::Collection::Album) {
    const QString a = entry_->field(QLatin1String("artist"));
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

void AmazonFetcher::parseTitle(Tellico::Data::EntryPtr entry, int collType) {
  Q_UNUSED(collType);
  // assume that everything in brackets or parentheses is extra
  QRegExp rx(QLatin1String("[\\(\\[](.*)[\\)\\]]"));
  rx.setMinimal(true);
  QString title = entry->field(QLatin1String("title"));
  int pos = rx.indexIn(title);
  while(pos > -1) {
    if(parseTitleToken(entry, rx.cap(1))) {
      title.remove(pos, rx.matchedLength());
      --pos; // search again there
    }
    pos = rx.indexIn(title, pos+1);
  }
  entry->setField(QLatin1String("title"), title.trimmed());
}

bool AmazonFetcher::parseTitleToken(Tellico::Data::EntryPtr entry, const QString& token) {
//  myDebug() << "title token:" << token;
  // if res = true, then the token gets removed from the title
  bool res = false;
  if(token.indexOf(QLatin1String("widescreen"), 0, Qt::CaseInsensitive) > -1 ||
     token.indexOf(i18n("Widescreen"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("widescreen"), QLatin1String("true"));
    // res = true; leave it in the title
  } else if(token.indexOf(QLatin1String("full screen"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  } else if(token.indexOf(QLatin1String("import"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  }
  if(token.indexOf(QLatin1String("blu-ray"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("medium"), i18n("Blu-ray"));
    res = true;
  } else if(token.indexOf(QLatin1String("hd dvd"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("medium"), i18n("HD DVD"));
    res = true;
  } else if(token.indexOf(QLatin1String("vhs"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("medium"), i18n("VHS"));
    res = true;
  }
  if(token.indexOf(QLatin1String("director's cut"), 0, Qt::CaseInsensitive) > -1 ||
     token.indexOf(i18n("Director's Cut"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("directors-cut"), QLatin1String("true"));
    // res = true; leave it in the title
  }
  if(token.toLower() == QLatin1String("ntsc")) {
    entry->setField(QLatin1String("format"), i18n("NTSC"));
    res = true;
  }
  if(token.toLower() == QLatin1String("dvd")) {
    entry->setField(QLatin1String("medium"), i18n("DVD"));
    res = true;
  }
  static QRegExp regionRx(QLatin1String("Region [1-9]"));
  if(regionRx.indexIn(token) > -1) {
    entry->setField(QLatin1String("region"), i18n(regionRx.cap(0).toUtf8()));
    res = true;
  }
  return res;
}

QString AmazonFetcher::secretKey() const {
  if(m_amazonKey.isEmpty()) {
    QByteArray maybeKey = Wallet::self()->readWalletEntry(m_access);
    if(!maybeKey.isNull()) {
      m_amazonKey = maybeKey;
    } else {
      myDebug() << "no amazon secret key found for" << source();
    }
  }
  return QString::fromUtf8(m_amazonKey);
}

//static
QString AmazonFetcher::defaultName() {
  return i18n("Amazon.com Web Services");
}

QString AmazonFetcher::defaultIcon() {
  return favIcon("http://amazon.com");
}

Tellico::StringHash AmazonFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("keyword")] = i18n("Keywords");
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
  m_accessEdit = new KLineEdit(optionsWidget());
  connect(m_accessEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_accessEdit, row, 1);
  QString w = i18n("Access to data from Amazon.com requires an AWS Access Key ID and a Secret Key.");
  label->setWhatsThis(w);
  m_accessEdit->setWhatsThis(w);
  label->setBuddy(m_accessEdit);

  label = new QLabel(i18n("Secret key: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_secretKeyEdit = new KLineEdit(optionsWidget());
//  m_secretKeyEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  connect(m_secretKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_secretKeyEdit, row, 1);
  label->setWhatsThis(w);
  m_secretKeyEdit->setWhatsThis(w);
  label->setBuddy(m_secretKeyEdit);

  label = new QLabel(i18n("Co&untry: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_siteCombo = new GUI::ComboBox(optionsWidget());
  m_siteCombo->addItem(i18n("United States"), US);
  m_siteCombo->addItem(i18n("United Kingdom"), UK);
  m_siteCombo->addItem(i18n("Germany"), DE);
  m_siteCombo->addItem(i18n("Japan"), JP);
  m_siteCombo->addItem(i18n("France"), FR);
  m_siteCombo->addItem(i18n("Canada"), CA);
  connect(m_siteCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_siteCombo, SIGNAL(activated(int)), SLOT(slotSiteChanged()));
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
  connect(m_imageCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_imageCombo, row, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  label->setWhatsThis(w);
  m_imageCombo->setWhatsThis(w);
  label->setBuddy(m_imageCombo);

  label = new QLabel(i18n("&Associate's ID: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_assocEdit = new KLineEdit(optionsWidget());
  connect(m_assocEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_assocEdit, row, 1);
  w = i18n("The associate's id identifies the person accessing the Amazon.com Web Services, and is included "
           "in any links to the Amazon.com site.");
  label->setWhatsThis(w);
  m_assocEdit->setWhatsThis(w);
  label->setBuddy(m_assocEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_siteCombo->setCurrentData(fetcher_->m_site);
    m_accessEdit->setText(fetcher_->m_access);
    m_secretKeyEdit->setText(fetcher_->secretKey());
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
  QByteArray b = m_secretKeyEdit->text().trimmed().toUtf8();
  if(!b.isEmpty()) {
    config_.writeEntry("SecretKey", b);
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

#include "amazonfetcher.moc"
