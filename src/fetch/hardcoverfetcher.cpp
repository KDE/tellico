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

#include "hardcoverfetcher.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/tellico_utils.h"
#include "../utils/objvalue.h"
#include "../utils/isbnvalidator.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>
#include <KLanguageName>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QLineEdit>

namespace {
  static const int HARDCOVER_MAX_RETURNS_TOTAL = 20;
  static const char* HARDCOVER_API_URL = "https://api.hardcover.app/v1/graphql";
}

using namespace Tellico;
using Tellico::Fetch::HardcoverFetcher;
using namespace Qt::Literals::StringLiterals;

HardcoverFetcher::HardcoverFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

HardcoverFetcher::~HardcoverFetcher() = default;

QString HardcoverFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString HardcoverFetcher::attribution() const {
  return TC_I18N3(providedBy, QStringLiteral("https://hardcover.app"), defaultName());
}

bool HardcoverFetcher::canSearch(Fetch::FetchKey k) const {
  return k == ISBN;
}

bool HardcoverFetcher::canFetch(int type) const {
  return type == Data::Collection::Book;
}

void HardcoverFetcher::readConfigHook(const KConfigGroup& config_) {
  m_apiKey = config_.readEntry("API Key");
}

void HardcoverFetcher::search() {
  continueSearch();
}

void HardcoverFetcher::continueSearch() {
  m_started = true;

  if(m_apiKey.isEmpty()) {
    myDebug() << source() << "- empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  QString operationName, query;
  QJsonObject vars;
  switch(request().key()) {
    case ISBN:
      operationName = QStringLiteral("GetBookByISBN");
      query = isbnQuery();
      {
        QJsonArray isbns;
        const auto valueList = FieldFormat::splitValue(request().value());
        for(auto value : valueList) {
          value.remove(QLatin1Char('-'));
          isbns += value;
        }
        vars.insert(QLatin1String("isbns"), isbns);
      }
      break;

    case Raw:
    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }

  QJsonObject payload;
  payload.insert(QLatin1String("operationName"), operationName);
  payload.insert(QLatin1String("query"), query);
  payload.insert(QLatin1String("variables"), vars);

  m_job = KIO::storedHttpPost(QJsonDocument(payload).toJson(),
                              QUrl(QString::fromLatin1(HARDCOVER_API_URL)),
                              KIO::HideProgressInfo);
  configureJob(m_job);
  connect(m_job.data(), &KJob::result, this, &HardcoverFetcher::slotComplete);
}

void HardcoverFetcher::stop() {
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

Tellico::Fetch::FetchRequest HardcoverFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  return FetchRequest();
}

void HardcoverFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "Hardcover: no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 1
  myWarning() << "Remove debug from hardcoverfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-hardcover.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::BookCollection(true));
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if(doc.isNull()) {
    myDebug() << job->errorString();
    message(job->errorString(), MessageHandler::Error);
    stop();
  }
  const auto docObj = doc.object();
  const auto results = docObj["data"_L1]["editions"_L1].toArray();
  const auto errors = docObj["errors"_L1].toArray();

  if(results.isEmpty()) {
    if(errors.isEmpty()) {
      myLog() << "No results";
    } else {
      QStringList msgList;
      for(const QJsonValue& error : errors) {
        msgList += error["message"_L1].toString();
        myDebug() << error["message"_L1].toString();
      }
      if(!msgList.isEmpty()) {
        message(msgList.join("\n"_L1), MessageHandler::Error);
      }
    }
    stop();
    return;
  }

  int count = 0;
  for(const QJsonValue& result : results) {
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toObject());

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
    ++count;
    if(count >= HARDCOVER_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

Tellico::Data::EntryPtr HardcoverFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  const QString isbnName = QStringLiteral("isbn");
  QString isbn = entry->field(isbnName);
  ISBNValidator val;
  val.fixup(isbn);
  entry->setField(isbnName, isbn);

  // image might still be a URL
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }
  return entry;
}

void HardcoverFetcher::populateEntry(Data::EntryPtr entry_, const QJsonObject& obj_) {
  entry_->setField(QStringLiteral("title"), objValue(obj_, "title"));
  entry_->setField(QStringLiteral("subtitle"), objValue(obj_, "subtitle"));
  entry_->setField(QStringLiteral("pub_year"), objValue(obj_, "release_date").left(4));
  entry_->setField(QStringLiteral("pages"), objValue(obj_, "pages"));
  entry_->setField(QStringLiteral("language"), objValue(obj_, "language", "language"));
  QString binding = objValue(obj_, "edition_format");
  if(binding.contains(QLatin1StringView("Paperback"))) {
    binding = i18n("Paperback");
  } else if(binding == QLatin1StringView("Hardcover")) {
    binding = i18n("Hardback");
  } else {
    binding = i18n(binding.toUtf8().constData());
  }
  entry_->setField(QStringLiteral("binding"), binding);
  entry_->setField(QStringLiteral("publisher"), objValue(obj_, "publisher", "name"));

  QString isbn = objValue(obj_, "isbn_10");
  if(isbn.isEmpty()) {
    isbn = objValue(obj_, "isbn_13");
  }
  entry_->setField(QStringLiteral("isbn"), isbn);

  QString cover = objValue(obj_, "image", "url");
  if(cover.isEmpty()) {
    cover = objValue(obj_, "book", "image", "url");
  }
  entry_->setField(QStringLiteral("cover"), cover);

  QStringList authors;
  auto list = obj_["book"_L1]["contributions"_L1].toArray();
  for(const auto& person: std::as_const(list)) {
    auto obj = person.toObject();
    const auto role = objValue(obj, "contribution");
    if(role.isEmpty() || role == "Author"_L1) {
      authors += objValue(obj, "author", "name");
    }
  }
  entry_->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));

  entry_->setField(QStringLiteral("comments"), objValue(obj_, "book", "description"));
}

void HardcoverFetcher::configureJob(QPointer<KIO::StoredTransferJob> job_) {
  KJobWidgets::setWindow(job_, GUI::Proxy::widget());
  job_->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  job_->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: Bearer ") + m_apiKey);
  Tellico::addUserAgent(job_);
}

Tellico::Fetch::ConfigWidget* HardcoverFetcher::configWidget(QWidget* parent_) const {
  return new HardcoverFetcher::ConfigWidget(parent_, this);
}

QString HardcoverFetcher::defaultName() {
  return QStringLiteral("Hardcover");
}

QString HardcoverFetcher::defaultIcon() {
  return favIcon("https://hardcover.app");
}

Tellico::StringHash HardcoverFetcher::allOptionalFields() {
  StringHash hash;
  return hash;
}

HardcoverFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const HardcoverFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                               QLatin1String("https://hardcover.app/account/api/keys/new?scope=read%3Acatalog")),
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
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(HardcoverFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }
}

QString HardcoverFetcher::ConfigWidget::preferredName() const {
  return HardcoverFetcher::defaultName();
}

void HardcoverFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}
