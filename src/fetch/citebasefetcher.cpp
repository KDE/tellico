/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "citebasefetcher.h"
#include "../translators/bibteximporter.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../core/netaccess.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>
#include <QFile>

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

QString CitebaseFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool CitebaseFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void CitebaseFetcher::readConfigHook(const KConfigGroup&) {
}

void CitebaseFetcher::search() {
  m_started = true;

//  myDebug() << "value = " << value_;

  KUrl u = searchURL(request().key, request().value);
  if(u.isEmpty()) {
    stop();
    return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)),
          SLOT(slotComplete(KJob*)));
}

void CitebaseFetcher::stop() {
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

void CitebaseFetcher::slotComplete(KJob*) {
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
  myWarning() << "Remove debug from citebasefetcher.cpp";
  QFile f(QLatin1String("/tmp/test.bib"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Import::BibtexImporter imp(QString::fromUtf8(data, data.size()));
  // to head off the warnings about using the current collection, a dummy bibtex collection is used for now
  imp.setCurrentCollection(Data::CollPtr(new Data::BibtexCollection(true)));
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << "no valid result";
    stop();
    return;
  }

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    if(!m_started) {
      // might get aborted
      break;
    }

    if(request().key == ArxivID && entry->field(QLatin1String("arxiv")).isEmpty()) {
      QString url = entry->field(QLatin1String("url"));
      QRegExp rx(QLatin1String("id=oai:arXiv\\.org:([^&]+)"));
      if(rx.indexIn(url) > -1) {
        if(!coll->hasField(QLatin1String("arxiv"))) {
          Data::FieldPtr field(new Data::Field(QLatin1String("arxiv"), QLatin1String("arXiv ID")));
          field->setCategory(i18n("Publishing"));
          coll->addFields(Data::FieldList() << field);
        }
        entry->setField(QLatin1String("arxiv"), rx.cap(1));
      }
    }

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }

  stop(); // required
}

Tellico::Data::EntryPtr CitebaseFetcher::fetchEntry(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  QRegExp versionRx(QLatin1String("v\\d+$"));
  // if the original search was not for a versioned ID, remove it
  if(request().key != ArxivID || !request().value.contains(versionRx)) {
    QString arxiv = entry->field(QLatin1String("arxiv"));
    arxiv.remove(versionRx);
    entry->setField(QLatin1String("arxiv"), arxiv);
  }
  return entry;
}

KUrl CitebaseFetcher::searchURL(FetchKey key_, const QString& value_) const {
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
      myWarning() << "key not recognized: " << key_;
      return KUrl();
  }

#ifdef CITEBASE_TEST
  u = KUrl("/home/robby/citebase.bib");
#endif
//  myDebug() << "url: " << u.url();
  return u;
}

Tellico::Fetch::FetchRequest CitebaseFetcher::updateRequest(Data::EntryPtr entry_) {
  QString arxiv = entry_->field(QLatin1String("arxiv"));
  if(!arxiv.isEmpty()) {
    // remove prefix and/or version number
    arxiv.remove(QRegExp(QLatin1String("^arxiv:"), Qt::CaseInsensitive));
    arxiv.remove(QRegExp(QLatin1String("v\\d+$")));
    return FetchRequest(Fetch::ArxivID, arxiv);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* CitebaseFetcher::configWidget(QWidget* parent_) const {
  return new CitebaseFetcher::ConfigWidget(parent_, this);
}

QString CitebaseFetcher::defaultName() {
  return QLatin1String("Citebase");
}

QString CitebaseFetcher::defaultIcon() {
  return favIcon("http://citebase.org");
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
