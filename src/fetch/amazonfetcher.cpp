/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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
#include "../translators/xslthandler.h"
#include "../translators/bookcaseimporter.h"
#include "../imagefactory.h"
#include "../kernel.h"
#include "../latin1literal.h"

#include <klocale.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <kconfig.h>

#include <qfile.h>

static const int AMAZON_RETURNS_PER_REQUEST = 10;
static const int AMAZON_MAX_RETURNS_TOTAL = 30;
static const char* AMAZON_DEV_TOKEN = "D1AYHI5IKAIDPL";
static const char* AMAZON_ASSOC_TOKEN = "bookcase-20";

using Bookcase::Fetch::AmazonFetcher;

// static
inline
const AmazonFetcher::SiteData& AmazonFetcher::siteData(Site site_) {
  static SiteData dataVector[4] = {
    {
      i18n("Amazon (US)"),
      "http://xml.amazon.com/onca/xml3",
      QString::null,
      QString::fromLatin1("books"),
      QString::fromLatin1("dvd"),
      QString::fromLatin1("vhs"),
      QString::fromLatin1("music"),
      QString::fromLatin1("classical")
    }, {
      i18n("Amazon (UK)"),
      "http://xml-eu.amazon.com/onca/xml3",
      QString::fromLatin1("uk"),
      QString::fromLatin1("books-uk"),
      QString::fromLatin1("dvd-uk"),
      QString::fromLatin1("vhs-uk"),
      QString::fromLatin1("music"),
      QString::fromLatin1("classical")
    }, {
      i18n("Amazon (Germany)"),
      "http://xml-eu.amazon.com/onca/xml3",
      QString::fromLatin1("de"),
      QString::fromLatin1("books-de"),
      QString::fromLatin1("dvd-de"),
      QString::fromLatin1("vhs-de"),
      QString::fromLatin1("pop-music-de"),
      QString::fromLatin1("classical-de")
    }, {
      i18n("Amazon (Japan)"),
      "http://xml.amazon.co.jp/onca/xml3",
      QString::fromLatin1("jp"),
      QString::fromLatin1("books-jp"),
      QString::fromLatin1("dvd-jp"),
      QString::fromLatin1("vhs-jp"),
      QString::fromLatin1("music-jp"),
      QString::fromLatin1("classical-jp")
    }
  };

  return dataVector[site_];
}

AmazonFetcher::AmazonFetcher(Site site_, Data::Collection* coll_, QObject* parent_, const char* name_)
    : Fetcher(coll_, parent_, name_), m_site(site_), m_primaryMode(true),
      m_page(1), m_total(-1), m_job(0), m_xsltHandler(0), m_started(false) {
#ifndef NDEBUG
  if(!coll_) {
    kdWarning() << "AmazonFetcher::AmazonFetcher() - null collection pointer!" << endl;
  }
#endif
  m_results.setAutoDelete(true); // entries will be handled in destructor

  KConfig* config = KGlobal::config();
  KConfigGroupSaver group(config, "Amazon Web Services");
  m_token = config->readEntry(QString::fromLatin1("DevToken"), QString::fromLatin1(AMAZON_DEV_TOKEN));
  m_assoc = config->readEntry(QString::fromLatin1("AssocToken"), QString::fromLatin1(AMAZON_ASSOC_TOKEN));
  m_addLinkField = config->readBoolEntry("AddLinkField", true);
  m_imageSize = static_cast<ImageSize>(QMAX(0, QMIN(2, config->readNumEntry("ImageSize", 1))));
}

AmazonFetcher::~AmazonFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;

  cleanUp();
}

QString AmazonFetcher::source() const {
  return siteData(m_site).title;
}

void AmazonFetcher::cleanUp() {
  // need to delete collection pointers
  QPtrList<Data::Collection> collList;
  for(QIntDictIterator<Data::Entry> it(m_entries); it.current(); ++it) {
    if(collList.findRef(it.current()->collection()) == -1) {
      collList.append(it.current()->collection());
    }
  }
  collList.setAutoDelete(true); // will automatically delete all entries
}

void AmazonFetcher::search(FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_;
  m_started = true;

//  kdDebug() << "AmazonFetcher::search() - getting page " << m_page << endl;
  //FIXME: think about doing a lite call first, then heavy later for entries that get fetched
  const SiteData& data = siteData(m_site);
#if 1
  KURL u = data.url;
  u.addQueryItem(QString::fromLatin1("t"), m_assoc);
  u.addQueryItem(QString::fromLatin1("dev-t"), m_token);
  u.addQueryItem(QString::fromLatin1("type"), QString::fromLatin1("heavy"));
  u.addQueryItem(QString::fromLatin1("f"), QString::fromLatin1("xml"));
  u.addQueryItem(QString::fromLatin1("page"), QString::number(m_page));

  if(!data.locale.isEmpty()) {
    u.addQueryItem(QString::fromLatin1("locale"), data.locale);
  }

  Data::Collection::Type type = collection()->type();
  switch(type) {
    case Data::Collection::Book:
    case Data::Collection::Bibtex:
      u.addQueryItem(QString::fromLatin1("mode"), data.books);
      break;

    case Data::Collection::Album:
      if(m_primaryMode) {
        u.addQueryItem(QString::fromLatin1("mode"), data.music);
      } else {
        u.addQueryItem(QString::fromLatin1("mode"), data.classical);
      }
      break;

    case Data::Collection::Video:
      if(m_primaryMode) {
        u.addQueryItem(QString::fromLatin1("mode"), data.dvd);
      } else {
        u.addQueryItem(QString::fromLatin1("mode"), data.vhs);
      }
      break;

    case Data::Collection::Coin:
    case Data::Collection::Stamp:
    case Data::Collection::ComicBook:
    case Data::Collection::Wine:
    case Data::Collection::Base:
    case Data::Collection::Card:
    default:
      emit signalStatus(i18n("%1 does not allow searching for this collection type.").arg(source()));
      stop();
      return;
  }

  switch(key_) {
    case Title:
      // power search is only valid for books
      if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
        u.addQueryItem(QString::fromLatin1("PowerSearch"), QString::fromLatin1("title: ") + value_);
      } else {
        u.addQueryItem(QString::fromLatin1("KeywordSearch"), value_);
      }
      break;

    case Person:
      if(type == Data::Collection::Video) {
        u.addQueryItem(QString::fromLatin1("ActorSearch"), value_);
        u.addQueryItem(QString::fromLatin1("DirectorSearch"), value_);
      } else if(type == Data::Collection::Album) {
        u.addQueryItem(QString::fromLatin1("ArtistSearch"), value_);
      } else if(type== Data::Collection::Book) {
        u.addQueryItem(QString::fromLatin1("AuthorSearch"), value_);
      }
      break;

    case ISBN:
      {
        // ISBN only get digits or 'X', and multiple values are connected with "; "
        if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
          QString str = value_;
          str.replace(QString::fromLatin1("; "), QString::fromLatin1(","));
          static const QRegExp badChars(QString::fromLatin1("[^\\dX,]"));
          str.replace(badChars, QString::null);
#ifndef NDEBUG
          if(str.contains(',') > 10) {
            kdWarning() << "AmazonFetcher::search() - multiple ISBN search limited to 10 queries." << endl;
            str = str.section(',', 0, 9);
          }
#endif
          u.addQueryItem(QString::fromLatin1("AsinSearch"), str);
        } else {
          u.addQueryItem(QString::fromLatin1("UpcSearch"), value_);
        }
      }
      break;

    case Keyword:
      u.addQueryItem(QString::fromLatin1("KeywordSearch"), value_);
      break;

    case Raw:
      {
        QString key = value_.section('=', 0, 0).stripWhiteSpace();
        QString str = value_.section('=', 1).stripWhiteSpace();
        u.addQueryItem(key, str);
      }
      break;

    default:
      kdWarning() << "AmazonFetcher::search() - key not recognized: " << key_ << endl;
      stop();
      return;
  }

//  kdDebug() << u.prettyURL() << endl;
  m_job = KIO::get(u, false, false);
#else
  m_job = KIO::get(KURL::fromPathOrURL(QString::fromLatin1("/home/robby/delpy-heavy.xml")), false, false);
#endif
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void AmazonFetcher::stop() {
  if(!m_started) {
    return;
  }
//  kdDebug() << "AmazonFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_page = 1;
  m_total = -1;
  m_primaryMode = true;
  m_data.truncate(0);
  emit signalDone();
  m_started = false;
}

void AmazonFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  uint oldSize = m_data.size();
  uint addSize = data_.size();
  m_data.resize(oldSize + addSize);

  for(uint i = 0; i < addSize; ++i) {
    m_data[oldSize + i] = data_[i];
  }
}

void AmazonFetcher::slotComplete(KIO::Job* job_) {
//  kdDebug() << "AmazonFetcher::slotComplete()" << endl;

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    stop();
    return;
  }

#if 0
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t << QCString(m_data, m_data.size()+1);
  }
  f.close();
#endif

  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(m_data, false)) {
      kdWarning() << "AmazonFetcher::slotComplete() - server did not return valid XML." << endl;
      stop();
    }
    // find TotalResults element
    // it's in the first level under the root element
    for(QDomNode n = dom.documentElement().firstChild(); !n.isNull(); n = n.nextSibling()) {
      QDomElement e = n.toElement();
      if(e.isNull()) {
        continue;
      }
      if(e.tagName() == Latin1Literal("TotalResults")) {
        m_total = e.text().toInt();
        break;
      }
    }
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
    }
  }

  // assume amazon is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QCString(m_data, m_data.size()+1));
  Import::BookcaseImporter imp(str);
  Data::Collection* coll = imp.collection();
  for(Data::EntryListIterator it(coll->entryList()); it.current(); ++it) {
    // special case for non-books mode and search for title
    if(m_key == Title && (coll->type() != Data::Collection::Book)) {
      // if the title doesn't have the search value, skip it
      if(it.current()->field(QString::fromLatin1("title")).find(m_value, 0, false) == -1) {
        continue;
      }
    }
    QString desc;
    switch(coll->type()) {
      case Data::Collection::Book:
        desc = it.current()->field(QString::fromLatin1("author"))
               + QChar('/')
               + it.current()->field(QString::fromLatin1("publisher"));
        if(!it.current()->field(QString::fromLatin1("cr_year")).isEmpty()) {
          desc += QChar('/') + it.current()->field(QString::fromLatin1("cr_year"));
        } else if(!it.current()->field(QString::fromLatin1("pub_year")).isEmpty()){
          desc += QChar('/') + it.current()->field(QString::fromLatin1("pub_year"));
        }
        break;

      case Data::Collection::Video:
        desc = it.current()->field(QString::fromLatin1("studio"))
               + QChar('/')
               + it.current()->field(QString::fromLatin1("director"))
               + QChar('/')
               + it.current()->field(QString::fromLatin1("year"))
               + QChar('/')
               + it.current()->field(QString::fromLatin1("medium"));
        break;

      case Data::Collection::Album:
        desc = it.current()->field(QString::fromLatin1("artist"))
               + QChar('/')
               + it.current()->field(QString::fromLatin1("label"))
               + QChar('/')
               + it.current()->field(QString::fromLatin1("year"));
        break;

      default:
        break;
    }
    SearchResult* r = new SearchResult(this, it.current()->title(), desc);
    m_results.insert(r->uid, r);
    m_entries.insert(r->uid, it.current());
    emit signalResultFound(*r);
  }

  if(m_page * AMAZON_RETURNS_PER_REQUEST < QMIN(m_total, AMAZON_MAX_RETURNS_TOTAL)) {
    int foundCount = (m_page-1) * AMAZON_RETURNS_PER_REQUEST + coll->entryCount();
    emit signalStatus(i18n("Results from %1: %2/%3").arg(source()).arg(foundCount).arg(m_total));

    ++m_page;
    m_data.truncate(0);
    search(m_key, m_value);
  } else if(m_primaryMode == true
            && (coll->type() == Data::Collection::Album || coll->type() == Data::Collection::Video)) {
    // repeating search with secondary mode
    m_page = 1;
    m_total = -1;
    m_data.truncate(0);
    m_primaryMode = false;
    search(m_key, m_value);
  } else {
    stop();
  }
}

Bookcase::Data::Entry* AmazonFetcher::fetchEntry(uint uid_) {
  static QRegExp stripHTML;
  if(stripHTML.isEmpty()) {
    stripHTML.setMinimal(true);
    stripHTML.setPattern(QString::fromLatin1("<.*>"));
  }

//  kdDebug() << "AmazonFetcher::fetchEntry() - looking for " << m_results[uid_]->desc << endl;
  Data::Entry* entry = m_entries[uid_];

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
  }
//  kdDebug() << "AmazonFetcher::fetchEntry() - grabbing " << imageURL.prettyURL() << endl;
  const Data::Image& img = ImageFactory::addImage(imageURL, true);
  // FIXME: need to add cover image field to bibtex collection
  if(img.isNull()) {
    // riich text causes layout issues
//    emit signalStatus(i18n("<qt>The cover image for <i>%1</i> could not be loaded.</qt>").arg(
//                            entry->field(QString::fromLatin1("title"))));
    emit signalStatus(i18n("The cover image could not be loaded."));
  } else if(img.width() > 1 and img.height() > 1) { // amazon serves up 1x1 gifs occasionally
    // all relevant collection types have cover fields
    entry->setField(QString::fromLatin1("cover"), img.id());
  }

  // strip HTML from comments, or plot in movies
  if(entry->collection()->type() == Data::Collection::Video) {
    QString plot = entry->field(QString::fromLatin1("plot"));
    plot.remove(stripHTML);
    entry->setField(QString::fromLatin1("plot"), plot);
  } else {
    QString comments = entry->field(QString::fromLatin1("comments"));
    comments.remove(stripHTML);
    entry->setField(QString::fromLatin1("comments"), comments);
  }

  // add new amazon link field
  if(m_addLinkField) {
    collection()->mergeField(collection()->fieldByName(QString::fromLatin1("amazon")));
  }
  return new Data::Entry(*entry, collection());
}

void AmazonFetcher::initXSLTHandler() {
  QString xsltfile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("amazon2bookcase.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - can not locate amazon2bookcase.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - error in amazon2bookcase.xsl." << endl;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

#include "amazonfetcher.moc"
