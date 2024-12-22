/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "upcitemdbfetcher.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../utils/guiproxy.h"
#include "../utils/mapvalue.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace {
  static const int UPCITEMDB_MAX_RETURNS_TOTAL = 20;
  static const char* UPCITEMDB_API_URL = "https://api.upcitemdb.com/prod/trial";
}

using namespace Tellico;
using Tellico::Fetch::UPCItemDbFetcher;

UPCItemDbFetcher::UPCItemDbFetcher(QObject* parent_)
    : Fetcher(parent_)
    , m_started(false) {
}

UPCItemDbFetcher::~UPCItemDbFetcher() {
}

QString UPCItemDbFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool UPCItemDbFetcher::canSearch(Fetch::FetchKey k) const {
  return k == UPC || k == ISBN;
}

bool UPCItemDbFetcher::canFetch(int type) const {
  return type == Data::Collection::Video ||
         type == Data::Collection::Book ||
         type == Data::Collection::Album ||
         type == Data::Collection::Game ||
         type == Data::Collection::BoardGame;
}

void UPCItemDbFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void UPCItemDbFetcher::saveConfigHook(KConfigGroup& config_) {
  Q_UNUSED(config_)
}

void UPCItemDbFetcher::search() {
  continueSearch();
}

void UPCItemDbFetcher::continueSearch() {
  m_started = true;

  QUrl u(QString::fromLatin1(UPCITEMDB_API_URL));
  u = u.adjusted(QUrl::StripTrailingSlash);
  u.setPath(u.path() + QLatin1String("/lookup"));
  QUrlQuery q;
  switch(request().key()) {
    case ISBN:
      // do a upc search by 13-digit isbn
      {
        // only grab first value
        QString isbn = request().value().section(QLatin1Char(';'), 0);
        isbn = ISBNValidator::isbn13(isbn);
        isbn.remove(QLatin1Char('-'));
        q.addQueryItem(QStringLiteral("upc"), isbn);
      }
      break;

    case UPC:
      q.addQueryItem(QStringLiteral("upc"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setQuery(q);

  myLog() << "Reading" << u.toDisplayString();
  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result, this, &UPCItemDbFetcher::slotComplete);
}

void UPCItemDbFetcher::stop() {
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

Tellico::Fetch::FetchRequest UPCItemDbFetcher::updateRequest(Data::EntryPtr entry_) {
  const QString isbn = entry_->field(QStringLiteral("isbn"));
  if(!isbn.isEmpty()) {
    return FetchRequest(ISBN, isbn);
  }

  const QString upc = entry_->field(QStringLiteral("upc"));
  if(!upc.isEmpty()) {
    return FetchRequest(UPC, upc);
  }

  const QString barcode = entry_->field(QStringLiteral("barcode"));
  if(!barcode.isEmpty()) {
    return FetchRequest(UPC, barcode);
  }

  return FetchRequest();
}

void UPCItemDbFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "No data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from upcitemdbfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test-upcitemdb.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if(doc.isNull()) {
    myDebug() << "null JSON document";
    stop();
    return;
  }
  const auto obj = doc.object();
  // check for error
  if(obj.value(QStringLiteral("code")) == QLatin1String("TOO_FAST")) {
    const QString msg = obj.value(QStringLiteral("message")).toString();
    message(msg, MessageHandler::Error);
    myDebug() << "UPCItemDbFetcher -" << msg;
    stop();
    return;
  }

  Data::CollPtr coll = CollectionFactory::collection(collectionType(), true);
  if(!coll) {
    stop();
    return;
  }

  if(optionalFields().contains(QStringLiteral("barcode"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("barcode"), i18n("Barcode")));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }

  const auto results = obj.value(QLatin1String("items")).toArray();
  if(results.isEmpty()) {
    myLog() << "No results";
    stop();
    return;
  }

  int count = 0;
  for(const QJsonValue& result : results) {
//    myDebug() << "found result:" << result;

    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, result.toObject().toVariantMap());

    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
    ++count;
    if(count >= UPCITEMDB_MAX_RETURNS_TOTAL) {
      break;
    }
  }

  stop();
}

Tellico::Data::EntryPtr UPCItemDbFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(!entry) {
    myWarning() << "no entry in dict";
    return Data::EntryPtr();
  }

  // image might still be a URL
  const QString cover(QStringLiteral("cover"));
  const QString image_id = entry->field(cover);
  if(image_id.contains(QLatin1Char('/'))) {
    const QUrl imageUrl = QUrl::fromUserInput(image_id);
    // use base url as referrer
    const QString id = ImageFactory::addImage(imageUrl, false /* quiet */, imageUrl.adjusted(QUrl::RemovePath));
    if(id.isEmpty()) {
      myDebug() << "image id is empty";
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(cover, id);
  }

  return entry;
}

void UPCItemDbFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_) {
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "title"));
  parseTitle(entry_);
//  entry_->setField(QStringLiteral("year"),  mapValue(resultMap_, "premiered").left(4));
  const QString barcode = QStringLiteral("barcode");
  if(optionalFields().contains(barcode)) {
    entry_->setField(barcode, mapValue(resultMap_, "upc"));
  }

  // take the first cover
  const auto imageList = resultMap_.value(QLatin1String("images")).toList();
  for(const auto& imageValue : imageList) {
    // skip booksamillion images
    const QString image = imageValue.toString();
    if(!image.isEmpty() && !image.contains(QLatin1String("booksamillion.com"))) {
      entry_->setField(QStringLiteral("cover"), image);
      break;
    }
  }

  switch(collectionType()) {
    case Data::Collection::Video:
      entry_->setField(QStringLiteral("studio"), mapValue(resultMap_, "brand"));
      entry_->setField(QStringLiteral("plot"), mapValue(resultMap_, "description"));
      break;

    case Data::Collection::Book:
      entry_->setField(QStringLiteral("publisher"), mapValue(resultMap_, "publisher"));
      entry_->setField(QStringLiteral("isbn"), mapValue(resultMap_, "isbn"));
      break;

    case Data::Collection::Album:
      entry_->setField(QStringLiteral("label"), mapValue(resultMap_, "brand"));
      {
        const QString cat = mapValue(resultMap_, "category");
        if(cat.contains(QStringLiteral("Music CDs"))) {
          entry_->setField(QStringLiteral("medium"), i18n("Compact Disc"));
        }
      }
      break;

    case Data::Collection::Game:
    case Data::Collection::BoardGame:
      entry_->setField(QStringLiteral("publisher"), mapValue(resultMap_, "brand"));
      entry_->setField(QStringLiteral("description"), mapValue(resultMap_, "description"));
      break;

    default:
      break;
  }

  // do this after all other parsing
  parseTitle(entry_);
}

void UPCItemDbFetcher::parseTitle(Tellico::Data::EntryPtr entry_) {
  // assume that everything in brackets or parentheses is extra
  static const QRegularExpression rx(QLatin1String("[\\(\\[](.*?)[\\)\\]]"));
  QString title = entry_->field(QStringLiteral("title"));
  int pos = 0;
  QRegularExpressionMatch match = rx.match(title, pos);
  while(match.hasMatch()) {
    pos = match.capturedStart();
    if(parseTitleToken(entry_, match.captured(1))) {
      title.remove(match.capturedStart(), match.capturedLength());
      --pos; // search again there
    }
    match = rx.match(title, pos+1);
  }
  // look for "word1 - word2"
  static const QRegularExpression dashWords(QLatin1String("(.+) - (.+)"));
  QRegularExpressionMatch dashMatch = dashWords.match(title);
  if(dashMatch.hasMatch()) {
    switch(collectionType()) {
      case Data::Collection::Book:
        title = dashMatch.captured(1);
        {
          static const QRegularExpression byAuthor(QLatin1String("by (.+)"));
          QRegularExpressionMatch authorMatch = byAuthor.match(dashMatch.captured(2));
          if(authorMatch.hasMatch()) {
            entry_->setField(QStringLiteral("author"), authorMatch.captured(1).simplified());
          }
        }
        break;

      case Data::Collection::Album:
        entry_->setField(QStringLiteral("artist"), dashMatch.captured(1).simplified());
        title = dashMatch.captured(2);
        break;

      case Data::Collection::Game:
        title = dashMatch.captured(1);
        {
          const QString platform = QStringLiteral("platform");
          const QString maybe = i18n(dashMatch.captured(2).simplified().toUtf8().constData());
          Data::FieldPtr f = entry_->collection()->fieldByName(platform);
          if(f && f->allowed().contains(maybe)) {
            entry_->setField(platform, maybe);
          }
        }
        break;
    }
  }
  entry_->setField(QStringLiteral("title"), title.simplified());
}

// mostly taken from amazonfetcher
bool UPCItemDbFetcher::parseTitleToken(Tellico::Data::EntryPtr entry_, const QString& token_) {
//  myDebug() << "title token:" << token_;
  // if res = true, then the token gets removed from the title
  bool res = false;
  static const QRegularExpression yearRx(QLatin1String("\\d{4}"));
  QRegularExpressionMatch yearMatch = yearRx.match(token_);
  if(yearMatch.hasMatch()) {
    entry_->setField(QStringLiteral("year"), yearMatch.captured());
    res = true;
  }
  if(token_.indexOf(QLatin1String("widescreen"), 0, Qt::CaseInsensitive) > -1 ||
     token_.indexOf(i18n("Widescreen"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("widescreen"), QStringLiteral("true"));
    // res = true; leave it in the title
  } else if(token_.indexOf(QLatin1String("full screen"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  } else if(token_.indexOf(QLatin1String("standard edition"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  } else if(token_.indexOf(QLatin1String("import"), 0, Qt::CaseInsensitive) > -1) {
    // skip, but go ahead and remove from title
    res = true;
  }
  if(token_.indexOf(QLatin1String("blu-ray"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("medium"), i18n("Blu-ray"));
    res = true;
  } else if(token_.indexOf(QLatin1String("hd dvd"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("medium"), i18n("HD DVD"));
    res = true;
  } else if(token_.indexOf(QLatin1String("vhs"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("medium"), i18n("VHS"));
    res = true;
  }
  if(token_.indexOf(QLatin1String("director's cut"), 0, Qt::CaseInsensitive) > -1 ||
     token_.indexOf(i18n("Director's Cut"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("directors-cut"), QStringLiteral("true"));
    // res = true; leave it in the title
  }
  const QString tokenLower = token_.toLower();
  if(tokenLower == QLatin1String("ntsc")) {
    entry_->setField(QStringLiteral("format"), i18n("NTSC"));
    res = true;
  }
  if(tokenLower == QLatin1String("dvd")) {
    entry_->setField(QStringLiteral("medium"), i18n("DVD"));
    res = true;
  }
  if(tokenLower == QLatin1String("cd") && collectionType() == Data::Collection::Album) {
    entry_->setField(QStringLiteral("medium"), i18n("Compact Disc"));
    res = true;
  }
  if(tokenLower == QLatin1String("dvd")) {
    entry_->setField(QStringLiteral("medium"), i18n("DVD"));
    res = true;
  }
  if(token_.indexOf(QLatin1String("series"), 0, Qt::CaseInsensitive) > -1) {
    entry_->setField(QStringLiteral("series"), token_);
    res = true;
  }
  static const QRegularExpression regionRx(QLatin1String("Region [1-9]"));
  QRegularExpressionMatch match = regionRx.match(token_);
  if(match.hasMatch()) {
    entry_->setField(QStringLiteral("region"), i18n(match.captured().toUtf8().constData()));
    res = true;
  }
  if(collectionType() == Data::Collection::Game) {
    Data::FieldPtr f = entry_->collection()->fieldByName(QStringLiteral("platform"));
    if(f && f->allowed().contains(token_)) {
      res = true;
    }
  } else if(collectionType() == Data::Collection::Book) {
    const QString binding = QStringLiteral("binding");
    Data::FieldPtr f = entry_->collection()->fieldByName(binding);
    const QString maybe = i18n(token_.toUtf8().constData());
    if(f && f->allowed().contains(maybe)) {
      entry_->setField(binding, maybe);
      res = true;
    }
  }
  return res;
}

Tellico::Fetch::ConfigWidget* UPCItemDbFetcher::configWidget(QWidget* parent_) const {
  return new UPCItemDbFetcher::ConfigWidget(parent_, this);
}

QString UPCItemDbFetcher::defaultName() {
  return QStringLiteral("UPCitemdb"); // this is the capitalization they use on their site
}

QString UPCItemDbFetcher::defaultIcon() {
  return favIcon("https://www.upcitemdb.com");
}

Tellico::StringHash UPCItemDbFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("barcode")] = i18n("Barcode");
  return hash;
}

UPCItemDbFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const UPCItemDbFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(UPCItemDbFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString UPCItemDbFetcher::ConfigWidget::preferredName() const {
  return UPCItemDbFetcher::defaultName();
}
