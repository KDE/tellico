/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "openlibraryfetcher.h"
#include "../collections/bookcollection.h"
#include "../collections/comicbookcollection.h"
#include "../images/imagefactory.h"
#include "../gui/combobox.h"
#include "../utils/isbnvalidator.h"
#include "../utils/guiproxy.h"
#include "../utils/objvalue.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace {
  static const char* OPENLIBRARY_QUERY_URL = "https://openlibrary.org/query.json";
  static const char* OPENLIBRARY_AUTHOR_QUERY_URL = "https://openlibrary.org/search/authors.json";
}

using namespace Tellico;
using Tellico::Fetch::OpenLibraryFetcher;

OpenLibraryFetcher::OpenLibraryFetcher(QObject* parent_)
    : Fetcher(parent_), m_imageSize(MediumImage), m_started(false) {
}

OpenLibraryFetcher::~OpenLibraryFetcher() {
}

QString OpenLibraryFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool OpenLibraryFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == ISBN || k == LCCN;
}

bool OpenLibraryFetcher::canFetch(int type) const {
  return type == Data::Collection::Book ||
      type == Data::Collection::Bibtex ||
      type == Data::Collection::ComicBook;
}

void OpenLibraryFetcher::readConfigHook(const KConfigGroup& config_) {
  const int imageSize = config_.readEntry("Image Size", -1);
  if(imageSize > -1) {
    m_imageSize = static_cast<ImageSize>(imageSize);
  }
}

void OpenLibraryFetcher::search() {
  m_started = true;
  // we only split ISBN and LCCN values
  QStringList searchTerms;
  if(request().key() == ISBN || request().key() == LCCN) {
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

void OpenLibraryFetcher::doSearch(const QString& term_) {
  QUrl u(QString::fromLatin1(OPENLIBRARY_QUERY_URL));
  QUrlQuery q;
  // books are type/edition
  q.addQueryItem(QStringLiteral("type"), QStringLiteral("/type/edition"));

  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("title"), term_);
      break;

    case Person:
      {
        QString author = getAuthorKeys(term_);
        if(author.isEmpty()) {
          myLog() << "No matching authors found";
          return;
        }
        author.prepend(QLatin1String("/authors/"));
        q.addQueryItem(QStringLiteral("authors"), author);
      }
      break;

    case ISBN:
      {
        const QString isbn = ISBNValidator::cleanValue(term_);
        if(isbn.size() > 10) {
          q.addQueryItem(QStringLiteral("isbn_13"), isbn);
        } else {
          q.addQueryItem(QStringLiteral("isbn_10"), isbn);
        }
      }
      break;

    case LCCN:
      q.addQueryItem(QStringLiteral("lccn"), term_);
      break;

    case Raw:
      {
        // raw query comes in as a query string, combine it
        QUrlQuery newQuery(term_);
        const auto newQueryItems = newQuery.queryItems();
        for(const auto& item : newQueryItems) {
          q.addQueryItem(item.first, item.second);
        }
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return;
  }
  q.addQueryItem(QStringLiteral("*"), QString());
  u.setQuery(q);
//  myDebug() << u;

  QPointer<KIO::StoredTransferJob> job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  connect(job.data(), &KJob::result, this, &OpenLibraryFetcher::slotComplete);
  m_jobs << job;
}

void OpenLibraryFetcher::endJob(KIO::StoredTransferJob* job_) {
  m_jobs.removeAll(job_);
  if(m_jobs.isEmpty())  {
    stop();
  }
}

void OpenLibraryFetcher::stop() {
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

Tellico::Data::EntryPtr OpenLibraryFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // possible that the author is set on the work but not the edition
  // see https://github.com/internetarchive/openlibrary/issues/8144
  const QString authorString(QStringLiteral("author"));
  const QString workString(QStringLiteral("openlibrary-work"));
  if(entry->field(authorString).isEmpty()) {
    const QString work = entry->field(workString);
    if(!work.isEmpty()) {
      const QUrl workUrl(QStringLiteral("https://openlibrary.org%1.json").arg(work));
      QStringList authors;
      const auto output = FileHandler::readDataFile(workUrl, true /*quiet*/);
#if 0
      myWarning() << "Remove debug from openlibraryfetcher.cpp";
      QFile f(QString::fromLatin1("/tmp/openlibrary-work.json"));
      if(f.open(QIODevice::WriteOnly)) {
        QTextStream t(&f);
        t << output;
      }
      f.close();
#endif
      QJsonDocument doc = QJsonDocument::fromJson(output);
      const auto obj = doc.object();
      const auto array = obj.value(QLatin1String("authors")).toArray();
      for(int i = 0; i < array.count(); i++) {
        const QString key = objValue(array.at(i).toObject(), "author", "key");
        if(m_authorHash.contains(key)) {
          authors += m_authorHash.value(key);
          continue;
        }
        // now grab author name by key
        const QUrl authorUrl(QStringLiteral("https://openlibrary.org%1.json").arg(key));
        const auto output2 = FileHandler::readDataFile(authorUrl, true /*quiet*/);
        QJsonDocument doc2 = QJsonDocument::fromJson(output2);
        const QString author = objValue(doc2.object(), "name");
        if(!author.isEmpty()) {
          m_authorHash.insert(key, author);
          authors += author;
        }
      }
      if(!authors.isEmpty()) {
        entry->setField(authorString, authors.join(FieldFormat::delimiterString()));
      }

      // since we already might have the info, check for series in the subjects field
      const QString seriesStr(QStringLiteral("series"));
      if(entry->field(seriesStr).isEmpty()) {
        const auto subjArray = obj.value(QLatin1String("subjects")).toArray();
        for(const auto& res : subjArray) {
          QString value = res.toString();
          if(value.startsWith(QLatin1String("series:"))) {
            value.remove(0, 7); // remove first 7 characters
            value.replace(QLatin1Char('_'), QLatin1Char(' '));
            value = FieldFormat::capitalize(value);
            entry->setField(seriesStr, value);
            break;
          }
        }
      }
    }
  }
  // no longer need the field
  entry->collection()->removeField(workString);

  const QString openlibraryString(QStringLiteral("openlibrary"));
  const QString coverString(QStringLiteral("cover"));
  if(m_imageSize != NoImage && entry->field(coverString).isEmpty()) {
    QString imgSize;
    switch(m_imageSize) {
      case LargeImage:  imgSize = QLatin1Char('L'); break;
      case MediumImage: imgSize = QLatin1Char('M'); break;
      case SmallImage:  imgSize = QLatin1Char('S'); break;
      case NoImage: myWarning() << "impossible image size"; break;
    }
    QString coverId, imageUrl;

    // just want the portion after the last slash
    QString olid = entry->field(openlibraryString).section(QLatin1Char('/'), -1);
    if(!olid.isEmpty()) {
      coverId = olid;
      imageUrl = QStringLiteral("https://covers.openlibrary.org/b/olid/%1-%2.jpg?default=false");
    } else {
      coverId = ISBNValidator::cleanValue(entry->field(QStringLiteral("isbn")));
      imageUrl = QStringLiteral("https://covers.openlibrary.org/b/isbn/%1-%2.jpg?default=false");
    }
    if(!coverId.isEmpty()) {
      imageUrl = imageUrl.arg(coverId, imgSize);
      entry->setField(coverString, ImageFactory::addImage(QUrl(imageUrl), true));
    }
  }

  // remove the openlibrary field if undesired
  if(entry->collection()->hasField(openlibraryString) &&
     !optionalFields().contains(openlibraryString)) {
    entry->setField(openlibraryString, QString());
    entry->collection()->removeField(entry->collection()->fieldByName(openlibraryString));
  }

  return entry;
}

Tellico::Fetch::FetchRequest OpenLibraryFetcher::updateRequest(Data::EntryPtr entry_) {
  QString olid = entry_->field(QStringLiteral("openlibrary")).section(QLatin1Char('/'), -2);
  if(!olid.isEmpty()) {
    return FetchRequest(Raw, QStringLiteral("key=/") + olid);
  }
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }
  const QString lccn = entry_->field(QStringLiteral("lccn"));
  if(!lccn.isEmpty()) {
    return FetchRequest(LCCN, lccn);
  }
  const QString title = entry_->field(QStringLiteral("title"));
  if(title.isEmpty()) {
    return FetchRequest();
  }

  // can't search by authors, for now, since the author value is a key reference
  // can't search by pub year since many of the publish_date fields are a full month, day, year

  const QString pub = entry_->field(QStringLiteral("publisher"));
  auto publishers = FieldFormat::splitValue(pub);
  if(!publishers.isEmpty()) {
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("title"), title);
    q.addQueryItem(QStringLiteral("publishers"), publishers.first());
    return FetchRequest(Raw, q.query());
  }

  // fallback to just title search
  return FetchRequest(Title, title);
}

void OpenLibraryFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

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
  myWarning() << "Remove debug from openlibraryfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const auto array = doc.array();
  if(array.isEmpty()) {
//    myDebug() << "no results";
    endJob(job);
    return;
  }

  Data::CollPtr coll;
  if(request().collectionType() == Data::Collection::ComicBook) {
    coll = new Data::ComicBookCollection(true);
  } else {
    coll = new Data::BookCollection(true);
  }
  // add a temporary work id field
  Data::FieldPtr wField(new Data::Field(QStringLiteral("openlibrary-work"), QString()));
  coll->addField(wField);

  // always set the openlibrary link, removed later if unwanted
  Data::FieldPtr field(new Data::Field(QStringLiteral("openlibrary"), i18n("OpenLibrary Link"), Data::Field::URL));
  field->setCategory(i18n("General"));
  coll->addField(field);

  for(const auto& result : array) {
    // be sure to check that the fetcher has not been stopped
    // crashes can occur if not
    if(!m_started) {
      break;
    }

    const auto resObj = result.toObject();
    if(coll->type() == Data::Collection::ComicBook) {
      const auto binding = objValue(resObj, "physical_format");
      if(!binding.isEmpty() && binding != QLatin1String("comic")) {
        myLog() << "Skipping non-comic result:" << objValue(resObj, "title");
        continue;
      }
    }

    Data::EntryPtr entry(new Data::Entry(coll));
    populate(entry, resObj);

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }

//  m_start = m_entries.count();
//  m_hasMoreResults = m_start <= m_total;
  m_hasMoreResults = false; // for now, no continued searches
  endJob(job);
}

void OpenLibraryFetcher::populate(Data::EntryPtr entry_, const QJsonObject& obj_) {
  static const QRegularExpression yearRx(QStringLiteral("\\d{4}"));

  entry_->setField(QStringLiteral("title"), objValue(obj_, "title"));

  // only allow comic format for comic book collections
  QString binding = objValue(obj_, "physical_format");
  const auto bindingLower = binding.toLower();
  if(bindingLower == QLatin1String("hardcover")) {
    binding = QStringLiteral("Hardback");
  } else if(bindingLower == QLatin1String("ebook")) {
    binding = QStringLiteral("E-Book");
  } else if(bindingLower.contains(QStringLiteral("paperback"))) {
    binding = QStringLiteral("Paperback");
  }
  if(!binding.isEmpty()) {
    entry_->setField(QStringLiteral("binding"), i18n(binding.toUtf8().constData()));
  }

  entry_->setField(QStringLiteral("subtitle"), objValue(obj_, "subtitle"));
  auto yearMatch = yearRx.match(objValue(obj_, "publish_date"));
  if(yearMatch.hasMatch()) {
    entry_->setField(QStringLiteral("pub_year"), yearMatch.captured());
  }
  yearMatch = yearRx.match(objValue(obj_, "copyright_date"));
  if(yearMatch.hasMatch()) {
    entry_->setField(QStringLiteral("cr_year"), yearMatch.captured());
  }
  QString isbn = objValue(obj_, "isbn_10");
  if(isbn.isEmpty()) {
    isbn = objValue(obj_, "isbn_13");
  }
  const QString isbnName(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    if(!entry_->collection()->hasField(isbnName)) {
      entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::IsbnField));
    }
    ISBNValidator val(this);
    val.fixup(isbn);
    entry_->setField(isbnName, isbn);
  }
  const QString lccnName(QStringLiteral("lccn"));
  const QString lccn = objValue(obj_, "lccn");
  if(!lccn.isEmpty()) {
    if(!entry_->collection()->hasField(lccnName)) {
      entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::LccnField));
    }
    entry_->setField(lccnName, lccn);
  }
  entry_->setField(QStringLiteral("genre"), objValue(obj_, "genres"));
  entry_->setField(QStringLiteral("keyword"), objValue(obj_, "subjects"));
  entry_->setField(QStringLiteral("edition"), objValue(obj_, "edition_name"));
  entry_->setField(QStringLiteral("publisher"), objValue(obj_, "publishers"));
  entry_->setField(QStringLiteral("series"), objValue(obj_, "series"));
  entry_->setField(QStringLiteral("pages"), objValue(obj_, "number_of_pages"));
  entry_->setField(QStringLiteral("comments"), objValue(obj_, "notes", "value"));
  entry_->setField(QStringLiteral("openlibrary"), QLatin1String("https://openlibrary.org") + objValue(obj_, "key"));

  const auto worksArray = obj_[QLatin1StringView("works")].toArray();
  if(!worksArray.isEmpty()) {
    const auto workObj = worksArray.first().toObject();
    const QString key = objValue(workObj, "key");
    if(!key.isEmpty()) {
      entry_->setField(QStringLiteral("openlibrary-work"), key);
    }
  }

  QStringList authors;
  const auto authorArray = obj_[QLatin1StringView("authors")].toArray();
  for(const auto& author : authorArray) {
    const auto authorObj = author.toObject();
    const QString key = objValue(authorObj, "key");
    if(m_authorHash.contains(key)) {
      authors += m_authorHash.value(key);
    } else if(!key.isEmpty()) {
      QUrl authorUrl(QString::fromLatin1(OPENLIBRARY_QUERY_URL));
      QUrlQuery q;
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("/type/author"));
      q.addQueryItem(QStringLiteral("key"), key);
      q.addQueryItem(QStringLiteral("name"), QString());
      authorUrl.setQuery(q);

      QString output = FileHandler::readTextFile(authorUrl, true /*quiet*/);
      QJsonDocument doc2 = QJsonDocument::fromJson(output.toUtf8());
      const auto authorArray = doc2.array();
      if(!authorArray.isEmpty()) {
        const QString name = objValue(authorArray.at(0).toObject(), "name");
        if(!name.isEmpty()) {
          authors << name;
          m_authorHash.insert(key, name);
        }
      }
    }
  }
  if(!authors.isEmpty()) {
    entry_->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
  }

  QStringList translators;
  const auto contribsArray = obj_[QLatin1StringView("contributors")].toArray();
  for(const auto& contrib : contribsArray) {
    const auto contribObj = contrib.toObject();
    const auto role = objValue(contribObj, "role");
    if(role == QLatin1StringView("Translator")) {
      translators += objValue(contribObj, "name");
    }
  }
  if(!translators.isEmpty()) {
    entry_->setField(QStringLiteral("translator"), translators.join(FieldFormat::delimiterString()));
  }

  QStringList langs;
  const auto langArray = obj_[QLatin1String("languages")].toArray();
  for(const auto& lang : langArray) {
    const auto langObj = lang.toObject();
    const QString key = objValue(langObj, "key");
    if(m_langHash.contains(key)) {
      langs += m_langHash.value(key);
    } else if(!key.isEmpty()) {
      QUrl langUrl(QString::fromLatin1(OPENLIBRARY_QUERY_URL));
      QUrlQuery q;
      q.addQueryItem(QStringLiteral("type"), QStringLiteral("/type/language"));
      q.addQueryItem(QStringLiteral("key"), key);
      q.addQueryItem(QStringLiteral("name"), QString());
      langUrl.setQuery(q);

      const auto output = FileHandler::readDataFile(langUrl, true /*quiet*/);
      QJsonDocument doc2 = QJsonDocument::fromJson(output);
      const auto langArray = doc2.array();
      if(!langArray.isEmpty()) {
        const QString name = objValue(langArray.at(0).toObject(), "name");
        if(!name.isEmpty()) {
          langs << i18n(name.toUtf8().constData());
          m_langHash.insert(key, langs.last());
        }
      }
    }
  }
  if(!langs.isEmpty()) {
    entry_->setField(QStringLiteral("language"), langs.join(FieldFormat::delimiterString()));
  }
}

QString OpenLibraryFetcher::getAuthorKeys(const QString& term_) {
  QUrl u(QString::fromLatin1(OPENLIBRARY_AUTHOR_QUERY_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("q"), term_);
  q.addQueryItem(QStringLiteral("fields"), QStringLiteral("key,name"));
  u.setQuery(q);

//  myLog() << "Searching for authors:" << u.toDisplayString();
  QString output = FileHandler::readTextFile(u, true /*quiet*/, true /*utf8*/);
#if 0
  myWarning() << "Remove author debug from openlibraryfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test-openlibraryauthor.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output;
  }
  f.close();
#endif
  const QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
  const auto array = doc.object().value(QLatin1String("docs")).toArray();
  if(array.isEmpty()) {
    return QString();
  }
  // right now, only use the first to search on
  const auto obj1 = array.at(0).toObject();
  const auto key = obj1.value(QLatin1String("key")).toString();
  const auto name = obj1.value(QLatin1String("name")).toString();
  m_authorHash.insert(key, name);
  return key;
}

Tellico::Fetch::ConfigWidget* OpenLibraryFetcher::configWidget(QWidget* parent_) const {
  return new OpenLibraryFetcher::ConfigWidget(parent_, this);
}

QString OpenLibraryFetcher::defaultName() {
  return QStringLiteral("Open Library"); // no translation
}

QString OpenLibraryFetcher::defaultIcon() {
  return favIcon("https://openlibrary.org");
}

Tellico::StringHash OpenLibraryFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("openlibrary")] = i18n("OpenLibrary Link");
  return hash;
}

OpenLibraryFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const OpenLibraryFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  auto label = new QLabel(i18n("&Image size: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_imageCombo = new GUI::ComboBox(optionsWidget());
  m_imageCombo->addItem(i18n("Small Image"), SmallImage);
  m_imageCombo->addItem(i18n("Medium Image"), MediumImage);
  m_imageCombo->addItem(i18n("Large Image"), LargeImage);
  m_imageCombo->addItem(i18n("No Image"), NoImage);
  void (GUI::ComboBox::* activatedInt)(int) = &GUI::ComboBox::activated;
  connect(m_imageCombo, activatedInt, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_imageCombo, row, 1);
  QString w = i18n("The cover image may be downloaded as well. However, too many large images in the "
                   "collection may degrade performance.");
  label->setWhatsThis(w);
  m_imageCombo->setWhatsThis(w);
  label->setBuddy(m_imageCombo);

  l->setRowStretch(++row, 10);

  if(fetcher_) {
    m_imageCombo->setCurrentData(fetcher_->m_imageSize);
  } else { // defaults
    m_imageCombo->setCurrentData(MediumImage);
  }

  // now add additional fields widget
  addFieldsWidget(OpenLibraryFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void OpenLibraryFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const int n = m_imageCombo->currentData().toInt();
  config_.writeEntry("Image Size", n);
}

QString OpenLibraryFetcher::ConfigWidget::preferredName() const {
  return OpenLibraryFetcher::defaultName();
}
