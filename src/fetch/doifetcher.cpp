/***************************************************************************
    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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

#include "doifetcher.h"
#include "../translators/bibteximporter.h"
#include "../utils/guiproxy.h"
#include "../collection.h"
#include "../entry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>

namespace {
  static const char* DOI_BASE_URL = "https://doi.org/";
}

using namespace Tellico;
using namespace Tellico::Fetch;
using Tellico::Fetch::DOIFetcher;

DOIFetcher::DOIFetcher(QObject* parent_)
    : Fetcher(parent_), m_job(nullptr), m_started(false) {
}

DOIFetcher::~DOIFetcher() = default;

QString DOIFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DOIFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void DOIFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void DOIFetcher::search() {
  m_started = true;

  QUrl u(QString::fromLatin1(DOI_BASE_URL));
  switch(request().key()) {
    case DOI:
      u.setPath(u.path() + request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
//  myLog() << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/x-bibtex"));
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &DOIFetcher::slotComplete);
}

void DOIFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  Q_EMIT signalDone(this);
}

void DOIFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    myLog() << "Error:" << m_job->errorString();
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;
#if 0
  myWarning() << "Remove debug from doifetcher.cpp";
  QFile f(QLatin1String("/tmp/test-doi.bib"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

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
    m_entries.insert(r->uid, Data::EntryPtr(entry));
    Q_EMIT signalResultFound(r);
  }

  stop(); // required
}

Tellico::Data::EntryPtr DOIFetcher::fetchEntryHook(uint uid_) {
  return m_entries[uid_];
}

Tellico::Fetch::FetchRequest DOIFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString doi = entry_->field(QStringLiteral("doi"));
  return doi.isEmpty() ? FetchRequest() : FetchRequest(Fetch::DOI, doi);
}

Tellico::Fetch::ConfigWidget* DOIFetcher::configWidget(QWidget* parent_) const {
  return new DOIFetcher::ConfigWidget(parent_, this);
}

QString DOIFetcher::defaultName() {
  return QStringLiteral("DOI.org"); // no translation
}

QString DOIFetcher::defaultIcon() {
  return favIcon("https://www.doi.org");
}

DOIFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DOIFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(DOIFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

}

QString DOIFetcher::ConfigWidget::preferredName() const {
  return DOIFetcher::defaultName();
}
