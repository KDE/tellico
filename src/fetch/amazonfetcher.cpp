/***************************************************************************
    copyright            : (C) 2004 by Robby Stephenson
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
#include "../translators/tellicoimporter.h"
#include "../imagefactory.h"
#include "../tellico_kernel.h"
#include "../latin1literal.h"

#include <klocale.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <kconfig.h>
#include <klineedit.h>
#include <kseparator.h>
#include <kcombobox.h>
#include <kmessagebox.h>

//#include <qfile.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qwhatsthis.h>
#include <qcheckbox.h>
#include <qfile.h>

namespace {
  static const int AMAZON_RETURNS_PER_REQUEST = 10;
  static const int AMAZON_MAX_RETURNS_TOTAL = 30;
  static const char* AMAZON_DEV_TOKEN = "D1AYHI5IKAIDPL";
  static const char* AMAZON_ASSOC_TOKEN = "tellico-20";
}

using Tellico::Fetch::AmazonFetcher;
Tellico::XSLTHandler* AmazonFetcher::s_xsltHandler = 0;

// static
const AmazonFetcher::SiteData& AmazonFetcher::siteData(Site site_) {
  static SiteData dataVector[6] = {
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
    }, {
      i18n("Amazon (France)"),
      "http://xml.amazon.fr/onca/xml3",
      QString::fromLatin1("fr"),
      QString::fromLatin1("books-fr"),
      QString::fromLatin1("dvd-fr"),
      QString::fromLatin1("vhs-fr"),
      QString::fromLatin1("music-fr"),
      QString::fromLatin1("classical-fr")
    }, {
      i18n("Amazon (Canada)"),
      "http://xml.amazon.ca/onca/xml3",
      QString::fromLatin1("ca"),
      QString::fromLatin1("books-ca"),
      QString::fromLatin1("dvd-ca"),
      QString::fromLatin1("vhs-ca"),
      QString::fromLatin1("music-ca"),
      QString::fromLatin1("classical-ca")
    }
  };

  return dataVector[site_];
}

AmazonFetcher::AmazonFetcher(Site site_, QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_site(site_), m_primaryMode(true), m_imageSize(NoImage),
      m_name(siteData(site_).title), m_token(QString::fromLatin1(AMAZON_DEV_TOKEN)),
      m_assoc(QString::fromLatin1(AMAZON_ASSOC_TOKEN)), m_addLinkField(true),
      m_page(1), m_total(-1), m_job(0), m_started(false) {
  m_results.setAutoDelete(true); // entries will be handled in destructor
}

AmazonFetcher::~AmazonFetcher() {
  cleanUp();
}

QString AmazonFetcher::source() const {
  return m_name;
}

void AmazonFetcher::readConfig(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Name");
  if(!s.isEmpty()) {
    m_name = s;
  }
  s = config_->readEntry("DevToken");
  if(!s.isEmpty()) {
    m_token = s;
  }
  s = config_->readEntry("AssocToken");
  if(!s.isEmpty()) {
    m_assoc = s;
  }
  int imageSize = config_->readNumEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
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

void AmazonFetcher::search(FetchKey key_, const QString& value_, bool multiple_) {
  m_key = key_;
  m_value = value_;
  m_multiple = multiple_;
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

  Data::Collection::Type type = Kernel::self()->collection()->type();
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
          // keep a list of isbn value we're searching for
          // but only set it when it's not set before
          if(m_isbnList.isEmpty()) {
            m_isbnList = QStringList::split(QString::fromLatin1("; "), m_value);
          }
          // static const QRegExp& badChars = Kernel::staticQRegExp("[^\\dX,]");
          // str.replace(badChars, QString::null);
          // assume the FetchManager has already validated the string
          QString s = value_;
          s.remove('-');
          if(!m_multiple) {
            u.addQueryItem(QString::fromLatin1("AsinSearch"), s);
          } else {
            // limit to first 10
            s.replace(QString::fromLatin1("; "), QString::fromLatin1(","));
            s = s.section(',', 0, 9);
            u.addQueryItem(QString::fromLatin1("AsinSearch"), s);
          }
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

  m_job = KIO::get(u, false, false);
#else
  m_job = KIO::get(KURL::fromPathOrURL(QString::fromLatin1("/tmp/test.xml")), false, false);
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
  m_isbnList.clear();
  m_data.truncate(0);
  emit signalDone(this);
  m_started = false;
}

void AmazonFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
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
    kdDebug() << "AmazonFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
kdWarning() << "Remove debug from amazonfetcher.cpp: " << len << endl;
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << QCString(m_data, m_data.size()+1);
  }
  f.close();
#endif

  if(m_total == -1) {
    QDomDocument dom;
    if(!dom.setContent(m_data, false)) {
      kdWarning() << "AmazonFetcher::slotComplete() - server did not return valid XML." << endl;
      stop();
      return;
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

  if(!s_xsltHandler) {
    initXSLTHandler();
    if(!s_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
    }
  }

  // assume amazon is always utf-8
  QString str = s_xsltHandler->applyStylesheet(QString::fromUtf8(m_data));
  Import::TellicoImporter imp(str);
  Data::Collection* coll = imp.collection();
  for(Data::EntryListIterator it(coll->entryList()); it.current(); ++it) {
    // special case for non-books mode and search for title
    if(m_key == Title && coll->type() != Data::Collection::Book) {
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

  if(m_page * AMAZON_RETURNS_PER_REQUEST < KMIN(m_total, AMAZON_MAX_RETURNS_TOTAL)) {
    int foundCount = (m_page-1) * AMAZON_RETURNS_PER_REQUEST + coll->entryCount();
    emit signalStatus(i18n("Results from %1: %2/%3").arg(source()).arg(foundCount).arg(m_total));

    ++m_page;
    m_data.truncate(0);
    search(m_key, m_value, m_multiple);
  } else if(m_primaryMode == true
            && (coll->type() == Data::Collection::Album || coll->type() == Data::Collection::Video)) {
    // repeating search with secondary mode
    m_page = 1;
    m_total = -1;
    m_data.truncate(0);
    m_primaryMode = false;
    search(m_key, m_value, m_multiple);
  } else if(m_multiple && m_value.contains(';') > 9) {
    m_page = 1;
    m_total = -1;
    m_data.truncate(0);
    m_primaryMode = true;
    search(m_key, m_value.section(';', 10), m_multiple);
  } else {
    // tell the user if some of his isbn values were not found
    if(m_key == Fetch::ISBN) {
      const QString isbn = QString::fromLatin1("isbn");
      QStringList isbnNotFound;
      for(QStringList::ConstIterator it = m_isbnList.begin(); it != m_isbnList.end(); ++it) {
        bool found = false;
        for(QIntDictIterator<Data::Entry> eIt(m_entries); eIt.current(); ++eIt) {
          if(eIt.current()->field(isbn) == *it) {
            found = true;
            break;
          }
        }
        if(!found) {
          isbnNotFound.append(*it);
        }
      }
      if(!isbnNotFound.isEmpty()) {
        qHeapSort(isbnNotFound);
        KMessageBox::informationList(0, i18n("<qt>No entries were found for the following ISBN values:</qt>"),
                                     isbnNotFound);
      }
    }
    stop();
  }
}

Tellico::Data::Entry* AmazonFetcher::fetchEntry(uint uid_) {
  static QRegExp* stripHTML = 0;
  if(!stripHTML) {
    stripHTML = new QRegExp(QString::fromLatin1("<.*>"), true);
    stripHTML->setMinimal(true);
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
    case NoImage:
      break;
  }
//  kdDebug() << "AmazonFetcher::fetchEntry() - grabbing " << imageURL.prettyURL() << endl;
  if(!imageURL.isEmpty()) {
    const Data::Image& img = ImageFactory::addImage(imageURL, true);
    // FIXME: need to add cover image field to bibtex collection
    if(img.isNull()) {
    // rich text causes layout issues
//      emit signalStatus(i18n("<qt>The cover image for <i>%1</i> could not be loaded.</qt>").arg(
//                              entry->field(QString::fromLatin1("title"))));
      emit signalStatus(i18n("The cover image could not be loaded."));
    } else if(img.width() > 1 && img.height() > 1) { // amazon serves up 1x1 gifs occasionally
      // all relevant collection types have cover fields
      entry->setField(QString::fromLatin1("cover"), img.id());
    }
  }

  // strip HTML from comments, or plot in movies
  if(entry->collection()->type() == Data::Collection::Video) {
    QString plot = entry->field(QString::fromLatin1("plot"));
    plot.remove(*stripHTML);
    entry->setField(QString::fromLatin1("plot"), plot);
  } else {
    QString comments = entry->field(QString::fromLatin1("comments"));
    comments.remove(*stripHTML);
    entry->setField(QString::fromLatin1("comments"), comments);
  }

  // remove amazon field if it's not to be aded
  if(!m_addLinkField) {
    entry->collection()->deleteField(entry->collection()->fieldByName(QString::fromLatin1("amazon")));
  }
  // don't pollute existing collection with image urls
  entry->collection()->deleteField(QString::fromLatin1("small-image"));
  entry->collection()->deleteField(QString::fromLatin1("medium-image"));
  entry->collection()->deleteField(QString::fromLatin1("large-image"));

  return new Data::Entry(*entry, Kernel::self()->collection());
}

Tellico::Fetch::ConfigWidget* AmazonFetcher::configWidget(QWidget* parent_) {
  return new AmazonFetcher::ConfigWidget(parent_, this);
}

void AmazonFetcher::initXSLTHandler() {
  QString xsltfile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("amazon2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - can not locate amazon2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  if(!s_xsltHandler) {
    s_xsltHandler = new XSLTHandler(u);
  }
  if(!s_xsltHandler->isValid()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - error in amazon2tellico.xsl." << endl;
    delete s_xsltHandler;
    s_xsltHandler = 0;
    return;
  }
}

AmazonFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, AmazonFetcher* fetcher_/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(this, 5, 2);
  l->setSpacing(4);
//  l->setAutoAdd(true);
  QLabel* label = new QLabel(i18n("Co&untry: "), this);
  l->addWidget(label, 0, 0);
  m_siteCombo = new KComboBox(this);
  // these countries MUST be in the same order as the enum
  m_siteCombo->insertItem(i18n("United States"));
  m_siteCombo->insertItem(i18n("United Kingdom"));
  m_siteCombo->insertItem(i18n("Germany"));
  m_siteCombo->insertItem(i18n("Japan"));
  m_siteCombo->insertItem(i18n("France"));
  m_siteCombo->insertItem(i18n("Canada"));
  connect(m_siteCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_siteCombo, 0, 1);
  QString w = i18n("Amazon.com provides data from several different localized sites. Choose the one "
                   "you wish to use for this data source.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_siteCombo, w);
  label->setBuddy(m_siteCombo);

  label = new QLabel(i18n("&Image size: "), this);
  l->addWidget(label, 1, 0);
  m_imageCombo = new KComboBox(this);
  // items must match image enum
  m_imageCombo->insertItem(i18n("Small Image"));
  m_imageCombo->insertItem(i18n("Medium Image"));
  m_imageCombo->insertItem(i18n("Large Image"));
  m_imageCombo->insertItem(i18n("No Image"));
  connect(m_imageCombo, SIGNAL(activated(int)), SLOT(slotSetModified()));
  l->addWidget(m_imageCombo, 1, 1);
  w = i18n("The cover image may be downloaded as well. However, too many large images in the "
           "collection may degrade performance.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_imageCombo, w);
  label->setBuddy(m_imageCombo);

  label = new QLabel(i18n("&Associate's ID: "), this);
  l->addWidget(label, 3, 0);
  m_assocEdit = new KLineEdit(this);
  connect(m_assocEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_assocEdit, 3, 1);
  w = i18n("The associate's id identifies the person accessing the Amazon.com Web Services, and is included "
           "in any links to the Amazon.com site.");
  QWhatsThis::add(label, w);
  QWhatsThis::add(m_assocEdit, w);
  label->setBuddy(m_assocEdit);

  l->setRowStretch(4, 1);

  if(fetcher_) {
    m_siteCombo->setCurrentItem(fetcher_->m_site);
    m_assocEdit->setText(fetcher_->m_assoc);
    m_imageCombo->setCurrentItem(fetcher_->m_imageSize);
  } else { // defaults
    m_assocEdit->setText(QString::fromLatin1(AMAZON_ASSOC_TOKEN));
    m_imageCombo->setCurrentItem(3); // no image
  }
}

void AmazonFetcher::ConfigWidget::saveConfig(KConfig* config_) {
  int n = m_siteCombo->currentItem();
  config_->writeEntry("Site", n);
  QString s = m_assocEdit->text();
  if(!s.isEmpty()) {
    config_->writeEntry("AssocToken", s);
  }
  n = m_imageCombo->currentItem();
  config_->writeEntry("Image Size", n);
  slotSetModified(false);
}

#include "amazonfetcher.moc"
