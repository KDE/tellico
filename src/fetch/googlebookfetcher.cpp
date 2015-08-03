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
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <KJobWidgets/KJobWidgets>
#include <KConfigGroup>

#include <QLineEdit>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
  static const int GOOGLEBOOK_MAX_RETURNS = 20;
  static const char* GOOGLEBOOK_API_URL = "https://www.googleapis.com/books/v1/volumes";
  static const char* GOOGLEBOOK_API_KEY = "AIzaSyBdsa_DEGpDQ6PzZyYHHHokRIBY8thOdUQ";
}

using namespace Tellico;
using Tellico::Fetch::GoogleBookFetcher;

GoogleBookFetcher::GoogleBookFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false)
    , m_start(0)
    , m_apiKey(QLatin1String(GOOGLEBOOK_API_KEY)) {
}

GoogleBookFetcher::~GoogleBookFetcher() {
}

QString GoogleBookFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool GoogleBookFetcher::canSearch(FetchKey k) const {
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
  if(request().key == ISBN) {
    searchTerms = FieldFormat::splitValue(request().value);
  } else  {
    searchTerms += request().value;
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

  u.addQueryItem(QLatin1String("maxResults"), QString::number(GOOGLEBOOK_MAX_RETURNS));
  u.addQueryItem(QLatin1String("startIndex"), QString::number(m_start));
  u.addQueryItem(QLatin1String("printType"), QLatin1String("books"));
  // we don't require a key, cause it might work without it
  if(!m_apiKey.isEmpty()) {
    u.addQueryItem(QLatin1String("key"), m_apiKey);
  }

  switch(request().key) {
    case Title:
      u.addQueryItem(QLatin1String("q"), QLatin1String("intitle:") + term_);
      break;

    case Person:
      u.addQueryItem(QLatin1String("q"), QLatin1String("inauthor:") + term_);
      break;

    case ISBN:
      {
        const QString isbn = ISBNValidator::cleanValue(term_);
        u.addQueryItem(QLatin1String("q"), QLatin1String("isbn:") + isbn);
      }
      break;

    case Keyword:
      u.addQueryItem(QLatin1String("q"), term_);
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      return;
  }

//  myDebug() << "url:" << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  connect(job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
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
  emit signalDone(this);
}

Tellico::Data::EntryPtr GoogleBookFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  QString gbs = entry->field(QLatin1String("gbs-link"));
  if(!gbs.isEmpty()) {
    // quiet
    QByteArray data = FileHandler::readDataFile(QUrl::fromUserInput(gbs), true);
    QJsonDocument doc = QJsonDocument::fromJson(data);
    populateEntry(entry, doc.object().toVariantMap());
  }

  const QString image_id = entry->field(QLatin1String("cover"));
  // if it's still a url, we need to load it
  if(image_id.startsWith(QLatin1String("http"))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
      entry->setField(QLatin1String("cover"), QString());
    } else {
      entry->setField(QLatin1String("cover"), id);
    }
  }

  // don't want to include gbs json link
  entry->setField(QLatin1String("gbs-link"), QString());

  return entry;
}

Tellico::Fetch::FetchRequest GoogleBookFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QLatin1String("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

void GoogleBookFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);
//  myDebug();

  if(job->error()) {
    job->ui()->showErrorMessage();
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
    t.setCodec("UTF-8");
    t << data;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::BookCollection(true));
  // always add the gbs-link for fetchEntryHook
  Data::FieldPtr field(new Data::Field(QLatin1String("gbs-link"), QLatin1String("GBS Link"), Data::Field::URL));
  field->setCategory(i18n("General"));
  coll->addField(field);

  if(!coll->hasField(QLatin1String("googlebook")) && optionalFields().contains(QLatin1String("googlebook"))) {
    Data::FieldPtr field(new Data::Field(QLatin1String("googlebook"), i18n("Google Book Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantMap result = doc.object().toVariantMap();
  m_total = result.value(QLatin1String("totalItems")).toInt();
//  myDebug() << "total:" << m_total;

  QVariantList resultList = result.value(QLatin1String("items")).toList();
  if(resultList.isEmpty()) {
    myDebug() << "no results";
    endJob(job);
    return;
  }

  foreach(const QVariant& result, resultList) {
  //  myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toMap());

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  m_start = m_entries.count();
  m_hasMoreResults = request().key != ISBN && m_start <= m_total;
  endJob(job);
}

void GoogleBookFetcher::populateEntry(Data::EntryPtr entry, const QVariantMap& resultMap) {
  if(entry->collection()->hasField(QLatin1String("gbs-link"))) {
    entry->setField(QLatin1String("gbs-link"), value(resultMap, "selfLink"));
  }

  const QVariantMap volumeMap = resultMap.value(QLatin1String("volumeInfo")).toMap();
  entry->setField(QLatin1String("title"),     value(volumeMap, "title"));
  entry->setField(QLatin1String("subtitle"),  value(volumeMap, "subtitle"));
  entry->setField(QLatin1String("pub_year"),  value(volumeMap, "publishedDate").left(4));
  entry->setField(QLatin1String("author"),    value(volumeMap, "authors"));
  // workaround for bug, where publisher can be enclosed in quotes
  QString pub = value(volumeMap, "publisher");
  if(pub.startsWith(QLatin1Char('"')) && pub.endsWith(QLatin1Char('"'))) {
    pub.chop(1);
    pub = pub.remove(0, 1);
  }
  entry->setField(QLatin1String("publisher"), pub);
  entry->setField(QLatin1String("pages"),     value(volumeMap, "pageCount"));
  entry->setField(QLatin1String("language"),  value(volumeMap, "language"));
  entry->setField(QLatin1String("comments"),  value(volumeMap, "description"));

  QStringList catList = volumeMap.value(QLatin1String("categories")).toStringList();
  // google is going to give us a lot of categories
  QSet<QString> cats;
  foreach(const QString& cat, catList) {
    cats += cat.split(QRegExp(QLatin1String("\\s*/\\s*"))).toSet();
  }
  // remove General
  cats.remove(QLatin1String("General"));
  catList = cats.toList();
  catList.sort();
  entry->setField(QLatin1String("keyword"), catList.join(FieldFormat::delimiterString()));

  QString isbn;
  foreach(const QVariant& idVariant, volumeMap.value(QLatin1String("industryIdentifiers")).toList()) {
    const QVariantMap idMap = idVariant.toMap();
    if(value(idMap, "type") == QLatin1String("ISBN_10")) {
      isbn = value(idMap, "identifier");
      break;
    } else if(value(idMap, "type") == QLatin1String("ISBN_13")) {
      isbn = value(idMap, "identifier");
      // allow isbn10 to override, so don't break here
    }
  }
  if(!isbn.isEmpty()) {
    ISBNValidator val(this);
    val.fixup(isbn);
    entry->setField(QLatin1String("isbn"), isbn);
  }

  const QVariantMap imageMap = volumeMap.value(QLatin1String("imageLinks")).toMap();
  if(imageMap.contains(QLatin1String("small"))) {
    entry->setField(QLatin1String("cover"), value(imageMap, "small"));
  } else if(imageMap.contains(QLatin1String("thumbnail"))) {
    entry->setField(QLatin1String("cover"), value(imageMap, "thumbnail"));
  } else if(imageMap.contains(QLatin1String("smallThumbnail"))) {
    entry->setField(QLatin1String("cover"), value(imageMap, "smallThumbnail"));
  }

  if(optionalFields().contains(QLatin1String("googlebook"))) {
    entry->setField(QLatin1String("googlebook"), value(volumeMap, "infoLink"));
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
  hash[QLatin1String("googlebook")] = i18n("Google Book Link");
  return hash;
}

GoogleBookFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const GoogleBookFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                preferredName(),
                                QLatin1String("https://code.google.com/apis/console")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("API key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
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

// static
QString GoogleBookFetcher::value(const QVariantMap& map, const char* name) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    return v.toStringList().join(Tellico::FieldFormat::delimiterString());
  } else {
    return QString();
  }
}

