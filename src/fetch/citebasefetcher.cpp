/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
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
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>

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
  return QLatin1String("Citebase");
}

QString CitebaseFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool CitebaseFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void CitebaseFetcher::readConfigHook(const KConfigGroup&) {
}

void CitebaseFetcher::search(Tellico::Fetch::FetchKey key_, const QString& value_) {
  m_key = key_;
  m_value = value_.trimmed();
  m_started = true;

  if(!canFetch(Kernel::self()->collectionType())) {
    message(i18n("%1 does not allow searching for this collection type.", source()), MessageHandler::Warning);
    stop();
    return;
  }

//  myDebug() << "CitebaseFetcher::search() - value = " << value_ << endl;

  KUrl u = searchURL(m_key, m_value);
  if(u.isEmpty()) {
    stop();
    return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(Kernel::self()->widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
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
  m_started = false;
  emit signalDone(this);
}

void CitebaseFetcher::slotComplete(KJob*) {
//  myDebug() << "CitebaseFetcher::slotComplete()" << endl;

  if(m_job->error()) {
    m_job->ui()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "CitebaseFetcher::slotComplete() - no data" << endl;
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = 0;
#if 0
  kWarning() << "Remove debug from citebasefetcher.cpp";
  QFile f(QLatin1String("/tmp/test.bib"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setEncoding(QTextStream::UnicodeUTF8);
    t << data;
  }
  f.close();
#endif

  Import::BibtexImporter imp(QString::fromUtf8(data, data.size()));
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << "CitebaseFetcher::slotComplete() - no valid result" << endl;
    stop();
    return;
  }

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    if(!m_started) {
      // might get aborted
      break;
    }
    QString desc = entry->field(QLatin1String("author"))
                 + QChar('/') + entry->field(QLatin1String("publisher"));
    if(!entry->field(QLatin1String("year")).isEmpty()) {
      desc += QChar('/') + entry->field(QLatin1String("year"));
    }

    SearchResult* r = new SearchResult(Fetcher::Ptr(this), entry->title(), desc, entry->field(QLatin1String("isbn")));
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }

  stop(); // required
}

Tellico::Data::EntryPtr CitebaseFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  QRegExp versionRx(QLatin1String("v\\d+$"));
  // if the original search was not for a versioned ID, remove it
  if(m_key != ArxivID || !m_value.contains(versionRx)) {
    QString arxiv = entry->field(QLatin1String("arxiv"));
    arxiv.remove(versionRx);
    entry->setField(QLatin1String("arxiv"), arxiv);
  }
  return entry;
}

KUrl CitebaseFetcher::searchURL(Tellico::Fetch::FetchKey key_, const QString& value_) const {
  KUrl u(CITEBASE_BASE_URL);

  switch(key_) {
    case ArxivID:
      {
      // remove prefix and/or version number
      QString value = value_;
      value.remove(QRegExp(QLatin1String("^arxiv:"), Qt::CaseInsensitive));
      value.remove(QRegExp(QLatin1String("v\\d+$")));
      u.addQueryItem(QLatin1String("rft_id"), QString::fromLatin1("oai:arXiv.org:%1").arg(value));
      }
      break;

    default:
      kWarning() << "CitebaseFetcher::search() - key not recognized: " << m_key;
      return KUrl();
  }

#ifdef CITEBASE_TEST
  u = KUrl("/home/robby/citebase.bib");
#endif
  myDebug() << "CitebaseFetcher::search() - url: " << u.url() << endl;
  return u;
}

void CitebaseFetcher::updateEntry(Tellico::Data::EntryPtr entry_) {
  QString arxiv = entry_->field(QLatin1String("arxiv"));
  if(!arxiv.isEmpty()) {
    search(Fetch::ArxivID, arxiv);
    return;
  }

  myDebug() << "CitebaseFetcher::updateEntry() - insufficient info to search" << endl;
  emit signalDone(this); // always need to emit this if not continuing with the search
}

void CitebaseFetcher::updateEntrySynchronous(Tellico::Data::EntryPtr entry) {
  if(!entry) {
    return;
  }
  QString arxiv = entry->field(QLatin1String("arxiv"));
  if(arxiv.isEmpty()) {
    return;
  }

  KUrl u = searchURL(ArxivID, arxiv);
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
