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

#include "srufetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../tellico_kernel.h"

#include <klocale.h>
#include <kdebug.h>
#include <kio/job.h>
#include <kstandarddirs.h>

#include <qlabel.h>
#include <qlayout.h>

using Tellico::Fetch::SRUFetcher;

SRUFetcher::SRUFetcher(QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_job(0), m_xsltHandler(0), m_collMerged(false), m_started(false) {
  m_results.setAutoDelete(true); // entries will be handles in destructor
}

SRUFetcher::~SRUFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;

  cleanUp();
}

QString SRUFetcher::source() const {
  return i18n("Library of Congress (US)");
}

void SRUFetcher::cleanUp() {
  // need to delete collection pointers
  QPtrList<Data::Collection> collList;
  for(QIntDictIterator<Data::Entry> it(m_entries); it.current(); ++it) {
    if(collList.findRef(it.current()->collection()) == -1) {
      collList.append(it.current()->collection());
    }
  }
  collList.setAutoDelete(true); // will automatically delete all entries
}

// multiple values not supported
void SRUFetcher::search(FetchKey key_, const QString& value_, bool) {
  m_started = true;
#if 1
  KURL u(QString::fromLatin1("http://z3950.loc.gov:7090/voyager"));
  u.addQueryItem(QString::fromLatin1("operation"), QString::fromLatin1("searchRetrieve"));
  u.addQueryItem(QString::fromLatin1("version"), QString::fromLatin1("1.1"));
  u.addQueryItem(QString::fromLatin1("maximumRecords"), QString::fromLatin1("25"));
  u.addQueryItem(QString::fromLatin1("recordSchema"), QString::fromLatin1("mods"));

  Data::Collection::Type type = Kernel::self()->collection()->type();
  QString str = QChar('"') + value_ + QChar('"');
  switch(key_) {
    case Title:
      u.addQueryItem(QString::fromLatin1("query"), QString::fromLatin1("dc.title=") + str);
      break;

    case Person:
      {
        QString s;
        if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
          s = QString::fromLatin1("author=") + str + QString::fromLatin1(" or dc.author=") + str;
        } else {
          s = QString::fromLatin1("dc.creator=") + str + QString::fromLatin1(" or dc.editor=") + str;
        }
        u.addQueryItem(QString::fromLatin1("query"), s);
      }
      break;

    case ISBN:
      {
        // lccn searches here, don't do any isbn validation, just trust the user
        str.replace('-', QString::null);
        QString s;
        if(type == Data::Collection::Book || type == Data::Collection::Bibtex) {
          s = QString::fromLatin1("bath.isbn=") + str + QString::fromLatin1(" or bath.lccn=") + str;
        } else {
          s = QString::fromLatin1("bath.isbn=") + str + QString::fromLatin1(" or bath.issn=") + str;
        }
        u.addQueryItem(QString::fromLatin1("query"), s);
      }
      break;

    case Keyword:
      u.addQueryItem(QString::fromLatin1("query"), str);
      break;

    case Raw:
      {
        QString key = value_.section('=', 0, 0).stripWhiteSpace();
        QString str = value_.section('=', 1).stripWhiteSpace();
        u.addQueryItem(key, str);
      }
      break;

    default:
      kdWarning() << "SRUFetcher::search() - key not recognized: " << key_ << endl;
      stop();
      break;
  }

  kdDebug() << u.prettyURL() << endl;
  m_job = KIO::get(u, false, false);
#else
  m_job = KIO::get(KURL::fromPathOrURL(QString::fromLatin1("/home/robby/tmp/mods/voyager.xml")), false, false);
#endif
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void SRUFetcher::stop() {
  if(!m_started) {
    return;
  }
  kdDebug() << "SRUFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_collMerged = false;
  m_data.truncate(0);
  emit signalDone(this);
  m_started = false;
}

void SRUFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void SRUFetcher::slotComplete(KIO::Job* job_) {
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

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
    }
  }

  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(m_data));
  Import::TellicoImporter imp(str);
  Data::Collection* coll = imp.collection();
  for(Data::EntryListIterator it(coll->entryList()); it.current(); ++it) {
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
               + it.current()->field(QString::fromLatin1("year"));
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
  stop();
}

Tellico::Data::Entry* SRUFetcher::fetchEntry(uint uid_) {
  kdDebug() << "SRUFetcher::fetchEntry() - looking for " << m_results[uid_]->desc << endl;
  Data::Entry* entry = m_entries[uid_];

  // merge all fields
  if(!m_collMerged) {
    for(Data::FieldListIterator fIt(entry->collection()->fieldList()); fIt.current(); ++fIt) {
      Kernel::self()->collection()->mergeField(fIt.current());
    }
    m_collMerged = true;
  }
  return new Data::Entry(*entry, Kernel::self()->collection());
}

void SRUFetcher::initXSLTHandler() {
  QString xsltfile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - can not locate mods2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kdWarning() << "AmazonFetcher::initXSLTHandler() - error in mods2tellico.xsl." << endl;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

Tellico::Fetch::ConfigWidget* SRUFetcher::configWidget(QWidget* parent_) {
  return new SRUFetcher::ConfigWidget(parent_);
}

SRUFetcher::ConfigWidget::ConfigWidget(QWidget* parent_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(new QLabel(i18n("This source has no options."), this));
  l->addStretch();
}

#include "srufetcher.moc"
