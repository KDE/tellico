/***************************************************************************
    copyright            : (C) 2004-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "amazonfetcher.h"
#include "messagehandler.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../imagefactory.h"
#include "../tellico_kernel.h"
#include "../collection.h"
#include "../document.h"
#include "../entry.h"
#include "../field.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"
#include "../isbnvalidator.h"
#include "../gui/combobox.h"

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
#include <QGridLayout>

namespace {
  static const int AMAZON_RETURNS_PER_REQUEST = 10;
  static const int AMAZON_MAX_RETURNS_TOTAL = 20;
  static const char* AMAZON_ACCESS_KEY = "0834VQ4S71KYPVSYQD02";
  static const char* AMAZON_ASSOC_TOKEN = "tellico-20";
  // need to have these in the translation file
  static const char* linkText = I18N_NOOP("Amazon Link");
}

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

AmazonFetcher::AmazonFetcher(Site site_, QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(0), m_site(site_), m_imageSize(MediumImage),
      m_access(QLatin1String(AMAZON_ACCESS_KEY)),
      m_assoc(QLatin1String(AMAZON_ASSOC_TOKEN)), m_addLinkField(true), m_limit(AMAZON_MAX_RETURNS_TOTAL),
      m_countOffset(0), m_page(1), m_total(-1), m_numResults(0), m_job(0), m_started(false) {
  m_name = siteData(site_).title;
  (void)linkText; // just to shut up the compiler
}

AmazonFetcher::~AmazonFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString AmazonFetcher::defaultName() {
  return i18n("Amazon.com Web Services");
}

QString AmazonFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
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

void AmazonFetcher::readConfigHook(const KConfigGroup& config_) {
  QString s = config_.readEntry("AccessKey");
  if(!s.isEmpty()) {
    m_access = s;
  }
  s = config_.readEntry("AssocToken");
  if(!s.isEmpty()) {
    m_assoc = s;
  }
  int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
  m_fields = config_.readEntry("Custom Fields", QStringList() << QLatin1String("keyword"));
}

void AmazonFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_.trimmed();
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
//  myDebug() << "AmazonFetcher::doSearch() - value = " << m_value << endl;
//  myDebug() << "AmazonFetcher::doSearch() - getting page " << m_page << endl;

  const SiteData& data = siteData(m_site);
  KUrl u = data.url;
  u.addQueryItem(QLatin1String("Service"),        QLatin1String("AWSECommerceService"));
  u.addQueryItem(QLatin1String("AssociateTag"),   m_assoc);
  u.addQueryItem(QLatin1String("AWSAccessKeyId"), m_access);
  u.addQueryItem(QLatin1String("Operation"),      QLatin1String("ItemSearch"));
  u.addQueryItem(QLatin1String("ResponseGroup"),  QLatin1String("Large"));
  u.addQueryItem(QLatin1String("ItemPage"),       QString::number(m_page));
  u.addQueryItem(QLatin1String("Version"),        QLatin1String("2007-10-29"));

  const int type = Kernel::self()->collectionType();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      u.addQueryItem(QLatin1String("SearchIndex"), QLatin1String("Books"));
      u.addQueryItem(QLatin1String("SortIndex"), QLatin1String("relevancerank"));
      break;

    case Data::Collection::Album:
      u.addQueryItem(QLatin1String("SearchIndex"), QLatin1String("Music"));
      break;

    case Data::Collection::Video:
      u.addQueryItem(QLatin1String("SearchIndex"), QLatin1String("Video"));
      u.addQueryItem(QLatin1String("SortIndex"), QLatin1String("relevancerank"));
      break;

    case Data::Collection::Game:
      u.addQueryItem(QLatin1String("SearchIndex"), QLatin1String("VideoGames"));
      break;

    case Data::Collection::BoardGame:
      u.addQueryItem(QLatin1String("SearchIndex"), QLatin1String("Toys"));
      u.addQueryItem(QLatin1String("SortIndex"), QLatin1String("relevancerank"));
      break;

    case Data::Collection::Coin:
    case Data::Collection::Stamp:
    case Data::Collection::Wine:
    case Data::Collection::Base:
    case Data::Collection::Card:
      message(i18n("%1 does not allow searching for this collection type.", source()), MessageHandler::Warning);
      stop();
      return;
  }

  // I have not been able to find any documentation about what character set to use
  // when URL encoding the search term in the Amazon REST interface. But I do know
  // that utf8 DOES NOT WORK. So I'm arbitrarily using iso-8859-1, except for JP.
  // Why different for JP? Well, I've not received any bug reports from that direction yet

//  QString value = KUrl::decode_string(value_, 106);
//  QString value = QString::fromLocal8Bit(value_.toUtf8());
  QString value = m_value;
  // a mibenum of 106 is utf-8, 4 is iso-8859-1, 0 means use user's locale,
//  int mib = m_site == AmazonFetcher::JP ? 106 : 4;

  switch(m_key) {
    case Title:
      u.addQueryItem(QLatin1String("Title"), value);
      break;

    case Person:
      if(type == Data::Collection::Video) {
        u.addQueryItem(QLatin1String("Actor"),        value);
        u.addQueryItem(QLatin1String("Director"),     value);
      } else if(type == Data::Collection::Album) {
        u.addQueryItem(QLatin1String("Artist"),       value);
      } else if(type == Data::Collection::Game) {
        u.addQueryItem(QLatin1String("Manufacturer"), value);
      } else { // books and bibtex
        QString s = QString::fromLatin1("author:%1 or publisher:%2").arg(value, value);
//        u.addQueryItem(QLatin1String("Author"),       value, mib);
//        u.addQueryItem(QLatin1String("Publisher"),    value, mib);
        u.addQueryItem(QLatin1String("Power"),    s);
      }
      break;

    case ISBN:
      {
        u.removeQueryItem(QLatin1String("Operation"));
        u.addQueryItem(QLatin1String("Operation"), QLatin1String("ItemLookup"));

        QString s = m_value; // not encValue!!!
        s.remove(QLatin1Char('-'));
        // ISBN only get digits or 'X', and multiple values are connected with "; "
        QStringList isbns = s.split(QLatin1String("; "));
        // Amazon isbn13 search is still very flaky, so if possible, we're going to convert
        // all of them to isbn10. If we run into a 979 isbn13, then we're forced to do an
        // isbn13 search
        bool isbn13 = false;
        for(QStringList::Iterator it = isbns.begin(); it != isbns.end(); ++it) {
          if(m_value.startsWith(QLatin1String("979"))) {
            if(m_site == JP) { // never works for JP
              myWarning() << "AmazonFetcher:doSearch() - ISBN-13 searching not implemented for Japan";
              isbns.erase(it); // automatically skips to next
              continue;
            }
            isbn13 = true;
            break;
          }
          ++it;
        }
        // if we want isbn10, then convert all
        if(!isbn13) {
          for(QStringList::Iterator it = isbns.begin(); it != isbns.end(); ) {
            if((*it).length() > 12) {
              (*it) = ISBNValidator::isbn10(*it);
              (*it).remove(QLatin1Char('-'));
            }
          }
          // the default search is by ASIN, which prohibits SearchIndex
          u.removeQueryItem(QLatin1String("SearchIndex"));
        }
        // limit to first 10
        while(isbns.size() > 10) {
          isbns.pop_back();
        }
        u.addQueryItem(QLatin1String("ItemId"), isbns.join(QLatin1String(",")));
        if(isbn13) {
          u.addQueryItem(QLatin1String("IdType"), QLatin1String("EAN"));
        }
      }
      break;

    case UPC:
      {
        u.removeQueryItem(QLatin1String("Operation"));
        u.addQueryItem(QLatin1String("Operation"), QLatin1String("ItemLookup"));
        // US allows UPC, all others are EAN
        if(m_site == US) {
          u.addQueryItem(QLatin1String("IdType"), QLatin1String("UPC"));
        } else {
          u.addQueryItem(QLatin1String("IdType"), QLatin1String("EAN"));
        }
        QString s = m_value; // not encValue!!!
        s.remove(QLatin1Char('-'));
        // limit to first 10
        s.replace(QLatin1String("; "), QLatin1String(","));
        s = s.section(QLatin1Char(','), 0, 9);
        u.addQueryItem(QLatin1String("ItemId"), s);
      }
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("Keywords"), m_value);
      break;

    case Raw:
      {
        QString key = value.section(QLatin1Char('='), 0, 0).trimmed();
        QString str = value.section(QLatin1Char('='), 1).trimmed();
        u.addQueryItem(key, str);
      }
      break;

    default:
      kWarning() << "AmazonFetcher::doSearch() - key not recognized: " << m_key;
      stop();
      return;
  }
//  myDebug() << "AmazonFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(Kernel::self()->widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void AmazonFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug() << "AmazonFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_started = false;
  emit signalDone(this);
}

void AmazonFetcher::slotComplete(KJob*) {
//  myDebug() << "AmazonFetcher::slotComplete()" << endl;

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "AmazonFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

#if 0
  kWarning() << "Remove debug from amazonfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test%1.xml").arg(m_page));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << data;
  }
  f.close();
#endif

  QStringList errors;
  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(data, false)) {
      kWarning() << "AmazonFetcher::slotComplete() - server did not return valid XML.";
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
        if(m_key == ISBN && isValidNode.toElement().text().toLower() == QLatin1String("true")) {
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
    myDebug() << "AmazonFetcher::slotComplete() - no collection pointer" << endl;
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
      myDebug() << "AmazonFetcher::" << *it << endl;
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
      QStringList values = entry->fields(QLatin1String("author"), false);
      for(QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
        (*it).replace(rx, QLatin1String(". \\1"));
      }
      entry->setField(QLatin1String("author"), values.join(QLatin1String("; ")));
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

    QString desc;
    switch(coll->type()) {
      case Data::Collection::Book:
      case Data::Collection::ComicBook:
      case Data::Collection::Bibtex:
        desc = entry->field(QLatin1String("author"))
               + QLatin1Char('/') + entry->field(QLatin1String("publisher"));
        if(!entry->field(QLatin1String("cr_year")).isEmpty()) {
          desc += QLatin1Char('/') + entry->field(QLatin1String("cr_year"));
        } else if(!entry->field(QLatin1String("pub_year")).isEmpty()){
          desc += QLatin1Char('/') + entry->field(QLatin1String("pub_year"));
        }
        break;

      case Data::Collection::Video:
        desc = entry->field(QLatin1String("studio"))
               + QLatin1Char('/')
               + entry->field(QLatin1String("director"))
               + QLatin1Char('/')
               + entry->field(QLatin1String("year"))
               + QLatin1Char('/')
               + entry->field(QLatin1String("medium"));
        break;

      case Data::Collection::Album:
        desc = entry->field(QLatin1String("artist"))
               + QLatin1Char('/')
               + entry->field(QLatin1String("label"))
               + QLatin1Char('/')
               + entry->field(QLatin1String("year"));
        break;

      case Data::Collection::Game:
        desc = entry->field(QLatin1String("platform"))
               + QLatin1Char('/')
               + entry->field(QLatin1String("year"));
        break;

      case Data::Collection::BoardGame:
        desc = entry->field(QLatin1String("publisher"))
               + QLatin1Char('/')
               + entry->field(QLatin1String("year"));
        break;

      default:
        break;
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
//    myDebug() << "AmazonFetcher::slotComplete() - " << entry->title() << endl;
    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry->title(), desc, entry->field(QLatin1String("isbn")));
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
  } else if(m_value.count(QLatin1Char(';')) > 9) {
    search(m_key, m_value.section(QLatin1Char(';'), 10));
  } else {
    m_countOffset = m_entries.count() % AMAZON_RETURNS_PER_REQUEST;
    if(m_countOffset == 0) {
      ++m_page; // need to go to next page
    }
    stop();
  }
}

Tellico::Data::EntryPtr AmazonFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    kWarning() << "AmazonFetcher::fetchEntry() - no entry in dict";
    return entry;
  }

  QStringList defaultFields = customFields().keys();
  for(QStringList::Iterator it = defaultFields.begin(); it != defaultFields.end(); ++it) {
    if(!m_fields.contains(*it)) {
      entry->setField(*it, QString());
    }
  }

  // do what we can to remove useless keywords
  const int type = Kernel::self()->collectionType();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      {
        const QString keywords = QLatin1String("keyword");
        QStringList oldWords = entry->fields(keywords, false);
        StringSet words;
        for(QStringList::Iterator it = oldWords.begin(); it != oldWords.end(); ++it) {
          // the amazon2tellico stylesheet separates keywords with '/'
          QStringList nodes = (*it).split(QLatin1Char('/'));
          for(QStringList::Iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            if(*it2 == QLatin1String("General") ||
               *it2 == QLatin1String("Subjects") ||
               *it2 == QLatin1String("Par prix") || // french stuff
               *it2 == QLatin1String("Divers") || // french stuff
               (*it2).startsWith(QLatin1Char('(')) ||
               (*it2).startsWith(QLatin1String("Authors"))) {
              continue;
            }
            words.add(*it2);
          }
        }
        entry->setField(keywords, words.toList().join(QLatin1String("; ")));
      }
      entry->setField(QLatin1String("comments"), Tellico::decodeHTML(entry->field(QLatin1String("comments"))));
      break;

    case Data::Collection::Video:
      {
        const QString genres = QLatin1String("genre");
        QStringList oldWords = entry->fields(genres, false);
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
        entry->setField(genres, words.toList().join(QLatin1String("; ")));
        // language tracks get duplicated, too
        words.clear();
        words.add(entry->fields(QLatin1String("language"), false));
        entry->setField(QLatin1String("language"), words.toList().join(QLatin1String("; ")));
      }
      entry->setField(QLatin1String("plot"), Tellico::decodeHTML(entry->field(QLatin1String("plot"))));
      break;

    case Data::Collection::Album:
      {
        const QString genres = QLatin1String("genre");
        QStringList oldWords = entry->fields(genres, false);
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
        entry->setField(genres, words.toList().join(QLatin1String("; ")));
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
  QRegExp blank(QLatin1String("[\\s:;]+")); // only white space, column separators and row separators
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
//  myDebug() << "AmazonFetcher::fetchEntry() - grabbing " << imageURL.prettyUrl() << endl;
  if(!imageURL.isEmpty()) {
    QString id = ImageFactory::addImage(imageURL, true);
    // FIXME: need to add cover image field to bibtex collection
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
    kWarning() << "AmazonFetcher::initXSLTHandler() - can not locate amazon2tellico.xsl.";
    return;
  }

  KUrl u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kWarning() << "AmazonFetcher::initXSLTHandler() - error in amazon2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void AmazonFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
//  myDebug() << "AmazonFetcher::updateEntry()" << endl;

  int type = entry_->collection()->type();
  if(type == Data::Collection::Book || type == Data::Collection::ComicBook || type == Data::Collection::Bibtex) {
    QString isbn = entry_->field(QLatin1String("isbn"));
    if(!isbn.isEmpty()) {
      m_limit = 5; // no need for more
      search(Fetch::ISBN, isbn);
      return;
    }
  } else if(type == Data::Collection::Album) {
    QString a = entry_->field(QLatin1String("artist"));
    if(!a.isEmpty()) {
      search(Fetch::Person, a);
      return;
    }
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QLatin1String("title"));
  if(!t.isEmpty()) {
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "AmazonFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
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
  // if res = true, then the token gets removed from the title
  bool res = false;
  if(token.indexOf(QLatin1String("widescreen"), 0, Qt::CaseInsensitive) > -1 ||
     token.indexOf(i18n("Widescreen"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("widescreen"), QLatin1String("true"));
    // res = true; leave it in the title
  } else if(token.indexOf(QLatin1String("full screen"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  }
  if(token.indexOf(QLatin1String("blu-ray"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("medium"), i18n("Blu-ray"));
    res = true;
  } else if(token.indexOf(QLatin1String("hd dvd"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("medium"), i18n("HD DVD"));
    res = true;
  }
  if(token.indexOf(QLatin1String("director's cut"), 0, Qt::CaseInsensitive) > -1 ||
     token.indexOf(i18n("Director's Cut"), 0, Qt::CaseInsensitive) > -1) {
    entry->setField(QLatin1String("directors-cut"), QLatin1String("true"));
    // res = true; leave it in the title
  }
  return res;
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
  QLabel* label = new QLabel(i18n("Co&untry: "), optionsWidget());
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
  QString w = i18n("Amazon.com provides data from several different localized sites. Choose the one "
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
    m_assocEdit->setText(fetcher_->m_assoc);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_assocEdit->setText(QLatin1String(AMAZON_ASSOC_TOKEN));
    m_imageCombo->setCurrentData(MediumImage);
  }

  addFieldsWidget(AmazonFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  KAcceleratorManager::manage(optionsWidget());
}

void AmazonFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  int n = m_siteCombo->currentData().toInt();
  config_.writeEntry("Site", n);
  QString s = m_assocEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("AssocToken", s);
  }
  n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);

  saveFieldsConfig(config_);
  slotSetModified(false);
}

QString AmazonFetcher::ConfigWidget::preferredName() const {
  return AmazonFetcher::siteData(m_siteCombo->currentData().toInt()).title;
}

void AmazonFetcher::ConfigWidget::slotSiteChanged() {
  emit signalName(preferredName());
}

//static
Tellico::StringMap AmazonFetcher::customFields() {
  StringMap map;
  map[QLatin1String("keyword")] = i18n("Keywords");
  return map;
}

#include "amazonfetcher.moc"
