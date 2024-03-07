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

#include "hathitrustfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/isbnvalidator.h"
#include "../utils/lccnvalidator.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDomDocument>

namespace {
  static const char* HATHITRUST_QUERY_URL = "http://catalog.hathitrust.org/api/volumes/full/json/";
}

using namespace Tellico;
using Tellico::Fetch::HathiTrustFetcher;

HathiTrustFetcher::HathiTrustFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false), m_MARC21XMLHandler(nullptr), m_MODSHandler(nullptr) {
}

HathiTrustFetcher::~HathiTrustFetcher() {
  delete m_MARC21XMLHandler;
  m_MARC21XMLHandler = nullptr;
  delete m_MODSHandler;
  m_MODSHandler = nullptr;
}

QString HathiTrustFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool HathiTrustFetcher::canSearch(Fetch::FetchKey k) const {
  return k == ISBN || k == LCCN;
}

bool HathiTrustFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void HathiTrustFetcher::readConfigHook(const KConfigGroup&) {
}

void HathiTrustFetcher::search() {
  m_started = true;
  doSearch();
}

void HathiTrustFetcher::doSearch() {
  if(request().key() != ISBN && request().key() != LCCN) {
    stop();
    return;
  }

  QUrl u(QString::fromLatin1(HATHITRUST_QUERY_URL));

  QStringList searchValues;
  // we split ISBN and LCCN values, which are the only ones we accept
  const QStringList searchTerms = FieldFormat::splitValue(request().value());
  foreach(const QString& searchTerm, searchTerms) {
    if(request().key() == ISBN) {
      searchValues += QStringLiteral("isbn:%1").arg(ISBNValidator::cleanValue(searchTerm));
    } else {
      searchValues += QStringLiteral("lccn:%1").arg(LCCNValidator::formalize(searchTerm));
    }
  }
  u.setPath(u.path() + searchValues.join(QLatin1String("|")));

//  myDebug() << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &HathiTrustFetcher::slotComplete);
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

  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("MARC21slim2MODS3.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate MARC21slim2MODS3.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_MARC21XMLHandler = new XSLTHandler(u);
  if(!m_MARC21XMLHandler->isValid()) {
    myWarning() << "error in MARC21slim2MODS3.xsl.";
    delete m_MARC21XMLHandler;
    m_MARC21XMLHandler = nullptr;
    return false;
  }
  return true;
}

bool HathiTrustFetcher::initMODSHandler() {
  if(m_MODSHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate mods2tellico.xsl.";
    return false;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  m_MODSHandler = new XSLTHandler(u);
  if(!m_MODSHandler->isValid()) {
    myWarning() << "error in mods2tellico.xsl.";
    delete m_MODSHandler;
    m_MODSHandler = nullptr;
    // no use in keeping the MARC handlers now
    delete m_MARC21XMLHandler;
    m_MARC21XMLHandler = nullptr;
    return false;
  }
  return true;
}

Tellico::Data::EntryPtr HathiTrustFetcher::fetchEntryHook(uint uid_) {
  return m_entries.value(uid_);
}

Tellico::Fetch::FetchRequest HathiTrustFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  const QString lccn = entry_->field(QStringLiteral("lccn"));
  if(!lccn.isEmpty()) {
    return FetchRequest(LCCN, lccn);
  }
  return FetchRequest();
}

void HathiTrustFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(!initMARC21Handler() || !initMODSHandler()) {
    // debug messages are taken care of in the specific methods
    stop();
    return;
  }

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
  myWarning() << "Remove debug from hathitrustfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap resultMap = doc.object().toVariantMap();
  if(resultMap.isEmpty()) {
    myDebug() << "no results";
    stop();
    return;
  }

  QVariantMap::const_iterator i = resultMap.constBegin();
  for( ; i != resultMap.constEnd(); ++i) {
    const QVariantMap recordMap = i.value().toMap().value(QStringLiteral("records")).toMap();
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
    QString marcxml = ri.value().toMap().value(QStringLiteral("marc-xml")).toString();
    // HathiTrust doesn't always include the XML NS in the JSON results. Assume it's always
    // MARC XML and check that
    QDomDocument dom;
    if(dom.setContent(marcxml, true /* namespace processing */) && dom.documentElement().namespaceURI().isEmpty()) {
      const QString rootName = dom.documentElement().tagName();
      myDebug() << "no namespace, attempting to set on" << rootName << "element";
      QRegularExpression rootRx(QLatin1Char('<') + rootName + QLatin1Char('>'));
      QString newRoot = QLatin1Char('<') + rootName + QLatin1String(" xmlns=\"http://www.loc.gov/MARC21/slim\">");
      marcxml.replace(rootRx, newRoot);
    }
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
    QHashIterator<QString, QString> i2(allOptionalFields());
    while(i2.hasNext()) {
      i2.next();
      Data::FieldPtr field = coll->fieldByName(i2.key());
      if(field) {
        field->setTitle(i2.value());
        coll->modifyField(field);
      }
    }

    foreach(Data::EntryPtr entry, coll->entries()) {
      FetchResult* r = new FetchResult(this, entry);
      m_entries.insert(r->uid, entry);
      emit signalResultFound(r);
    }
  }

  m_hasMoreResults = false; // for now, no continued searches
  stop();
}

Tellico::Fetch::ConfigWidget* HathiTrustFetcher::configWidget(QWidget* parent_) const {
  return new HathiTrustFetcher::ConfigWidget(parent_, this);
}

QString HathiTrustFetcher::defaultName() {
  return QStringLiteral("HathiTrust"); // no translation
}

QString HathiTrustFetcher::defaultIcon() {
  return favIcon("http://www.hathitrust.org");
}

Tellico::StringHash HathiTrustFetcher::allOptionalFields() {
  // same ones as z3950fetcher
  StringHash hash;
  hash[QStringLiteral("address")]  = i18n("Address");
  hash[QStringLiteral("abstract")] = i18n("Abstract");
  hash[QStringLiteral("illustrator")] = i18n("Illustrator");
  hash[QStringLiteral("dewey")] = i18nc("Dewey Decimal classification system", "Dewey Decimal");
  hash[QStringLiteral("lcc")] = i18nc("Library of Congress classification system", "LoC Classification");
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
