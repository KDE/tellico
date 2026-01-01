/***************************************************************************
    Copyright (C) 2006-2020 Robby Stephenson <robby@periapsis.org>
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
#include "../utils/objvalue.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KConfigGroup>
#include <KJobWidgets>

#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
  static const int ISBNDB_MAX_RETURNS_TOTAL = 25;
  static const char* ISBNDB_BASE_URL = "https://api2.isbndb.com";
}

using namespace Tellico;
using Tellico::Fetch::ISBNdbFetcher;
using namespace Qt::Literals::StringLiterals;

ISBNdbFetcher::ISBNdbFetcher(QObject* parent_)
    : Fetcher(parent_),
      m_limit(ISBNDB_MAX_RETURNS_TOTAL),
      m_total(-1),
      m_numResults(0),
      m_started(false),
      m_batchIsbn(false) {
}

ISBNdbFetcher::~ISBNdbFetcher() {
}

QString ISBNdbFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool ISBNdbFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

bool ISBNdbFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == ISBN || k == Keyword;
}

void ISBNdbFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  m_batchIsbn = config_.readEntry("Batch ISBN", false);
}

void ISBNdbFetcher::search() {
  m_started = true;
  m_total = -1;
  m_numResults = 0;

  // we only split ISBN when not doing batch searching
  QStringList searchTerms;
  if(request().key() == ISBN && !m_batchIsbn) {
    searchTerms = FieldFormat::splitValue(request().value());
  } else  {
    searchTerms += request().value();
  }
  foreach(const QString& searchTerm, searchTerms) {
    doSearch(searchTerm);
  }
  if(m_jobs.isEmpty()) {
    stop();
  }
}

void ISBNdbFetcher::continueSearch() {
  m_started = true;

  doSearch(request().value());
}

void ISBNdbFetcher::doSearch(const QString& term_) {
  const bool multipleIsbn = request().key() == ISBN && term_.contains(QLatin1Char(';'));

  QUrl u(QString::fromLatin1(ISBNDB_BASE_URL));
  switch(request().key()) {
    case Title:
      u.setPath(QStringLiteral("/books/") + term_);
      break;

    case Person:
      // the /books/query search endpoint seems to not work with the author column yet [2020-09-02]
      // so continue to user /author/query search (which may not return all the same info)
      u.setPath(QStringLiteral("/author/") + term_);
      break;

    case ISBN:
      if(multipleIsbn) {
        u.setPath(QStringLiteral("/books"));
      } else {
        u.setPath(QStringLiteral("/book/"));
        // can only grab first value
        QString v = term_.section(QLatin1Char(';'), 0);
        v.remove(QLatin1Char('-'));
        u.setPath(u.path() + v);
      }
      break;

    case Keyword:
      // the /books/query search endpoint seems to not work with the author column yet [2020-09-02]
      // so continue to user /author/query search (which may not return all the same info)
      u.setPath(QStringLiteral("/books/") + term_);
      {
         QUrlQuery q;
         q.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
         q.addQueryItem(QStringLiteral("pageSize"), QString::number(ISBNDB_MAX_RETURNS_TOTAL));
         // disable beta searching
         q.addQueryItem(QStringLiteral("beta"), QStringLiteral("0"));
         u.setQuery(q);
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }

  if(m_testUrl1.isEmpty() && m_apiKey.isEmpty()) {
    myDebug() << source() << "- empty API key";
    message(i18n("An access key is required to use this data source.")
            + QLatin1Char(' ') +
            i18n("Those values must be entered in the data source settings."), MessageHandler::Error);
    stop();
    return;
  }
  if(!m_testUrl1.isEmpty()) u = m_testUrl1;

//  myDebug() << "url: " << u.url();

  QPointer<KIO::StoredTransferJob> job;
  if(multipleIsbn) {
    QString postData = request().value();
    postData = postData.replace(QLatin1Char(';'), QLatin1Char(','))
                       .remove(QLatin1Char('-'))
                       .remove(QLatin1Char(' '));
    postData.prepend(QStringLiteral("isbns="));
//    myDebug() << "posting" << postData;
    job = KIO::storedHttpPost(postData.toUtf8(), u, KIO::HideProgressInfo);
  } else {
    job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  }

  job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("Authorization: ") + m_apiKey);
  job->addMetaData(QStringLiteral("content-type"), QStringLiteral("application/json"));
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  connect(job.data(), &KJob::result, this, &ISBNdbFetcher::slotComplete);
  m_jobs << job;
}

void ISBNdbFetcher::endJob(KIO::StoredTransferJob* job_) {
  m_jobs.removeAll(job_);
  if(m_jobs.isEmpty())  {
    stop();
  }
}

void ISBNdbFetcher::stop() {
  if(!m_started) {
    return;
  }
  foreach(QPointer<KIO::StoredTransferJob> job, m_jobs) {
    if(job) {
      job->kill();
    }
  }
  m_jobs.clear();
  m_started = false;
  Q_EMIT signalDone(this);
}

void ISBNdbFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    endJob(job);
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    endJob(job);
    return;
  }

#if 0
  myWarning() << "Remove debug from isbndbfetcher.cpp";
  QFile file(QString::fromLatin1("/tmp/test-isbndb.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t << data;
  }
  file.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const auto obj = doc.object();
  QJsonArray resultList;
  if(obj.contains("book"_L1)) {
    resultList += obj.value("book"_L1);
    m_total = 1;
  } else if(obj.contains("books"_L1)) {
    m_total = obj["total"_L1].toInt();
    resultList = obj["books"_L1].toArray();
  } else if(obj.contains("data"_L1)) {
    m_total = obj["total"_L1].toInt();
    resultList = obj["data"_L1].toArray();
  } else {
    QString msg = objValue(obj, "message");
    if(msg.isEmpty()) msg = objValue(obj, "errorMessage");
    myDebug() << "no results from ISBNDBFetcher:" << msg;
    message(msg, MessageHandler::Error);
    endJob(job);
    return;
  }
//  myDebug() << "Total:" << m_total;

  Data::CollPtr coll(new Data::BookCollection(true));

  int count = 0;
  for(const auto& result : std::as_const(resultList)) {
//    myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toObject());

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
    ++count;
    ++m_numResults;
    if(count >= m_limit) {
      break;
    }
  }

  endJob(job);
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

void ISBNdbFetcher::populateEntry(Data::EntryPtr entry_, const QJsonObject& obj_) {
  static const QRegularExpression nonDigits(QStringLiteral("[^\\d]"));
  entry_->setField(QStringLiteral("title"), objValue(obj_, "title"));
  entry_->setField(QStringLiteral("isbn"), objValue(obj_, "isbn"));
  // "date_published" can be "2008-12-13" or "July 2012"
  QString pubYear = objValue(obj_, "date_published").remove(nonDigits).left(4);
  entry_->setField(QStringLiteral("pub_year"), pubYear);
  entry_->setField(QStringLiteral("author"), objValue(obj_, "authors"));
  entry_->setField(QStringLiteral("publisher"), objValue(obj_, "publisher"));
  entry_->setField(QStringLiteral("edition"), objValue(obj_, "edition"));
  QString binding = objValue(obj_, "binding");
  if(binding.isEmpty()) {
    binding = objValue(obj_, "format");
  }
  if(binding.startsWith(QLatin1StringView("Hardcover"))) {
    binding = QStringLiteral("Hardback");
  } else if(binding.startsWith(QLatin1StringView("Paperback"))) {
    binding = QStringLiteral("Paperback");
  }
  if(!binding.isEmpty()) {
    entry_->setField(QStringLiteral("binding"), i18n(binding.toUtf8().constData()));
  }
  entry_->setField(QStringLiteral("genre"), objValue(obj_, "subjects"));
  entry_->setField(QStringLiteral("cover"), objValue(obj_, "image"));
  entry_->setField(QStringLiteral("pages"), objValue(obj_, "pages"));
  entry_->setField(QStringLiteral("language"), objValue(obj_, "language"));
  const QString plotName(QStringLiteral("plot"));
  entry_->setField(plotName, objValue(obj_, "overview"));
  if(entry_->field(plotName).isEmpty()) {
    entry_->setField(plotName, objValue(obj_, "synopsis"));
  }

  const QString dewey = objValue(obj_, "dewey_decimal");
  if(!dewey.isEmpty() && optionalFields().contains(QStringLiteral("dewey"))) {
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
  return favIcon("https://isbndb.com/sites/default/files/favicon_0.ico");
}

Tellico::StringHash ISBNdbFetcher::allOptionalFields() {
  // same ones as z3950fetcher
  StringHash hash;
  hash[QStringLiteral("dewey")] = i18nc("Dewey Decimal classification system", "Dewey Decimal");
  return hash;
}

ISBNdbFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ISBNdbFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QStringLiteral("https://isbndb.com/isbn-database")),
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

  m_enableBatchIsbn = new QCheckBox(i18n("Enable batch ISBN searching (requires Premium or Pro plan)"), optionsWidget());
  connect(m_enableBatchIsbn, &QAbstractButton::clicked, this, &ConfigWidget::slotSetModified);
  ++row;
  l->addWidget(m_enableBatchIsbn, row, 0, 1, 2);
  QString w = i18n("Batch searching for ISBN values is faster but only available for Premium or Pro plans.");
  m_enableBatchIsbn->setWhatsThis(w);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
    m_enableBatchIsbn->setChecked(fetcher_->m_batchIsbn);
  } else { //defaults
    m_enableBatchIsbn->setChecked(false);
  }

  // now add additional fields widget
  addFieldsWidget(ISBNdbFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void ISBNdbFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
  config_.writeEntry("Batch ISBN", m_enableBatchIsbn->isChecked());
}

QString ISBNdbFetcher::ConfigWidget::preferredName() const {
  return ISBNdbFetcher::defaultName();
}
