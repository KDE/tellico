/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "isbndbfetcher.h"
#include "../collections/bookcollection.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KConfigGroup>
#include <KJobWidgets/KJobWidgets>

#include <QLineEdit>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const int ISBNDB_MAX_RETURNS_TOTAL = 25;
  static const char* ISBNDB_BASE_URL = "https://api.isbndb.com";
}

using namespace Tellico;
using Tellico::Fetch::ISBNdbFetcher;

ISBNdbFetcher::ISBNdbFetcher(QObject* parent_)
    : Fetcher(parent_),
      m_limit(ISBNDB_MAX_RETURNS_TOTAL), m_total(-1), m_numResults(0),
      m_job(nullptr), m_started(false) {
}

ISBNdbFetcher::~ISBNdbFetcher() {
}

QString ISBNdbFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ISBNdbFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void ISBNdbFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void ISBNdbFetcher::search() {
  m_started = true;
  m_total = -1;
  m_numResults = 0;

  doSearch();
}

void ISBNdbFetcher::continueSearch() {
  m_started = true;

  if(m_apiKey.isEmpty()) {
    myDebug() << "empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }

  doSearch();
}

void ISBNdbFetcher::doSearch() {
//  myDebug() << "value = " << value_;

  QUrl u(QString::fromLatin1(ISBNDB_BASE_URL));
  switch(request().key) {
    case Title:
      u.setPath(QLatin1String("/books/") + request().value);
      break;

    case Person:
      u.setPath(QLatin1String("/author/") + request().value);
      break;

    case ISBN:
      u.setPath(QLatin1String("/book/"));
      {
        // can only grab first value
        QString v = request().value.section(QLatin1Char(';'), 0);
        v.remove(QLatin1Char('-'));
        u.setPath(u.path() + v);
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      stop();
      return;
  }
//  myDebug() << "url: " << u.url();

  m_job = isbndbJob(u, m_apiKey);
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void ISBNdbFetcher::stop() {
  if(!m_started) {
    return;
  }
//  myDebug();
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }

  m_started = false;
  emit signalDone(this);
}

void ISBNdbFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from isbndbfetcher.cpp";
  QFile file(QString::fromLatin1("/tmp/test.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t.setCodec("UTF-8");
    t << data;
  }
  file.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap result = doc.object().toVariantMap();
  QVariantList resultList;
  if(result.contains(QStringLiteral("book"))) {
    resultList += result.value(QStringLiteral("book"));
    m_total = 1;
  } else if(result.contains(QStringLiteral("books"))) {
    m_total = result.value(QStringLiteral("total")).toInt();
    resultList = result.value(QStringLiteral("books")).toList();
  } else {
    const QString msg = result.value(QStringLiteral("message")).toString();
    myDebug() << "no results:" << msg;
    stop();
    return;
  }
//  myDebug() << "Total:" << m_total;

  Data::CollPtr coll(new Data::BookCollection(true));

  int count = 0;
  foreach(const QVariant& result, resultList) {
//    myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
    ++m_numResults;
    if(count >= m_limit) {
      break;
    }
  }

  stop(); // required
}

Tellico::Data::EntryPtr ISBNdbFetcher::fetchEntryHook(uint uid_) {
  if(!m_entries.contains(uid_)) {
    myDebug() << "no entry ptr";
    return Data::EntryPtr();
  }

  Data::EntryPtr entry = m_entries.value(uid_);

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

Tellico::Fetch::FetchRequest ISBNdbFetcher::updateRequest(Data::EntryPtr entry_) {
  QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(Fetch::ISBN, isbn);
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

void ISBNdbFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_) {
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  entry_->setField(QStringLiteral("isbn"), mapValue(resultMap_, "isbn"));
  // "date_published" can be "2008-12-13" or "July 2012"
  QString pubYear = mapValue(resultMap_, "date_published").remove(QRegExp(QStringLiteral("[^\\d]"))).left(4);
  entry_->setField(QStringLiteral("pub_year"), pubYear);
  QStringList authors;
  foreach(const QVariant& author, resultMap_.value(QLatin1String("authors")).toList()) {
    authors += author.toString();
  }
  entry_->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("publisher"), mapValue(resultMap_, "publisher"));
  entry_->setField(QStringLiteral("edition"), mapValue(resultMap_, "edition"));
  QString binding = mapValue(resultMap_, "binding");
  if(binding.isEmpty()) {
    binding = mapValue(resultMap_, "format");
  }
  if(binding.startsWith(QStringLiteral("Hardcover"))) {
    binding = QStringLiteral("Hardback");
  } else if(binding.startsWith(QStringLiteral("Paperback"))) {
    binding = QStringLiteral("Paperback");
  }
  if(!binding.isEmpty()) {
    entry_->setField(QStringLiteral("binding"), i18n(binding.toUtf8().constData()));
  }
  QStringList subjects;
  foreach(const QVariant& subject, resultMap_.value(QLatin1String("subjects")).toList()) {
    subjects += subject.toString();
  }
  entry_->setField(QStringLiteral("genre"), subjects.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("cover"), mapValue(resultMap_, "image"));
  entry_->setField(QStringLiteral("pages"), mapValue(resultMap_, "pages"));
  entry_->setField(QStringLiteral("language"), mapValue(resultMap_, "language"));
  entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "overview"));
  if(mapValue(resultMap_, "overview").isEmpty()) {
    entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "synopsis"));
  }

  const QString dewey = mapValue(resultMap_, "dewey_decimal");
  if(!dewey.isEmpty() && optionalFields().contains(QLatin1String("dewey"))) {
    if(!entry_->collection()->hasField(QStringLiteral("dewey"))) {
      Data::FieldPtr field(new Data::Field(QStringLiteral("dewey"), i18n("Dewey Decimal"), Data::Field::Line));
      field->setCategory(i18n("Publishing"));
      entry_->collection()->addField(field);
    }
    entry_->setField(QStringLiteral("dewey"), dewey);
  }
}

Tellico::Fetch::ConfigWidget* ISBNdbFetcher::configWidget(QWidget* parent_) const {
  return new ISBNdbFetcher::ConfigWidget(parent_, this);
}

QString ISBNdbFetcher::defaultName() {
  return i18n("ISBNdb.com");
}

QString ISBNdbFetcher::defaultIcon() {
  return favIcon("http://isbndb.com");
}

Tellico::StringHash ISBNdbFetcher::allOptionalFields() {
  // same ones as z3950fetcher
  StringHash hash;
  hash[QStringLiteral("dewey")] = i18nc("Dewey Decimal classification system", "Dewey Decimal");
  return hash;
}

QPointer<KIO::StoredTransferJob> ISBNdbFetcher::isbndbJob(const QUrl& url_, const QString& apiKey_) {
  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(url_, KIO::NoReload, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("customHTTPHeader"), QLatin1String("X-API-Key: ") + apiKey_);
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  return job;
}

ISBNdbFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ISBNdbFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                preferredName(),
                                QLatin1String("https://isbndb.com/isbn-database")),
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
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }

  // now add additional fields widget
  addFieldsWidget(ISBNdbFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void ISBNdbFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString ISBNdbFetcher::ConfigWidget::preferredName() const {
  return ISBNdbFetcher::defaultName();
}
