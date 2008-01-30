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

#include "bibsonomyfetcher.h"
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

namespace {
  // always bibtex
  static const char* BIBSONOMY_BASE_URL = "http://bibsonomy.org";
  static const int BIBSONOMY_MAX_RESULTS = 20;
}

using Tellico::Fetch::BibsonomyFetcher;

BibsonomyFetcher::BibsonomyFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(0), m_started(false) {
}

BibsonomyFetcher::~BibsonomyFetcher() {
}

QString BibsonomyFetcher::defaultName() {
  return QString::fromLatin1("Bibsonomy");
}

QString BibsonomyFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool BibsonomyFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void BibsonomyFetcher::readConfigHook(const KConfigGroup&) {
}

void BibsonomyFetcher::search(FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_.stripWhiteSpace();
  m_started = true;

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.").arg(source()), MessageHandler::Warning);
    stop();
    return;
  }

  m_data.truncate(0);

//  myDebug() << "BibsonomyFetcher::search() - value = " << value_ << endl;

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

void BibsonomyFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug() << "BibsonomyFetcher::stop()" << endl;
  if(m_job) {
    m_job->kill();
    m_job = 0;
  }
  m_data.truncate(0);
  m_started = false;
  emit signalDone(this);
}

void BibsonomyFetcher::slotData(KIO::Job*, const QByteArray& data_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(data_.data(), data_.size());
}

void BibsonomyFetcher::slotComplete(KIO::Job* job_) {
//  myDebug() << "BibsonomyFetcher::slotComplete()" << endl;
  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;

  if(job_->error()) {
    job_->showErrorDialog(Kernel::self()->widget());
    stop();
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "BibsonomyFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

  Import::BibtexImporter imp(QString::fromUtf8(m_data, m_data.size()));
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << "BibsonomyFetcher::slotComplete() - no valid result" << endl;
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

Tellico::Data::EntryPtr BibsonomyFetcher::fetchEntry(uint uid_) {
  return m_entries[uid_];
}

KURL BibsonomyFetcher::searchURL(FetchKey key_, const QString& value_) const {
  KURL u(QString::fromLatin1(BIBSONOMY_BASE_URL));
  u.setPath(QString::fromLatin1("/bib/"));

  switch(key_) {
    case Person:
      u.addPath(QString::fromLatin1("author/%1").arg(value_));
      break;

    case Keyword:
      u.addPath(QString::fromLatin1("search/%1").arg(value_));
      break;

    default:
      kdWarning() << "BibsonomyFetcher::search() - key not recognized: " << m_key << endl;
      return KURL();
  }

  u.addQueryItem(QString::fromLatin1("items"), QString::number(BIBSONOMY_MAX_RESULTS));
  myDebug() << "BibsonomyFetcher::search() - url: " << u.url() << endl;
  return u;
}

void BibsonomyFetcher::updateEntry(Data::EntryPtr entry_) {
  QString title = entry_->field(QString::fromLatin1("title"));
  if(!title.isEmpty()) {
    search(Fetch::Keyword, title);
    return;
  }

  myDebug() << "BibsonomyFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

Tellico::Fetch::ConfigWidget* BibsonomyFetcher::configWidget(QWidget* parent_) const {
  return new BibsonomyFetcher::ConfigWidget(parent_, this);
}

BibsonomyFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const BibsonomyFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void BibsonomyFetcher::ConfigWidget::saveConfig(KConfigGroup&) {
}

QString BibsonomyFetcher::ConfigWidget::preferredName() const {
  return BibsonomyFetcher::defaultName();
}

#include "bibsonomyfetcher.moc"
