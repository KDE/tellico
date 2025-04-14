/***************************************************************************
    Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>
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

#include "metronfetcher.h"
#include "../collections/comicbookcollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/objvalue.h"
#include "../utils/tellico_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJobWidgets>
#include <KJobUiDelegate>
#include <KIO/StoredTransferJob>
#include <KPasswordDialog>
#include <KIconLoader>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

using namespace Qt::Literals::StringLiterals;

namespace {
  static const int METRON_MAX_RETURNS_TOTAL = 20;
  static const char* METRON_API_URL = "https://metron.cloud/api";
}

using namespace Tellico;
using Tellico::Fetch::MetronFetcher;

MetronFetcher::MetronFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

MetronFetcher::~MetronFetcher() {
}

QString MetronFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool MetronFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Keyword;
}

bool MetronFetcher::canFetch(int type) const {
  return type == Data::Collection::ComicBook;
}

void MetronFetcher::readConfigHook(const KConfigGroup& config_) {
  // normally, the username and password is entered by the user and not saved in the config file
  // this approach is just for testing
  m_username = config_.readEntry("Username");
  m_password = config_.readEntry("Password");
}

void MetronFetcher::saveConfigHook(KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void MetronFetcher::search() {
  m_page = 0;
  m_started = true;
  if(m_auth.isEmpty() && !getAuthorization()) {
    stop();
    return;
  }
  continueSearch();
}

void MetronFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(METRON_API_URL));
  u.setPath(u.path() + QLatin1String("/issue/"));
  QUrlQuery q;
  switch(request().key()) {
    case Keyword:
      {
        // extract possible issue number
        QString issue;
        auto value = request().value();
        static const QRegularExpression issueRx(" #(\\d+)$"_L1);
        auto match = issueRx.match(value);
        if(match.hasMatch()) {
          value = value.remove(issueRx);
          issue = match.captured(1);
        }
        q.addQueryItem(u"series_name"_s, value);
        if(!issue.isEmpty()) {
          q.addQueryItem(u"number"_s, issue);
        }
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  q.addQueryItem(u"page"_s, QString::number(++m_page));
  u.setQuery(q);

  myLog() << "Reading" << u.toDisplayString();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  QStringList customHeaders;
  customHeaders += (u"Authorization: Basic "_s + m_auth);
  m_job->addMetaData("customHTTPHeader"_L1, customHeaders.join(QLatin1String("\r\n")));
  m_job->addMetaData("accept"_L1, QStringLiteral("application/json"));
  Tellico::addUserAgent(m_job);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &MetronFetcher::slotComplete);
}

void MetronFetcher::stop() {
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

Tellico::Fetch::FetchRequest MetronFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString series = entry_->field(u"series"_s);
  const QString issue = entry_->field(u"issue"_s);
  if(!series.isEmpty()) {
    return FetchRequest(Keyword, series + " #"_L1 + issue);
  }

  return FetchRequest();
}

void MetronFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  const auto code = job->queryMetaData("responsecode"_L1);
//  myDebug() << "Metron response code:" << code;
  if(code == "401"_L1) {
    myLog() << "Invalid login for Metron";
    m_auth.clear();
    if(getAuthorization()) {
      // try again
      stop();
      continueSearch();
      return;
    }
  }

  if(code == "429"_L1) {
    myLog() << "Requests exceeded rate limit for Metron (30 calls/min or 10,000 calls/day)";
    message(i18n("The rate limit has been exceeded."), MessageHandler::Error);
    stop();
    return;
  }

  if(job->error()) {
    myDebug() << "Response code:" << code;
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "No data, response code:" << code;
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from metronfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-metron.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if(doc.isNull()) {
    myDebug() << "null JSON document, response code:" << code;
    stop();
    return;
  }
  const auto obj = doc.object();
  // check for error
  if(obj.contains("detail"_L1)) {
    const auto msg = objValue(obj, "detail");
    message(msg, MessageHandler::Error);
    myDebug() << "MetronFetcher -" << msg;
    stop();
    return;
  }

  const auto results = obj.value(QLatin1StringView("results")).toArray();
  if(results.isEmpty()) {
    myLog() << "No results";
    stop();
    return;
  }

  Data::CollPtr coll(new Data::ComicBookCollection(true));
  // placeholder for metron id, to be removed later
  Data::FieldPtr f1(new Data::Field(QStringLiteral("metron-id"), QString(), Data::Field::Number));
  coll->addField(f1);
  if(optionalFields().contains(QStringLiteral("metron"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("metron"), i18n("Metron Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("isbn"))) {
    Data::FieldPtr field = Data::Field::createDefaultField(Data::Field::IsbnField);
    coll->addField(field);
  }

  int count = 0;
  for(const QJsonValue& result : results) {
    Data::EntryPtr entry(new Data::Entry(coll));
    const auto obj = result.toObject();
    populateEntry(entry, obj);

    FetchResult* r = new FetchResult(this, entry);
    // special check
    if(r->title.isEmpty()) {
      r->title = objValue(obj, "issue");
    }
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
    ++count;
    if(count >= METRON_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

Tellico::Data::EntryPtr MetronFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // if there's no metron id, we've already pulled the details
  const QString metron_id = entry->field(QStringLiteral("metron-id"));
  if(metron_id.isEmpty() || m_auth.isEmpty()) { // auth should have already been set
    return entry;
  }

  QUrl u(QString::fromLatin1(METRON_API_URL));
  u.setPath(u.path() + QLatin1String("/issue/") + metron_id + QLatin1Char('/'));

  myLog() << "Reading" << u.toDisplayString();
  auto job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  QStringList customHeaders;
  customHeaders += (u"Authorization: Basic "_s + m_auth);
  job->addMetaData("customHTTPHeader"_L1, customHeaders.join(QLatin1String("\r\n")));
  Tellico::addUserAgent(job);
  if(!job->exec()) {
    myDebug() << "Failed to load" << u;
    return entry;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "No data";
    return entry;
  }
#if 0
  myWarning() << "Remove debug from metronfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-metron-detailed.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if(doc.isNull()) {
    myDebug() << "null JSON document";
    return entry;
  }
  populateEntry(entry, doc.object());

  // image might still be a URL
  const QString cover(QStringLiteral("cover"));
  const QString image_id = entry->field(cover);
  if(image_id.contains(QLatin1Char('/'))) {
    const QUrl imageUrl = QUrl::fromUserInput(image_id);
    // use base url as referrer
    const QString id = ImageFactory::addImage(imageUrl, true /* quiet */, imageUrl.adjusted(QUrl::RemovePath));
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(cover, id);
  }

  // don't want to include ID field - absence indicates entry is fully populated
  entry->setField(QStringLiteral("metron-id"), QString());

  return entry;
}

void MetronFetcher::populateEntry(Data::EntryPtr entry_, const QJsonObject& obj_) {
  entry_->setField(QStringLiteral("metron-id"), objValue(obj_, "id"));
  QString title = objValue(obj_, "title");
  if(title.isEmpty()) {
    title = objValue(obj_, "name");
  }
  entry_->setField(QStringLiteral("title"), title);
  entry_->setField(QStringLiteral("series"), objValue(obj_, "series", "name"));
  entry_->setField(QStringLiteral("publisher"), objValue(obj_, "publisher", "name"));
  entry_->setField(QStringLiteral("pub_year"), objValue(obj_, "cover_date").left(4));
  entry_->setField(QStringLiteral("cover"), objValue(obj_, "image"));
  entry_->setField(QStringLiteral("issue"), objValue(obj_, "number"));
  entry_->setField(QStringLiteral("genre"), objValue(obj_, "series", "genres", "name"));
  entry_->setField(QStringLiteral("pages"), objValue(obj_, "page"));
  entry_->setField(QStringLiteral("plot"), objValue(obj_, "desc"));

  const QString metron(QStringLiteral("metron"));
  if(optionalFields().contains(metron)) {
    entry_->setField(QStringLiteral("metron"), objValue(obj_, "resource_url"));
  }
  const QString isbn(QStringLiteral("isbn"));
  if(optionalFields().contains(isbn)) {
    entry_->setField(isbn, objValue(obj_, "isbn"));
  }

  QStringList writers, artists;
  const auto peopleArray = obj_.value(QLatin1StringView("credits")).toArray();
  for(const QJsonValue& person : peopleArray) {
    const auto personObj = person.toObject();
    const auto roleArray = personObj[QLatin1StringView("role")].toArray();
    const QString role = roleArray.isEmpty() ? QString() : roleArray.first()[QLatin1StringView("name")].toString();
    if(role == QLatin1String("Writer")) {
      writers << objValue(personObj, "creator");
    } else if(role == QLatin1String("Inker") ||
              role == QLatin1String("Penciller") ||
              role == QLatin1String("Colorist")) {
      // Bedetheque source adds a separate colorist field, but go ahead and compine all artists
      artists << objValue(personObj, "creator");
    }
  }
  entry_->setField(QStringLiteral("writer"), writers.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("artist"), artists.join(FieldFormat::delimiterString()));
}

bool MetronFetcher::getAuthorization() {
  QString username, password;

  if(!m_password.isEmpty()) {
    username = m_username;
    password = m_password;
  } else if(GUI::Proxy::widget()) {
    KPasswordDialog dlg(GUI::Proxy::widget(), KPasswordDialog::ShowUsernameLine);
    dlg.setPrompt(i18n("Please enter your Metron username and password"));
    // TODO: use local copy of logo?
    auto icon = KIconLoader::global()->loadIcon(defaultIcon(), KIconLoader::Dialog, 0);
    dlg.setIcon(icon);
    if(!dlg.exec()) {
      message(i18n("Registration is required to use this data source."), MessageHandler::Error);
      return false;
    }
    username = dlg.username();
    password = dlg.password();
  } else {
    myDebug() << "No widget or password for metron fetcher";
    return false;
  }
  const auto authString = QStringLiteral("%1:%2").arg(username, password);
  m_auth = QString::fromLatin1(authString.toLatin1().toBase64(QByteArray::OmitTrailingEquals));
  return true;
}

Tellico::Fetch::ConfigWidget* MetronFetcher::configWidget(QWidget* parent_) const {
  return new MetronFetcher::ConfigWidget(parent_, this);
}

QString MetronFetcher::defaultName() {
  return QStringLiteral("Metron");
}

QString MetronFetcher::defaultIcon() {
  return favIcon(QUrl("https://metron.cloud"_L1),
                 QUrl("https://static.metron.cloud/static/site/img/favicon.ico"_L1));
}

Tellico::StringHash MetronFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("metron")] = i18n("Metron Link");
  hash[QStringLiteral("isbn")]   = i18n("ISBN#");
  return hash;
}

MetronFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const MetronFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(MetronFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString MetronFetcher::ConfigWidget::preferredName() const {
  return MetronFetcher::defaultName();
}
