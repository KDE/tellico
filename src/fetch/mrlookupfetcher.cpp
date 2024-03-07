/***************************************************************************
    Copyright (C) 2014-2020 Robby Stephenson <robby@periapsis.org>
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

#include "mrlookupfetcher.h"
#include "../translators/bibteximporter.h"
#include "../collections/bibtexcollection.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KConfigGroup>
#include <KJobWidgets>

#include <QLabel>
#include <QVBoxLayout>
#include <QFile>
#include <QTextCodec>
#include <QUrlQuery>

namespace {
  static const char* MRLOOKUP_URL = "https://mathscinet.ams.org/mrlookup";
}

using namespace Tellico;
using Tellico::Fetch::MRLookupFetcher;

MRLookupFetcher::MRLookupFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

MRLookupFetcher::~MRLookupFetcher() {
}

QString MRLookupFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool MRLookupFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person;
}

bool MRLookupFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void MRLookupFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void MRLookupFetcher::search() {
  m_started = true;

  QUrl u(QString::fromLatin1(MRLOOKUP_URL));
  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("ti"), request().value());
      break;

    case Person:
      q.addQueryItem(QStringLiteral("au"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  q.addQueryItem(QStringLiteral("format"), QStringLiteral("bibtex"));
  u.setQuery(q);

//  myDebug() << u;
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &MRLookupFetcher::slotComplete);
}

void MRLookupFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  emit signalDone(this);
}

Tellico::Data::EntryPtr MRLookupFetcher::fetchEntryHook(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest MRLookupFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void MRLookupFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from mrlookup.cpp";
  QFile f(QString::fromLatin1("/tmp/test-mrlookup.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif
  const QString text = QString::fromUtf8(data.constData(), data.size());
  QString bibtexString;

  // grab everything within the <pre></pre> block
  static const QRegularExpression preRx(QLatin1String("<pre>(.+?)</pre>"), QRegularExpression::DotMatchesEverythingOption);
  QRegularExpressionMatchIterator i = preRx.globalMatch(text);
  while(i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    bibtexString += match.captured(1);
  }
  if(bibtexString.isEmpty()) {
    myDebug() << "no bibtex response";
    stop();
    return;
  }
#if 0
  myWarning() << "Remove debug from mrlookup.cpp";
  QFile f(QString::fromLatin1("/tmp/test.bibtex"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << bibtexString;
  }
  f.close();
#endif

  Import::BibtexImporter imp(bibtexString);
  // quiet warnings...
  imp.setCurrentCollection(Data::CollPtr(new Data::BibtexCollection(true)));
  Data::CollPtr coll = imp.collection();
  if(!coll) {
    myDebug() << "no collection pointer";
    stop();
    return;
  }

  // switch the FJournal field with the Journal field
  foreach(Data::EntryPtr entry, coll->entries()) {
    entry->setField(QStringLiteral("journal"), entry->field(QStringLiteral("fjournal")));
  }
  coll->removeField(QStringLiteral("fjournal"));
  // unnecessary fields
  coll->removeField(QStringLiteral("mrclass"));
  coll->removeField(QStringLiteral("mrnumber"));
  coll->removeField(QStringLiteral("mrreviewer"));

  foreach(Data::EntryPtr entry, coll->entries()) {
    if(!m_started) {
      // might get aborted
      break;
    }

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    emit signalResultFound(r);
  }
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches

  stop(); // required
}

Tellico::Fetch::ConfigWidget* MRLookupFetcher::configWidget(QWidget* parent_) const {
  return new MRLookupFetcher::ConfigWidget(parent_, this);
}

QString MRLookupFetcher::defaultName() {
  return QStringLiteral("Mathematical Reviews");
}

QString MRLookupFetcher::defaultIcon() {
  return favIcon("http://www.ams.org");
}

MRLookupFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MRLookupFetcher* /*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

QString MRLookupFetcher::ConfigWidget::preferredName() const {
  return MRLookupFetcher::defaultName();
}
