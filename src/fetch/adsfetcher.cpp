/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "adsfetcher.h"
#include "../translators/risimporter.h"
#include "../entry.h"
#include "../utils/string_utils.h"
#include "../utils/guiproxy.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>
#include <QFile>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace {
  static const int ADS_RETURNS_PER_REQUEST = 20;
  static const char* ADS_BASE_URL = "https://api.adsabs.harvard.edu/v1";
  static const char* ADS_API_KEY = "7b374b31c4b297d969245069dea91a517c35a3e899e16406de98d09271347645c0a86553c7b4f395de9299f0640f2d490f673c09c0aacabc8efa743e3e5b3e74d8eb2c753f6c4708132abcf1492cfb8f";
}

using namespace Tellico;
using Tellico::Fetch::ADSFetcher;

ADSFetcher::ADSFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_job(nullptr), m_started(false), m_start(0), m_total(-1) {
  m_apiKey = Tellico::reverseObfuscate(ADS_API_KEY);
}

ADSFetcher::~ADSFetcher() {
}

QString ADSFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ADSFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == Keyword || k == DOI;
}

bool ADSFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void ADSFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    // the API key used to be saved in the config
    // now in API v4, the API Key is unique to the application and the API PIN is user-specific
    // the name of the config option was kept the same
    m_apiKey = k;
  }
}

void ADSFetcher::search() {
  m_started = true;
  m_start = 0;
  m_total = -1;
  doSearch();
}

void ADSFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void ADSFetcher::doSearch() {
  QUrl u(QString::fromLatin1(ADS_BASE_URL));
  u.setPath(u.path() + QLatin1String("/search/query"));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("row"), QString::number(ADS_RETURNS_PER_REQUEST));
  q.addQueryItem(QStringLiteral("start"), QString::number(m_start));
  q.addQueryItem(QStringLiteral("fl"), QLatin1String("bibcode,first_author,title,year"));

  auto queryValue = QUrl::toPercentEncoding(request().value());
  if(!queryValue.startsWith('"')) {
    queryValue.prepend('"');
  }
  if(!queryValue.endsWith('"')) {
    queryValue.append('"');
  }
  const QString value = QString::fromUtf8(queryValue);
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("title:%1").arg(value));
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("q"), value);
      break;

    case Person:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("author:%1").arg(value));
      break;

    case DOI:
      q.addQueryItem(QStringLiteral("q"), QStringLiteral("doi:%1").arg(value));
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);
//  myDebug() << "search url: " << u.url();

  m_job = getJob(u);
  connect(m_job.data(), &KJob::result,
          this, &ADSFetcher::slotComplete);
}

void ADSFetcher::stop() {
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

void ADSFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "ADS - no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from adsfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-ads.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << QString::fromUtf8(data.constData(), data.size());
  }
  f.close();
#endif

  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if(doc.isNull()) {
    myDebug() << "null JSON document:" << jsonError.errorString();
    message(jsonError.errorString(), MessageHandler::Error);
    stop();
    return;
  }
  const auto obj = doc.object();
  const auto response = obj.value(QLatin1String("response")).toObject();
  if(response.isEmpty()) {
    const auto errorMsg = obj[QLatin1String("error")][QLatin1String("msg")].toString();
    myDebug() << "No response:" << errorMsg;
    message(errorMsg, MessageHandler::Error);
    stop();
    return;
  }
  m_total = response.value(QLatin1String("numFound")).toInt();

  QJsonArray results = response.value(QLatin1String("docs")).toArray();
  for(QJsonArray::const_iterator i = results.constBegin(); i != results.constEnd(); ++i) {
    if(!m_started) {
      // might get aborted
      break;
    }
    QJsonObject result = (*i).toObject();
    const QString title = result.value(QLatin1String("title")).toArray().at(0).toString();
    FetchResult* r = new FetchResult(this,
                                     title,
                                     result.value(QLatin1String("first_author")).toString()
                                     + QLatin1Char('/')
                                     + result.value(QLatin1String("year")).toString());
    m_results.insert(r->uid, result.value(QLatin1String("bibcode")).toString());
    emit signalResultFound(r);
//    myDebug() << "found" << title;
  }
  m_start = m_results.count();
  m_hasMoreResults = (m_start > 0 && m_start <= m_total);
//  myDebug() << "start:" << m_start << "; total:" << m_total;

  stop(); // required
}

Tellico::Data::EntryPtr ADSFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(entry) {
    return entry;
  }

  if(!m_results.contains(uid_)) {
    myWarning() << "no matching bibcode";
    return Data::EntryPtr();
  }

  QUrl u(QString::fromLatin1(ADS_BASE_URL));
  // prefer RIS to Bibtex since Tellico isn't always compiled with Bibtex support
  u.setPath(u.path() + QLatin1String("/export/ris"));
  QJsonArray codes;
  codes += m_results.value(uid_);
  QJsonObject obj;
  obj.insert(QLatin1String("bibcode"), codes);
  const QByteArray payload = QJsonDocument(obj).toJson();

  QPointer<KIO::StoredTransferJob> job = KIO::storedHttpPost(payload, u, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("content-type"), QStringLiteral("Content-Type: application/json"));
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: Bearer ") + m_apiKey);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  if(!job->exec()) {
    myDebug() << "ADS: export failure";
    myDebug() << job->errorString() << u;
    return Data::EntryPtr();
  }

  auto data = job->data();
#if 0
  myWarning() << "Remove debug2 from adsfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-ads-export.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << QString::fromUtf8(data.constData(), data.size());
  }
  f.close();
#endif

  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if(doc.isNull()) {
    myDebug() << "null JSON document:" << jsonError.errorString();
    message(jsonError.errorString(), MessageHandler::Error);
    return Data::EntryPtr();
  }
  Import::RISImporter imp(doc.object().value(QLatin1String("export")).toString());
  auto coll = imp.collection();

  if(coll->entryCount() == 0) {
    return Data::EntryPtr();
  }
  m_entries.insert(uid_, coll->entries().at(0));
  return coll->entries().at(0);
}

Tellico::Fetch::FetchRequest ADSFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString doi = entry_->field(QStringLiteral("doi"));
  if(!doi.isEmpty()) {
    return FetchRequest(DOI, doi);
  }
  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

QPointer<KIO::StoredTransferJob> ADSFetcher::getJob(const QUrl& url_) {
  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(url_, KIO::NoReload, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: Bearer ") + m_apiKey);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  return job;
}

Tellico::Fetch::ConfigWidget* ADSFetcher::configWidget(QWidget* parent_) const {
  return new ADSFetcher::ConfigWidget(parent_, this);
}

QString ADSFetcher::defaultName() {
  return i18n("SAO/NASA Astrophysics Data System");
}

QString ADSFetcher::defaultIcon() {
  return favIcon("https://ui.adsabs.harvard.edu");
}

ADSFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ADSFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("https://ui.adsabs.harvard.edu/user/settings/token")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, &QLineEdit::textChanged, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_apiKeyEdit, row, 1);
  QString w = i18n("The default Tellico key may be used, but searching may fail due to reaching access limits.");
  label->setWhatsThis(w);
  m_apiKeyEdit->setWhatsThis(w);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(ADSFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }
}

QString ADSFetcher::ConfigWidget::preferredName() const {
  return ADSFetcher::defaultName();
}

void ADSFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}
