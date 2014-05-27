/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "hathitrustfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/isbnvalidator.h"
#include "../utils/lccnvalidator.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KStandardDirs>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>

#ifdef HAVE_QJSON
#include <qjson/parser.h>
#endif

namespace {
  static const char* HATHITRUST_QUERY_URL = "http://catalog.hathitrust.org/api/volumes/full/json/";
}

using namespace Tellico;
using Tellico::Fetch::HathiTrustFetcher;

HathiTrustFetcher::HathiTrustFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_MARC21XMLHandler(0), m_MODSHandler(0) {
}

HathiTrustFetcher::~HathiTrustFetcher() {
  delete m_MARC21XMLHandler;
  m_MARC21XMLHandler = 0;
  delete m_MODSHandler;
  m_MODSHandler = 0;
}

QString HathiTrustFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool HathiTrustFetcher::canSearch(FetchKey k) const {
#ifdef HAVE_QJSON
  return k == ISBN || k == LCCN;
#else
  return false;
#endif
}

bool HathiTrustFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void HathiTrustFetcher::readConfigHook(const KConfigGroup&) {
}

void HathiTrustFetcher::search() {
  m_started = true;
#ifdef HAVE_QJSON
  doSearch();
#else
  stop();
#endif
}

void HathiTrustFetcher::doSearch() {
  KUrl u(HATHITRUST_QUERY_URL);

  QStringList searchValues;
  // we split ISBN and LCCN values, which are the only ones we accept anyway
  const QStringList searchTerms = FieldFormat::splitValue(request().value);
  foreach(const QString& searchTerm, searchTerms) {
    if(request().key == ISBN) {
      searchValues += QString::fromLatin1("isbn:%1").arg(ISBNValidator::cleanValue(searchTerm));
    } else {
      searchValues += QString::fromLatin1("lccn:%1").arg(LCCNValidator::formalize(searchTerm));
    }
  }
  u.addPath(searchValues.join(QLatin1String("|")));

  myDebug() << "url:" << u.url();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->ui()->setWindow(GUI::Proxy::widget());
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void HathiTrustFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
  }
  m_started = false;
  emit signalDone(this);
}

bool HathiTrustFetcher::initMARC21Handler() {
  if(m_MARC21XMLHandler) {
    return true;
  }

  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate MARC21slim2MODS3.xsl.";
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);

  m_MARC21XMLHandler = new XSLTHandler(u);
  if(!m_MARC21XMLHandler->isValid()) {
    myWarning() << "error in MARC21slim2MODS3.xsl.";
    delete m_MARC21XMLHandler;
    m_MARC21XMLHandler = 0;
    return false;
  }
  return true;
}

bool HathiTrustFetcher::initMODSHandler() {
  if(m_MODSHandler) {
    return true;
  }

  QString xsltfile = KStandardDirs::locate("appdata", QLatin1String("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate mods2tellico.xsl.";
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);

  m_MODSHandler = new XSLTHandler(u);
  if(!m_MODSHandler->isValid()) {
    myWarning() << "error in mods2tellico.xsl.";
    delete m_MODSHandler;
    m_MODSHandler = 0;
    // no use in keeping the MARC handlers now
    delete m_MARC21XMLHandler;
    m_MARC21XMLHandler = 0;
    return false;
  }
  return true;
}

Tellico::Data::EntryPtr HathiTrustFetcher::fetchEntryHook(uint uid_) {
  return m_entries.value(uid_);
}

Tellico::Fetch::FetchRequest HathiTrustFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  const QString lccn = entry_->field(QLatin1String("lccn"));
  if(!lccn.isEmpty()) {
    return FetchRequest(LCCN, lccn);
  }
  return FetchRequest();
}

void HathiTrustFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

#ifdef HAVE_QJSON
//  myDebug();
  if(!initMARC21Handler() || !initMODSHandler()) {
    // debug messages are taken care of in the specific methods
    stop();
    return;
  }

  if(job->error()) {
    job->ui()->showErrorMessage();
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
  m_job = 0;

#if 0
  myWarning() << "Remove debug from hathitrustfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJson::Parser parser;
  QVariantMap resultMap = parser.parse(data).toMap();
  if(resultMap.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  QVariantMap::const_iterator i = resultMap.constBegin();
  for( ; i != resultMap.constEnd(); ++i) {
    const QVariantMap recordMap = i.value().toMap().value(QLatin1String("records")).toMap();
    if(recordMap.isEmpty()) {
      myDebug() << "empty result map";
      continue;
    }
    // we know there's a record, so no need to check for existence of first iterator in map
    QVariantMap::const_iterator ri = recordMap.constBegin();
    if(ri == recordMap.constEnd()) {
      myWarning() << "no iterator in record";
      continue;
    }
    const QString marcxml = ri.value().toMap().value(QLatin1String("marc-xml")).toString();
    const QString modsxml = m_MARC21XMLHandler->applyStylesheet(marcxml);

    Import::TellicoImporter imp(m_MODSHandler->applyStylesheet(modsxml));
    imp.setOptions(imp.options() ^ Import::ImportProgress); // no progress needed
    Data::CollPtr coll = imp.collection();
    if(!coll) {
      myWarning() << "no coll pointer";
      continue;
    }

    // since the Dewey and LoC field titles have a context in their i18n call here
    // but not in the mods2tellico.xsl stylesheet where the field is actually created
    // update the field titles here
    QHashIterator<QString, QString> i(allOptionalFields());
    while(i.hasNext()) {
      i.next();
      Data::FieldPtr field = coll->fieldByName(i.key());
      if(field) {
        field->setTitle(i.value());
        coll->modifyField(field);
      }
    }


    foreach(Data::EntryPtr entry, coll->entries()) {
      FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
      m_entries.insert(r->uid, entry);
      emit signalResultFound(r);
    }
  }

  m_hasMoreResults = false; // for now, no continued searches
#endif
  stop();
}

Tellico::Fetch::ConfigWidget* HathiTrustFetcher::configWidget(QWidget* parent_) const {
  return new HathiTrustFetcher::ConfigWidget(parent_, this);
}

QString HathiTrustFetcher::defaultName() {
  return QLatin1String("HathiTrust"); // no translation
}

QString HathiTrustFetcher::defaultIcon() {
  return favIcon("http://http://www.hathitrust.org");
}

Tellico::StringHash HathiTrustFetcher::allOptionalFields() {
  // same ones as z3950fetcher
  StringHash hash;
  hash[QLatin1String("address")]  = i18n("Address");
  hash[QLatin1String("abstract")] = i18n("Abstract");
  hash[QLatin1String("illustrator")] = i18n("Illustrator");
  hash[QLatin1String("dewey")] = i18nc("Dewey Decimal classification system", "Dewey Decimal");
  hash[QLatin1String("lcc")] = i18nc("Library of Congress classification system", "LoC Classification");
  return hash;
}

HathiTrustFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const HathiTrustFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(HathiTrustFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void HathiTrustFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString HathiTrustFetcher::ConfigWidget::preferredName() const {
  return HathiTrustFetcher::defaultName();
}

#include "hathitrustfetcher.moc"
