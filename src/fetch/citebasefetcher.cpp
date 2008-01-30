/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "citebasefetcher.h"
#include "messagehandler.h"
#include "../translators/bibteximporter.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../core/netaccess.h"
#include "../filehandler.h"
#include "../tellico_debug.h"

#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>

// #define CITEBASE_TEST

namespace {
  // always bibtex
  static const char* CITEBASE_BASE_URL = "http://www.citebase.org/openurl/?url_ver=Z39.88-2004&svc_id=bibtex";
}

using Tellico::Fetch::CitebaseFetcher;

CitebaseFetcher::CitebaseFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(0), m_started(false) {
}

CitebaseFetcher::~CitebaseFetcher() {
}

QString CitebaseFetcher::defaultName() {
  return QString::fromLatin1("Citebase");
}

QString CitebaseFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool CitebaseFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void CitebaseFetcher::readConfigHook(const KConfigGroup&) {
}

void CitebaseFetcher::search(FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_.stripWhiteSpace();
  m_started = true;

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
    stop();
    return;
  }

  m_data.truncate(0);

//  myDebug() << "CitebaseFetcher::search() - value = " << value_ << endl;

  KURL u = searchURL(m_key, m_value);
  if(u.isEmpty()) {
    stop();
    return;
  }

  m_job = KIO::get(u, false, false);
  connect(m_job, SIGNAL(data(KIO::Job*, const QByteArray&)),
          SLOT(slotData(KIO::Job*, const QByteArray&)));
  connect(m_job, SIGNAL(result(KIO::Job*)),
          SLOT(slotComplete(KIO::Job*)));
}

void CitebaseFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug() << "CitebaseFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_data.truncate(0);
  m_started = false;
  emit signalDone(this);
}

void CitebaseFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void CitebaseFetcher::slotComplete(KIO::Job* job_) {
//  myDebug() << "CitebaseFetcher::slotComplete()" << endl;
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "CitebaseFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

#if 0
  kdWarning() << "Remove debug from citebasefetcher.cpp" << endl;
  QFile f(QString::fromLatin1("/tmp/test.bib"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << QCString(m_data, m_data.size()+1);
  }
  f.close();
#endif

  Import::BibtexImporter imp(QString::fromUtf8(m_data, m_data.size()));
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << "CitebaseFetcher::slotComplete() - no valid result" << endl;
    stop();
    return;
  }

  Data::EntryVec entries = coll->entries();
  for(Data::EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    if(!m_started) {
      // might get aborted
      break;
    }
    QString desc = entry->field(QString::fromLatin1("author"))
                 + QChar('/') + entry->field(QString::fromLatin1("publisher"));
    if(!entry->field(QString::fromLatin1("year")).isEmpty()) {
      desc += QChar('/') + entry->field(QString::fromLatin1("year"));
    }

    SearchResult* r = new SearchResult(this, entry->title(), desc, entry->field(QString::fromLatin1("isbn")));
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }

  stop(); // required
}

Tellico::Data::EntryPtr CitebaseFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

KURL CitebaseFetcher::searchURL(FetchKey key_, const QString& value_) const {
  KURL u(QString::fromLatin1(CITEBASE_BASE_URL));

  switch(key_) {
    case ArxivID:
      {
      // remove prefix and/or version number
      QString value = value_;
      value.remove(QRegExp(QString::fromLatin1("^arxiv:"), false));
      value.remove(QRegExp(QString::fromLatin1("v\\d+$")));
      u.addQueryItem(QString::fromLatin1("rft_id"), QString::fromLatin1("oai:arXiv.org:%1").arg(value));
      }
      break;

    default:
      kdWarning() << "CitebaseFetcher::search() - key not recognized: " << m_key << endl;
      return KURL();
  }

#ifdef CITEBASE_TEST
  u = KURL::fromPathOrURL(QString::fromLatin1("/home/robby/citebase.bib"));
#endif
  myDebug() << "CitebaseFetcher::search() - url: " << u.url() << endl;
  return u;
}

void CitebaseFetcher::updateEntry(Data::EntryPtr entry_) {
  QString arxiv = entry_->field(QString::fromLatin1("arxiv"));
  if(!arxiv.isEmpty()) {
    search(Fetch::ArxivID, arxiv);
    return;
  }

  myDebug() << "CitebaseFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

void CitebaseFetcher::updateEntrySynchronous(Data::EntryPtr entry) {
  if(!entry) {
    return;
  }
  QString arxiv = entry->field(QString::fromLatin1("arxiv"));
  if(arxiv.isEmpty()) {
    return;
  }

  KURL u = searchURL(ArxivID, arxiv);
  QString bibtex = FileHandler::readTextFile(u, true);
  if(bibtex.isEmpty()) {
    return;
  }

  // assume result is always utf-8
  Import::BibtexImporter imp(bibtex);
  Data::CollPtr coll = imp.collection();
  if(coll && coll->entryCount() > 0) {
    myLog() << "CitebaseFetcher::updateEntrySynchronous() - found arxiv result, merging" << endl;
    Data::Collection::mergeEntry(entry, coll->entries().front(), false /*overwrite*/);
  }
}

Tellico::Fetch::ConfigWidget* CitebaseFetcher::configWidget(QWidget* parent_) const {
  return new CitebaseFetcher::ConfigWidget(parent_, this);
}

CitebaseFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const CitebaseFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void CitebaseFetcher::ConfigWidget::saveConfig(KConfigGroup&) {
}

QString CitebaseFetcher::ConfigWidget::preferredName() const {
  return CitebaseFetcher::defaultName();
}

#include "citebasefetcher.moc"
