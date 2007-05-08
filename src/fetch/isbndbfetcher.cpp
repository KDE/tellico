/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "isbndbfetcher.h"
#include "messagehandler.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"

#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>

#include <qdom.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qfile.h>

namespace {
  static const int ISBNDB_MAX_RETURNS_TOTAL = 10;
  static const char* ISBNDB_BASE_URL = "http://isbndb.com/api/books.xml";
  static const char* ISBNDB_APP_ID = "3B9S3BQS";
}

using Tellico::Fetch::ISBNdbFetcher;

ISBNdbFetcher::ISBNdbFetcher(QObject* parent_, const char* name_)
    : Fetcher(parent_, name_), m_xsltHandler(0),
      m_limit(ISBNDB_MAX_RETURNS_TOTAL), m_job(0), m_started(false) {
}

ISBNdbFetcher::~ISBNdbFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = 0;
}

QString ISBNdbFetcher::defaultName() {
  return i18n("ISBNdb.com");
}

QString ISBNdbFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ISBNdbFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void ISBNdbFetcher::readConfigHook(KConfig* config_, const QString& group_) {
  Q_UNUSED(config_);
  Q_UNUSED(group_);
}

void ISBNdbFetcher::search(FetchKey key_, const QString& value_) {
  m_started = true;

//  myDebug() << "ISBNdbFetcher::search() - value = " << value_ << endl;

  KURL u(QString::fromLatin1(ISBNDB_BASE_URL));
  u.addQueryItem(QString::fromLatin1("access_key"),   QString::fromLatin1(ISBNDB_APP_ID));

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
    stop();
    return;
  }

  switch(key_) {
    case Title:
      u.addQueryItem(QString::fromLatin1("index1"), QString::fromLatin1("title"));
      u.addQueryItem(QString::fromLatin1("value1"), value_);
      break;

    case ISBN:
      u.addQueryItem(QString::fromLatin1("index1"), QString::fromLatin1("isbn"));
      {
        QString v = value_;
        v.remove('-');
        u.addQueryItem(QString::fromLatin1("value1"), v);
      }
      break;

    default:
      kdWarning() << "ISBNdbFetcher::search() - key not recognized: " << key_ << endl;
      stop();
      return;
  }
//  myDebug() << "ISBNdbFetcher::search() - url: " << u.url() << endl;

  m_job = KIO::get(u, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void ISBNdbFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug() << "ISBNdbFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_data.truncate(0);
  m_started = false;
  emit signalDone(this);
}

void ISBNdbFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void ISBNdbFetcher::slotComplete(KIO::Job* job_) {
//  myDebug() << "ISBNdbFetcher::slotComplete()" << endl;
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "ISBNdbFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
  kdWarning() << "Remove debug from isbndbfetcher.cpp" << endl;
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << QCString(m_data, m_data.size()+1);
  }
  f.close();
#endif

  QDomDocument dom;
  if(!dom.setContent(m_data, false)) {
    kdWarning() << "ISBNdbFetcher::slotComplete() - server did not return valid XML." << endl;
    return;
  }

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  // assume result is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(m_data, m_data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();

  int count = 0;
  Data::EntryVec entries = coll->entries();
  for(Data::EntryVec::Iterator entry = entries.begin(); count < m_limit && entry != entries.end(); ++entry, ++count) {
    if(!m_started) {
      // might get aborted
      break;
    }
    QString desc = entry->field(QString::fromLatin1("author"))
                 + QChar('/') + entry->field(QString::fromLatin1("publisher"));
    if(!entry->field(QString::fromLatin1("cr_year")).isEmpty()) {
      desc += QChar('/') + entry->field(QString::fromLatin1("cr_year"));
    } else if(!entry->field(QString::fromLatin1("pub_year")).isEmpty()){
      desc += QChar('/') + entry->field(QString::fromLatin1("pub_year"));
    }

    SearchResult* r = new SearchResult(this, entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }
  stop(); // required
}

Tellico::Data::EntryPtr ISBNdbFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  if(!entry) {
    kdWarning() << "ISBNdbFetcher::fetchEntry() - no entry in dict" << endl;
    return 0;
  }

  return entry;
}

void ISBNdbFetcher::initXSLTHandler() {
  QString xsltfile = locate("appdata", QString::fromLatin1("isbndb2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    kdWarning() << "ISBNdbFetcher::initXSLTHandler() - can not locate isbndb2tellico.xsl." << endl;
    return;
  }

  KURL u;
  u.setPath(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    kdWarning() << "ISBNdbFetcher::initXSLTHandler() - error in isbndb2tellico.xsl." << endl;
    delete m_xsltHandler;
    m_xsltHandler = 0;
    return;
  }
}

void ISBNdbFetcher::updateEntry(Data::EntryPtr entry_) {
//  myDebug() << "ISBNdbFetcher::updateEntry()" << endl;
  // limit to top 5 results
  m_limit = 5;

  QString isbn = entry_->field(QString::fromLatin1("isbn"));
  if(!isbn.isEmpty()) {
    search(Fetch::ISBN, isbn);
    return;
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QString::fromLatin1("title"));
  if(!t.isEmpty()) {
    m_limit = 10; // raise limit so more possibility of match
    search(Fetch::Title, t);
    return;
  }

  myDebug() << "ISBNdbFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* ISBNdbFetcher::configWidget(QWidget* parent_) const {
  return new ISBNdbFetcher::ConfigWidget(parent_, this);
}

ISBNdbFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ISBNdbFetcher*/*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString ISBNdbFetcher::ConfigWidget::preferredName() const {
  return ISBNdbFetcher::defaultName();
}

#include "isbndbfetcher.moc"
