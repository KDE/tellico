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

#include "bibsonomyfetcher.h"
#include "../translators/bibteximporter.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collection.h"
#include "../entry.h"
#include "../core/netaccess.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QLabel>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  // always bibtex
  static const char* BIBSONOMY_BASE_URL = "http://bibsonomy.org/";
  static const int BIBSONOMY_MAX_RESULTS = 20;
}

using namespace Tellico;
using Tellico::Fetch::BibsonomyFetcher;

BibsonomyFetcher::BibsonomyFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(nullptr), m_started(false) {
}

BibsonomyFetcher::~BibsonomyFetcher() {
}

QString BibsonomyFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool BibsonomyFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Person || k == Keyword;
}

bool BibsonomyFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void BibsonomyFetcher::readConfigHook(const KConfigGroup&) {
}

void BibsonomyFetcher::search() {
  m_started = true;

//  myDebug() << "value = " << value_;

  QUrl u(QString::fromLatin1(BIBSONOMY_BASE_URL));
  u.setPath(QStringLiteral("/bib/"));

  switch(request().key()) {
    case Person:
      u.setPath(u.path() + QStringLiteral("author/%1").arg(request().value()));
      break;

    case Keyword:
      u.setPath(u.path() + QStringLiteral("search/%1").arg(request().value()));
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("items"), QString::number(BIBSONOMY_MAX_RESULTS));
  u.setQuery(q);

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &BibsonomyFetcher::slotComplete);
}

void BibsonomyFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug();
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  Q_EMIT signalDone(this);
}

void BibsonomyFetcher::slotComplete(KJob*) {
//  myDebug();

  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
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
  m_job = nullptr;

  Import::BibtexImporter imp(QString::fromUtf8(data.constData(), data.size()));
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

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }

  stop(); // required
}

Tellico::Data::EntryPtr BibsonomyFetcher::fetchEntryHook(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest BibsonomyFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Fetch::Keyword, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* BibsonomyFetcher::configWidget(QWidget* parent_) const {
  return new BibsonomyFetcher::ConfigWidget(parent_, this);
}

QString BibsonomyFetcher::defaultName() {
  return QStringLiteral("Bibsonomy");
}

QString BibsonomyFetcher::defaultIcon() {
  return favIcon("https://www.bibsonomy.org");
}

BibsonomyFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const BibsonomyFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void BibsonomyFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString BibsonomyFetcher::ConfigWidget::preferredName() const {
  return BibsonomyFetcher::defaultName();
}
