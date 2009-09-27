/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
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
#include "amazonrequest.h"
#include "messagehandler.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../imagefactory.h"
#include "../tellico_kernel.h"
#include "../latin1literal.h"
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
#include <kstandarddirs.h>
#include <kconfig.h>
#include <klineedit.h>
#include <kseparator.h>
#include <kcombobox.h>
#include <kaccelmanager.h>

#include <qdom.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qfile.h>
#include <qtextcodec.h>

namespace {
  static const int AMAZON_RETURNS_PER_REQUEST = 10;
  static const int AMAZON_MAX_RETURNS_TOTAL = 20;
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
      "http://webservices.amazon.com/onca/xml"
    }, {
      i18n("Amazon (UK)"),
      "http://webservices.amazon.co.uk/onca/xml"
    }, {
      i18n("Amazon (Germany)"),
      "http://webservices.amazon.de/onca/xml"
    }, {
      i18n("Amazon (Japan)"),
      "http://webservices.amazon.co.jp/onca/xml"
    }, {
      i18n("Amazon (France)"),
      "http://webservices.amazon.fr/onca/xml"
    }, {
      i18n("Amazon (Canada)"),
      "http://webservices.amazon.ca/onca/xml"
    }
  };

  return dataVector[site_];
}

AmazonFetcher::AmazonFetcher(Site site_, QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_xsltHandler(0), m_site(site_), m_imageSize(MediumImage),
      m_assoc(QString::fromLatin1(AMAZON_ASSOC_TOKEN)), m_addLinkField(true), m_limit(AMAZON_MAX_RETURNS_TOTAL),
      m_countOffset(0), m_page(1), m_total(-1), m_numResults(0), m_job(0), m_started(false) {
  m_name = siteData(site_).title;
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
  s = config_.readEntry("SecretKey");
  if(!s.isEmpty()) {
    m_amazonKey = s;
  }
  s = config_.readEntry("AssocToken");
  if(!s.isEmpty()) {
    m_assoc = s;
  }
  int imageSize = config_.readNumEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
  m_fields = config_.readListEntry("Custom Fields", QString::fromLatin1("keyword"));
}

void AmazonFetcher::search(FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_.stripWhiteSpace();
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
  m_data.truncate(0);

//  myDebug() << "AmazonFetcher::doSearch() - value = " << m_value << endl;
//  myDebug() << "AmazonFetcher::doSearch() - getting page " << m_page << endl;

  const SiteData& data = siteData(m_site);

  QMap<QString, QString> params;
  params.insert(QString::fromLatin1("Service"),        QString::fromLatin1("AWSECommerceService"));
  params.insert(QString::fromLatin1("AssociateTag"),   m_assoc);
  params.insert(QString::fromLatin1("AWSAccessKeyId"), m_access);
  params.insert(QString::fromLatin1("Operation"),      QString::fromLatin1("ItemSearch"));
  params.insert(QString::fromLatin1("ResponseGroup"),  QString::fromLatin1("Large"));
  params.insert(QString::fromLatin1("ItemPage"),       QString::number(m_page));
  params.insert(QString::fromLatin1("Version"),        QString::fromLatin1("2007-10-29"));

  const int type = Kernel::self()->collectionType();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      params.insert(QString::fromLatin1("SearchIndex"), QString::fromLatin1("Books"));
      params.insert(QString::fromLatin1("SortIndex"), QString::fromLatin1("relevancerank"));
      break;

    case Data::Collection::Album:
      params.insert(QString::fromLatin1("SearchIndex"), QString::fromLatin1("Music"));
      break;

    case Data::Collection::Video:
      params.insert(QString::fromLatin1("SearchIndex"), QString::fromLatin1("Video"));
      params.insert(QString::fromLatin1("SortIndex"), QString::fromLatin1("relevancerank"));
      break;

    case Data::Collection::Game:
      params.insert(QString::fromLatin1("SearchIndex"), QString::fromLatin1("VideoGames"));
      break;

    case Data::Collection::BoardGame:
      params.insert(QString::fromLatin1("SearchIndex"), QString::fromLatin1("Toys"));
      params.insert(QString::fromLatin1("SortIndex"), QString::fromLatin1("relevancerank"));
      break;

    case Data::Collection::Coin:
    case Data::Collection::Stamp:
    case Data::Collection::Wine:
    case Data::Collection::Base:
    case Data::Collection::Card:
      message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
      stop();
      return;
  }

  // I have not been able to find any documentation about what character set to use
  // when URL encoding the search term in the Amazon REST interface. But I do know
  // that utf8 DOES NOT WORK. So I'm arbitrarily using iso-8859-1, except for JP.
  // Why different for JP? Well, I've not received any bug reports from that direction yet

//  QString value = KURL::decode_string(value_, 106);
//  QString value = QString::fromLocal8Bit(value_.utf8());
  QString value = m_value;
  // a mibenum of 106 is utf-8, 4 is iso-8859-1, 0 means use user's locale,
  int mib = m_site == AmazonFetcher::JP ? 106 : 4;

  switch(m_key) {
    case Title:
      params.insert(QString::fromLatin1("Title"), value);
      break;

    case Person:
      if(type == Data::Collection::Video) {
        params.insert(QString::fromLatin1("Actor"),        value);
        params.insert(QString::fromLatin1("Director"),     value);
      } else if(type == Data::Collection::Album) {
        params.insert(QString::fromLatin1("Artist"),       value);
      } else if(type == Data::Collection::Game) {
        params.insert(QString::fromLatin1("Manufacturer"), value);
      } else { // books and bibtex
        QString s = QString::fromLatin1("author:%1 or publisher:%2").arg(value, value);
//        params.insert(QString::fromLatin1("Author"),       value, mib);
//        params.insert(QString::fromLatin1("Publisher"),    value, mib);
        params.insert(QString::fromLatin1("Power"),    s);
      }
      break;

    case ISBN:
      {
        params.insert(QString::fromLatin1("Operation"), QString::fromLatin1("ItemLookup"));

        QString s = m_value; // not encValue!!!
        s.remove('-');
        // ISBN only get digits or 'X', and multiple values are connected with "; "
        QStringList isbns = QStringList::split(QString::fromLatin1("; "), s);
        // Amazon isbn13 search is still very flaky, so if possible, we're going to convert
        // all of them to isbn10. If we run into a 979 isbn13, then we're forced to do an
        // isbn13 search
        bool isbn13 = false;
        for(QStringList::Iterator it = isbns.begin(); it != isbns.end(); ) {
          if(m_value.startsWith(QString::fromLatin1("979"))) {
            if(m_site == JP) { // never works for JP
              kdWarning() << "AmazonFetcher:doSearch() - ISBN-13 searching not implemented for Japan" << endl;
              isbns.remove(it); // automatically skips to next
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
              (*it).remove('-');
            }
          }
          // the default search is by ASIN, which prohibits SearchIndex
          params.remove(QString::fromLatin1("SearchIndex"));
        }
        // limit to first 10
        while(isbns.size() > 10) {
          isbns.pop_back();
        }
        params.insert(QString::fromLatin1("ItemId"), isbns.join(QString::fromLatin1(",")));
        if(isbn13) {
          params.insert(QString::fromLatin1("IdType"), QString::fromLatin1("EAN"));
        }
      }
      break;

    case UPC:
      {
        params.insert(QString::fromLatin1("Operation"), QString::fromLatin1("ItemLookup"));
        // US allows UPC, all others are EAN
        if(m_site == US) {
          params.insert(QString::fromLatin1("IdType"), QString::fromLatin1("UPC"));
        } else {
          params.insert(QString::fromLatin1("IdType"), QString::fromLatin1("EAN"));
        }
        QString s = m_value; // not encValue!!!
        s.remove('-');
        // limit to first 10
        s.replace(QString::fromLatin1("; "), QString::fromLatin1(","));
        s = s.section(',', 0, 9);
        params.insert(QString::fromLatin1("ItemId"), s);
      }
      break;

    case Keyword:
      params.insert(QString::fromLatin1("Keywords"), value);
      break;

    case Raw:
      {
        QString key = value.section('=', 0, 0).stripWhiteSpace();
        QString str = value.section('=', 1).stripWhiteSpace();
        params.insert(key, str);
      }
      break;

    default:
      kdWarning() << "AmazonFetcher::search() - key not recognized: " << m_key << endl;
      stop();
      return;
  }

  AmazonRequest request(siteData(m_site).url, m_amazonKey);
  KURL newUrl = request.signedRequest(params);
//  myDebug() << "AmazonFetcher::search() - url: " << newUrl.url() << endl;

  m_job = KIO::get(newUrl, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
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
  m_data.truncate(0);
  m_started = false;
  emit signalDone(this);
}

void AmazonFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void AmazonFetcher::slotComplete(KIO::Job* job_) {
//  myDebug() << "AmazonFetcher::slotComplete()" << endl;

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "AmazonFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
  kdWarning() << "Remove debug from amazonfetcher.cpp" << endl;
  QFile f(QString::fromLatin1("/tmp/test%1.xml").arg(m_page));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << QCString(m_data, m_data.size()+1);
  }
  f.close();
#endif

  QStringList errors;
  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(m_data, false)) {
      kdWarning() << "AmazonFetcher::slotComplete() - server did not return valid XML." << endl;
      stop();
      return;
    }
    // find TotalResults element
    // it's in the first level under the root element
    //ItemSearchResponse/Items/TotalResults
    QDomNode n = dom.documentElement().namedItem(QString::fromLatin1("Items"))
                                      .namedItem(QString::fromLatin1("TotalResults"));
    QDomElement e = n.toElement();
    if(!e.isNull()) {
      m_total = e.text().toInt();
    }
    n = dom.documentElement().namedItem(QString::fromLatin1("Items"))
                             .namedItem(QString::fromLatin1("Request"))
                             .namedItem(QString::fromLatin1("Errors"));
    e = n.toElement();
    if(!e.isNull()) {
      QDomNodeList nodes = e.elementsByTagName(QString::fromLatin1("Error"));
      for(uint i = 0; i < nodes.count(); ++i) {
        e = nodes.item(i).toElement().namedItem(QString::fromLatin1("Code")).toElement();
        if(!e.isNull() && e.text() == Latin1Literal("AWS.ECommerceService.NoExactMatches")) {
          // no exact match, not a real error, so skip
          continue;
        }
        // for some reason, Amazon will return an error simply when a valid ISBN is not found
        // I really want to ignore that, so check the IsValid element in the Request element
        QDomNode isValidNode = n.parentNode().namedItem(QString::fromLatin1("IsValid"));
        if(m_key == ISBN && isValidNode.toElement().text().lower() == Latin1Literal("true")) {
          continue;
        }
        e = nodes.item(i).toElement().namedItem(QString::fromLatin1("Message")).toElement();
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

//  QRegExp stripHTML(QString::fromLatin1("<.*>"), true);
//  stripHTML.setMinimal(true);

  // assume amazon is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(m_data, m_data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "AmazonFetcher::slotComplete() - no collection pointer" << endl;
    stop();
    return;
  }

  if(!m_addLinkField) {
    // remove amazon field if it's not to be added
    coll->removeField(QString::fromLatin1("amazon"));
  }

  Data::EntryVec entries = coll->entries();
  if(entries.isEmpty() && !errors.isEmpty()) {
    for(QStringList::ConstIterator it = errors.constBegin(); it != errors.constEnd(); ++it) {
      myDebug() << "AmazonFetcher::" << *it << endl;
    }
    message(errors[0], MessageHandler::Error);
    stop();
    return;
  }

  int count = 0;
  for(Data::EntryVec::Iterator entry = entries.begin();
      m_numResults < m_limit && entry != entries.end();
      ++entry, ++count) {
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
      QRegExp rx(QString::fromLatin1("\\.([^\\s])"));
      QStringList values = entry->fields(QString::fromLatin1("author"), false);
      for(QStringList::Iterator it = values.begin(); it != values.end(); ++it) {
        (*it).replace(rx, QString::fromLatin1(". \\1"));
      }
      entry->setField(QString::fromLatin1("author"), values.join(QString::fromLatin1("; ")));
    }

    // UK puts the year in the title for some reason
    if(m_site == UK && coll->type() == Data::Collection::Video) {
      QRegExp rx(QString::fromLatin1("\\[(\\d{4})\\]"));
      QString t = entry->title();
      if(t.find(rx) > -1) {
        QString y = rx.cap(1);
        t.remove(rx).simplifyWhiteSpace();
        entry->setField(QString::fromLatin1("title"), t);
        if(entry->field(QString::fromLatin1("year")).isEmpty()) {
          entry->setField(QString::fromLatin1("year"), y);
        }
      }
    }

    QString desc;
    switch(coll->type()) {
      case Data::Collection::Book:
      case Data::Collection::ComicBook:
      case Data::Collection::Bibtex:
        desc = entry->field(QString::fromLatin1("author"))
               + QChar('/') + entry->field(QString::fromLatin1("publisher"));
        if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
          desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
        } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
          desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
        }
        break;

      case Data::Collection::Video:
        desc = entry->field(QString::fromLatin1("studio"))
               + QChar('/')
               + entry->field(QString::fromLatin1("director"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"))
               + QChar('/')
               + entry->field(QString::fromLatin1("medium"));
        break;

      case Data::Collection::Album:
        desc = entry->field(QString::fromLatin1("artist"))
               + QChar('/')
               + entry->field(QString::fromLatin1("label"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"));
        break;

      case Data::Collection::Game:
        desc = entry->field(QString::fromLatin1("platform"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"));
        break;

      case Data::Collection::BoardGame:
        desc = entry->field(QString::fromLatin1("publisher"))
               + QChar('/')
               + entry->field(QString::fromLatin1("year"));
        break;

      default:
        break;
    }

    // strip HTML from comments, or plot in movies
    // tentatively don't do this, looks like ECS 4 cleaned everything up
/*
    if(coll->type() == Data::Collection::Video) {
      QString plot = entry->field(QString::fromLatin1("plot"));
      plot.remove(stripHTML);
      entry->setField(QString::fromLatin1("plot"), plot);
    } else if(coll->type() == Data::Collection::Game) {
      QString desc = entry->field(QString::fromLatin1("description"));
      desc.remove(stripHTML);
      entry->setField(QString::fromLatin1("description"), desc);
    } else {
      QString comments = entry->field(QString::fromLatin1("comments"));
      comments.remove(stripHTML);
      entry->setField(QString::fromLatin1("comments"), comments);
    }
*/
//    myDebug() << "AmazonFetcher::slotComplete() - " << entry->title() << endl;
    SearchResult* r = new SearchResult(this, entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
    ++m_numResults;
  }

  // we might have gotten aborted
  if(!m_started) {
    return;
  }

  // are there any additional results to get?
  m_hasMoreResults = m_page * AMAZON_RETURNS_PER_REQUEST < m_total;

  const int currentTotal = QMIN(m_total, m_limit);
  if(m_page * AMAZON_RETURNS_PER_REQUEST < currentTotal) {
    int foundCount = (m_page-1) * AMAZON_RETURNS_PER_REQUEST + coll->entryCount();
    message(i18n("Results from %1: %2/%3").arg(source()).arg(foundCount).arg(m_total), MessageHandler::Status);
    ++m_page;
    m_countOffset = 0;
    doSearch();
  } else if(m_value.contains(';') > 9) {
    search(m_key, m_value.section(';', 10));
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
    kdWarning() << "AmazonFetcher::fetchEntry() - no entry in dict" << endl;
    return 0;
  }

  QStringList defaultFields = customFields().keys();
  for(QStringList::Iterator it = defaultFields.begin(); it != defaultFields.end(); ++it) {
    if(!m_fields.contains(*it)) {
      entry->setField(*it, QString::null);
    }
  }

  // do what we can to remove useless keywords
  const int type = Kernel::self()->collectionType();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::ComicBook:
    case Data::Collection::Bibtex:
      {
        const QString keywords = QString::fromLatin1("keyword");
        QStringList oldWords = entry->fields(keywords, false);
        StringSet words;
        for(QStringList::Iterator it = oldWords.begin(); it != oldWords.end(); ++it) {
          // the amazon2tellico stylesheet separates keywords with '/'
          QStringList nodes = QStringList::split('/', *it);
          for(QStringList::Iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            if(*it2 == Latin1Literal("General") ||
               *it2 == Latin1Literal("Subjects") ||
               *it2 == Latin1Literal("Par prix") || // french stuff
               *it2 == Latin1Literal("Divers") || // french stuff
               (*it2).startsWith(QChar('(')) ||
               (*it2).startsWith(QString::fromLatin1("Authors"))) {
              continue;
            }
            words.add(*it2);
          }
        }
        entry->setField(keywords, words.toList().join(QString::fromLatin1("; ")));
      }
      entry->setField(QString::fromLatin1("comments"), Tellico::decodeHTML(entry->field(QString::fromLatin1("comments"))));
      break;

    case Data::Collection::Video:
      {
        const QString genres = QString::fromLatin1("genre");
        QStringList oldWords = entry->fields(genres, false);
        StringSet words;
        // only care about genres that have "Genres" in the amazon response
        // and take the first word after that
        for(QStringList::Iterator it = oldWords.begin(); it != oldWords.end(); ++it) {
          if((*it).find(QString::fromLatin1("Genres")) == -1) {
            continue;
          }

          // the amazon2tellico stylesheet separates words with '/'
          QStringList nodes = QStringList::split('/', *it);
          for(QStringList::Iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            if(*it2 != Latin1Literal("Genres")) {
              continue;
            }
            ++it2;
            if(it2 != nodes.end() && *it2 != Latin1Literal("General")) {
              words.add(*it2);
            }
            break; // we're done
          }
        }
        entry->setField(genres, words.toList().join(QString::fromLatin1("; ")));
        // language tracks get duplicated, too
        QStringList langs = entry->fields(QString::fromLatin1("language"), false);
        words.clear();
        for(QStringList::ConstIterator it = langs.begin(); it != langs.end(); ++it) {
          words.add(*it);
        }
        entry->setField(QString::fromLatin1("language"), words.toList().join(QString::fromLatin1("; ")));
      }
      entry->setField(QString::fromLatin1("plot"), Tellico::decodeHTML(entry->field(QString::fromLatin1("plot"))));
      break;

    case Data::Collection::Album:
      {
        const QString genres = QString::fromLatin1("genre");
        QStringList oldWords = entry->fields(genres, false);
        StringSet words;
        // only care about genres that have "Styles" in the amazon response
        // and take the first word after that
        for(QStringList::Iterator it = oldWords.begin(); it != oldWords.end(); ++it) {
          if((*it).find(QString::fromLatin1("Styles")) == -1) {
            continue;
          }

          // the amazon2tellico stylesheet separates words with '/'
          QStringList nodes = QStringList::split('/', *it);
          bool isStyle = false;
          for(QStringList::Iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
            if(!isStyle) {
              if(*it2 == Latin1Literal("Styles")) {
                isStyle = true;
              }
              continue;
            }
            if(*it2 != Latin1Literal("General")) {
              words.add(*it2);
            }
          }
        }
        entry->setField(genres, words.toList().join(QString::fromLatin1("; ")));
      }
      entry->setField(QString::fromLatin1("comments"), Tellico::decodeHTML(entry->field(QString::fromLatin1("comments"))));
      break;

    case Data::Collection::Game:
      entry->setField(QString::fromLatin1("description"), Tellico::decodeHTML(entry->field(QString::fromLatin1("description"))));
      break;
  }

  // clean up the title
  parseTitle(entry, type);

  // also sometimes table fields have rows but no values
  Data::FieldVec fields = entry->collection()->fields();
  QRegExp blank(QString::fromLatin1("[\\s:;]+")); // only white space, column separators and row separators
  for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
    if(fIt->type() != Data::Field::Table) {
      continue;
    }
    if(blank.exactMatch(entry->field(fIt))) {
      entry->setField(fIt, QString::null);
    }
  }

  KURL imageURL;
  switch(m_imageSize) {
    case SmallImage:
      imageURL = entry->field(QString::fromLatin1("small-image"));
      break;
    case MediumImage:
      imageURL = entry->field(QString::fromLatin1("medium-image"));
      break;
    case LargeImage:
      imageURL = entry->field(QString::fromLatin1("large-image"));
      break;
    case NoImage:
    default:
      break;
  }
//  myDebug() << "AmazonFetcher::fetchEntry() - grabbing " << imageURL.prettyURL() << endl;
  if(!imageURL.isEmpty()) {
    QString id = ImageFactory::addImage(imageURL, true);
    // FIXME: need to add cover image field to bibtex collection
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    } else { // amazon serves up 1x1 gifs occasionally, but that's caught in the image constructor
      // all relevant collection types have cover fields
      entry->setField(QString::fromLatin1("cover"), id);
    }
  }

  // don't want to show image urls in the fetch dialog
  entry->setField(QString::fromLatin1("small-image"),  QString::null);
  entry->setField(QString::fromLatin1("medium-image"), QString::null);
  entry->setField(QString::fromLatin1("large-image"),  QString::null);
  return entry;
}

void AmazonFetcher::initXSLTHandler() {
  QString xsltfile = locate("appdata", QString::fromLatin1("amazon2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - can not locate amazon2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - error in amazon2tellico.xsl." << endl;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void AmazonFetcher::updateEntry(Data::EntryPtr entry_) {
//  myDebug() << "AmazonFetcher::updateEntry()" << endl;

  int type = entry_->collection()->type();
  if(type == Data::Collection::Book || type == Data::Collection::ComicBook || type == Data::Collection::Bibtex) {
    QString isbn = entry_->field(QString::fromLatin1("isbn"));
    if(!isbn.isEmpty()) {
      m_limit = 5; // no need for more
      search(Fetch::ISBN, isbn);
      return;
    }
  } else if(type == Data::Collection::Album) {
    QString a = entry_->field(QString::fromLatin1("artist"));
    if(!a.isEmpty()) {
      search(Fetch::Person, a);
      return;
    }
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QString::fromLatin1("title"));
  if(!t.isEmpty()) {
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "AmazonFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

void AmazonFetcher::parseTitle(Data::EntryPtr entry, int collType) {
  Q_UNUSED(collType);
  // assume that everything in brackets or parentheses is extra
  QRegExp rx(QString::fromLatin1("[\\(\\[](.*)[\\)\\]]"));
  rx.setMinimal(true);
  QString title = entry->field(QString::fromLatin1("title"));
  int pos = rx.search(title);
  while(pos > -1) {
    if(parseTitleToken(entry, rx.cap(1))) {
      title.remove(pos, rx.matchedLength());
      --pos; // search again there
    }
    pos = rx.search(title, pos+1);
  }
  entry->setField(QString::fromLatin1("title"), title.stripWhiteSpace());
}

bool AmazonFetcher::parseTitleToken(Data::EntryPtr entry, const QString& token) {
  // if res = true, then the token gets removed from the title
  bool res = false;
  if(token.find(QString::fromLatin1("widescreen"), 0, false /* case-insensitive*/) > -1 ||
     token.find(i18n("Widescreen"), 0, false) > -1) {
    entry->setField(QString::fromLatin1("widescreen"), QString::fromLatin1("true"));
    // res = true; leave it in the title
  } else if(token.find(QString::fromLatin1("full screen"), 0, false) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  }
  if(token.find(QString::fromLatin1("blu-ray"), 0, false) > -1) {
    entry->setField(QString::fromLatin1("medium"), i18n("Blu-ray"));
    res = true;
  } else if(token.find(QString::fromLatin1("hd dvd"), 0, false) > -1) {
    entry->setField(QString::fromLatin1("medium"), i18n("HD DVD"));
    res = true;
  }
  if(token.find(QString::fromLatin1("director's cut"), 0, false) > -1 ||
     token.find(i18n("Director's Cut"), 0, false) > -1) {
    entry->setField(QString::fromLatin1("directors-cut"), QString::fromLatin1("true"));
    // res = true; leave it in the title
  }
  return res;
}

Tellico::Fetch::ConfigWidget* AmazonFetcher::configWidget(QWidget* parent_) const {
  return new AmazonFetcher::ConfigWidget(parent_, this);
}

AmazonFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AmazonFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget(), 4, 2);
  l->setSpacing(4);
  l->setColStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("Co&untry: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_siteCombo = new GUI::ComboBox(optionsWidget());
  m_siteCombo->insertItem(i18n("United States"), US);
  m_siteCombo->insertItem(i18n("United Kingdom"), UK);
  m_siteCombo->insertItem(i18n("Germany"), DE);
  m_siteCombo->insertItem(i18n("Japan"), JP);
  m_siteCombo->insertItem(i18n("France"), FR);
  m_siteCombo->insertItem(i18n("Canada"), CA);
  connect(m_siteCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  connect(m_siteCombo, SIGNAL(activated(int)), SLOT(slotSiteChanged()));
  l->addWidget(m_siteCombo, row, 1);
  QString w = i18n("Amazon.com provides data from several different localized sites. Choose the one "
                   "you wish to use for this data source.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_siteCombo, w);
  label->setBuddy(m_siteCombo);

  label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->insertItem(i18n("Small Image"), SmallImage);
  m_imageCombo->insertItem(i18n("Medium Image"), MediumImage);
  m_imageCombo->insertItem(i18n("Large Image"), LargeImage);
  m_imageCombo->insertItem(i18n("No Image"), NoImage);
  connect(m_imageCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_imageCombo, row, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_imageCombo, w);
  label->setBuddy(m_imageCombo);

  label = new QLabel(i18n("&Associate's ID: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_assocEdit = new KLineEdit(optionsWidget());
  connect(m_assocEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_assocEdit, row, 1);
  w = i18n("The associate's id identifies the person accessing the Amazon.com Web Services, and is included "
           "in any links to the Amazon.com site.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_assocEdit, w);
  label->setBuddy(m_assocEdit);

  label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_accessEdit = new KLineEdit(optionsWidget());
  connect(m_accessEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_accessEdit, row, 1);

  label = new QLabel(i18n("Secret Key: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_secretKeyEdit = new KLineEdit(optionsWidget());
  connect(m_secretKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_secretKeyEdit, row, 1);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_siteCombo->setCurrentData(fetcher_->m_site);
    m_assocEdit->setText(fetcher_->m_assoc);
    m_accessEdit->setText(fetcher_->m_access);
    m_secretKeyEdit->setText(fetcher_->m_amazonKey);
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_assocEdit->setText(QString::fromLatin1(AMAZON_ASSOC_TOKEN));
    m_imageCombo->setCurrentData(MediumImage);
  }

  addFieldsWidget(AmazonFetcher::customFields(), fetcher_ ? fetcher_->m_fields : QStringList());

  KAcceleratorManager::manage(optionsWidget());
}

void AmazonFetcher::ConfigWidget::saveConfig(KConfigGroup& config_) {
  int n = m_siteCombo->currentData().toInt();
  config_.writeEntry("Site", n);
  QString s = m_assocEdit->text().stripWhiteSpace();
  if(!s.isEmpty()) {
    config_.writeEntry("AssocToken", s);
  }
  s = m_accessEdit->text().stripWhiteSpace();
  if(!s.isEmpty()) {
    config_.writeEntry("AccessKey", s);
  }
  s = m_secretKeyEdit->text().stripWhiteSpace();
  if(!s.isEmpty()) {
    config_.writeEntry("SecretKey", s);
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
  map[QString::fromLatin1("keyword")] = i18n("Keywords");
  return map;
}

#include "amazonfetcher.moc"
