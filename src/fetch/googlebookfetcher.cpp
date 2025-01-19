/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "googlebookfetcher.h"
#include "../collections/bookcollection.h"
#include "../entry.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/mapvalue.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>
#include <KConfigGroup>

#include <QLineEdit>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace {
  static const int GOOGLEBOOK_MAX_RETURNS = 20;
  static const char* GOOGLEBOOK_API_URL = "https://www.googleapis.com/books/v1/volumes";
  static const char* GOOGLEBOOK_API_KEY = "b0e1702513773b743b53b1c5566ea0f93e7c3b720351bad197f801491951d29afca54b32712ba6dc4e1e4c7a5b0ad99d9dedfdbab4f10642b7e821403340fc98692bcdb4dc8fd0b14339236ae4a5";
}

using namespace Tellico;
using Tellico::Fetch::GoogleBookFetcher;

GoogleBookFetcher::GoogleBookFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_start(0)
    , m_total(0) {
  m_apiKey = Tellico::reverseObfuscate(GOOGLEBOOK_API_KEY);
}

GoogleBookFetcher::~GoogleBookFetcher() {
}

QString GoogleBookFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool GoogleBookFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == ISBN || k == Keyword;
}

bool GoogleBookFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void GoogleBookFetcher::readConfigHook(const KConfigGroup& config_) {
  // allow an empty key if the config key does exist
  m_apiKey = config_.readEntry("API Key", GOOGLEBOOK_API_KEY);
}

void GoogleBookFetcher::search() {
  m_start = 0;
  m_total = -1;
  continueSearch();
}

void GoogleBookFetcher::continueSearch() {
  m_started = true;
  // we only split ISBN and LCCN values
  QStringList searchTerms;
  if(request().key() == ISBN) {
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

void GoogleBookFetcher::doSearch(const QString& term_) {
  QUrl u(QString::fromLatin1(GOOGLEBOOK_API_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("maxResults"), QString::number(GOOGLEBOOK_MAX_RETURNS));
  q.addQueryItem(QStringLiteral("startIndex"), QString::number(m_start));
  q.addQueryItem(QStringLiteral("printType"), QStringLiteral("books"));
  // we don't require a key, cause it might work without it
  if(!m_apiKey.isEmpty()) {
    q.addQueryItem(QStringLiteral("key"), m_apiKey);
  }

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("q"), QLatin1String("intitle:") + term_);
      break;

    case Person:
      // for people, go ahead and enclose in quotes
      // risk of missing middle initials, etc. balanced by google splitting front and last name
      q.addQueryItem(QStringLiteral("q"), QLatin1String("inauthor:\"") + term_ + QLatin1Char('"'));
      break;

    case ISBN:
      {
        const QString isbn = ISBNValidator::cleanValue(term_);
        q.addQueryItem(QStringLiteral("q"), QLatin1String("isbn:") + isbn);
      }
      break;

    case Keyword:
      q.addQueryItem(QStringLiteral("q"), term_);
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return;
  }
  u.setQuery(q);
//  myDebug() << "url:" << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  connect(job.data(), &KJob::result, this, &GoogleBookFetcher::slotComplete);
  m_jobs << job;
}

void GoogleBookFetcher::endJob(KIO::StoredTransferJob* job_) {
  m_jobs.removeOne(job_);
  if(m_jobs.isEmpty())  {
    stop();
  }
}

void GoogleBookFetcher::stop() {
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

Tellico::Data::EntryPtr GoogleBookFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString gbs = entry->field(QStringLiteral("gbs-link"));
  if(!gbs.isEmpty()) {
    // quiet
    QByteArray data = FileHandler::readDataFile(QUrl::fromUserInput(gbs), true);
    QJsonDocument doc = QJsonDocument::fromJson(data);
    populateEntry(entry, doc.object().toVariantMap());
  }

  const QString image_id = entry->field(QStringLiteral("cover"));
  // if it's still a url, we need to load it
  if(image_id.startsWith(QLatin1String("http"))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
      entry->setField(QStringLiteral("cover"), QString());
    } else {
      entry->setField(QStringLiteral("cover"), id);
    }
  }

  // don't want to include gbs json link
  entry->setField(QStringLiteral("gbs-link"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest GoogleBookFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void GoogleBookFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
//  myDebug();

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    endJob(job);
    return;
  }

  QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    endJob(job);
    return;
  }

#if 0
  myWarning() << "Remove debug from googlebookfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::BookCollection(true));
  // always add the gbs-link for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QStringLiteral("gbs-link"), QStringLiteral("GBS Link"), Data::Field::URL));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(!coll->hasField(QStringLiteral("googlebook")) && optionalFields().contains(QStringLiteral("googlebook"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("googlebook"), i18n("Google Book Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap result = doc.object().toVariantMap();
  m_total = result.value(QStringLiteral("totalItems")).toInt();
//  myDebug() << "total:" << m_total;

  QVariantList resultList = result.value(QStringLiteral("items")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    endJob(job);
    return;
  }

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }

  m_start = m_entries.count();
  m_hasMoreResults = request().key() != ISBN && m_start <= m_total;
  endJob(job);
}

void GoogleBookFetcher::populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap) {
  if(entry->collection()->hasField(QStringLiteral("gbs-link"))) {
    entry->setField(QStringLiteral("gbs-link"), mapValue(resultMap, "selfLink"));
  }

  const QVariantMap volumeMap = resultMap.value(QStringLiteral("volumeInfo")).toMap();
  entry->setField(QStringLiteral("title"),     mapValue(volumeMap, "title"));
  entry->setField(QStringLiteral("subtitle"),  mapValue(volumeMap, "subtitle"));
  entry->setField(QStringLiteral("pub_year"),  mapValue(volumeMap, "publishedDate").left(4));
  entry->setField(QStringLiteral("author"),    mapValue(volumeMap, "authors"));
  // workaround for bug, where publisher can be enclosed in quotes
  QString pub = mapValue(volumeMap, "publisher");
  if(pub.startsWith(QLatin1Char('"')) && pub.endsWith(QLatin1Char('"'))) {
    pub.chop(1);
    pub = pub.remove(0, 1);
  }
  entry->setField(QStringLiteral("publisher"), pub);
  entry->setField(QStringLiteral("pages"),     mapValue(volumeMap, "pageCount"));
  entry->setField(QStringLiteral("language"),  mapValue(volumeMap, "language"));
  entry->setField(QStringLiteral("comments"),  mapValue(volumeMap, "description"));

  const QStringList catList = volumeMap.value(QStringLiteral("categories")).toStringList();
  // google is going to give us a lot of categories
  static const QRegularExpression slash(QLatin1String("\\s*/\\s*"));
  QStringList cleanCategories;
  foreach(const QString& cat, catList) {
    // split them by the '/' character, too
    cleanCategories += cat.split(slash);
  }
  cleanCategories.sort();
  cleanCategories.removeDuplicates();
  // remove General since it's vague enough to not matter
  cleanCategories.removeOne(QStringLiteral("General"));
  entry->setField(QStringLiteral("keyword"), cleanCategories.join(FieldFormat::delimiterString()));

  QString isbn;
  foreach(const QVariant& idVariant, volumeMap.value(QLatin1String("industryIdentifiers")).toList()) {
    const QVariantMap idMap = idVariant.toMap();
    if(mapValue(idMap, "type") == QLatin1String("ISBN_10")) {
      isbn = mapValue(idMap, "identifier");
      break;
    } else if(mapValue(idMap, "type") == QLatin1String("ISBN_13")) {
      isbn = mapValue(idMap, "identifier");
      // allow isbn10 to override, so don't break here
    }
  }
  if(!isbn.isEmpty()) {
    ISBNValidator val(this);
    val.fixup(isbn);
    entry->setField(QStringLiteral("isbn"), isbn);
  }

  const QVariantMap imageMap = volumeMap.value(QStringLiteral("imageLinks")).toMap();
  if(imageMap.contains(QStringLiteral("small"))) {
    entry->setField(QStringLiteral("cover"), mapValue(imageMap, "small"));
  } else if(imageMap.contains(QStringLiteral("thumbnail"))) {
    entry->setField(QStringLiteral("cover"), mapValue(imageMap, "thumbnail"));
  } else if(imageMap.contains(QStringLiteral("smallThumbnail"))) {
    entry->setField(QStringLiteral("cover"), mapValue(imageMap, "smallThumbnail"));
  }

  if(optionalFields().contains(QStringLiteral("googlebook"))) {
    entry->setField(QStringLiteral("googlebook"), mapValue(volumeMap, "infoLink"));
  }
}

Tellico::Fetch::ConfigWidget* GoogleBookFetcher::configWidget(QWidget* parent_) const {
  return new GoogleBookFetcher::ConfigWidget(parent_, this);
}

QString GoogleBookFetcher::defaultName() {
  return i18n("Google Book Search");
}

QString GoogleBookFetcher::defaultIcon() {
  return favIcon("http://books.google.com");
}

Tellico::StringHash GoogleBookFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("googlebook")] = i18n("Google Book Link");
  return hash;
}

GoogleBookFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GoogleBookFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing this data source. "
                               "If you agree to the terms and conditions, <a href='%1'>sign "
                               "up for an account</a>, and enter your information below.",
                                QLatin1String("https://code.google.com/apis/console")),
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
  addFieldsWidget(GoogleBookFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_ && fetcher_->m_apiKey != QLatin1String(GOOGLEBOOK_API_KEY)) {
    // only show the key if it is not the default Tellico one...
    // that way the user is prompted to apply for their own
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }
}

void GoogleBookFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("API Key", apiKey);
  }
}

QString GoogleBookFetcher::ConfigWidget::preferredName() const {
  return GoogleBookFetcher::defaultName();
}
