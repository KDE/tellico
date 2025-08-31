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

#include "onmyshelfimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/comicbookcollection.h"
#include "../collections/boardgamecollection.h"
#include "../core/filehandler.h"
#include "../utils/objvalue.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

using Tellico::Import::OnMyShelfImporter;
using namespace Qt::Literals::StringLiterals;

OnMyShelfImporter::OnMyShelfImporter(const QUrl& url) : Import::Importer(url) {
}

bool OnMyShelfImporter::canImport(int type) const {
  return type == Data::Collection::Book ||
         type == Data::Collection::Video ||
         type == Data::Collection::ComicBook ||
         type == Data::Collection::BoardGame;
}

Tellico::Data::CollPtr OnMyShelfImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  const QByteArray data = Tellico::FileHandler::readDataFile(url(), false /* quiet */);
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if(doc.isNull()) {
    myDebug() << "Bad json data:" << parseError.errorString();
    return Data::CollPtr();
  }

  const auto topObj = doc.object();

  const auto collType = topObj["type"_L1].toString();
  myLog() << "Reading collection type:" << collType;
  if(collType == "books"_L1) {
    m_coll = new Data::BookCollection(true);
  } else if(collType == "movies"_L1) {
    m_coll = new Data::VideoCollection(true);
  } else if(collType == "comics"_L1) {
    m_coll = new Data::ComicBookCollection(true);
  } else if(collType == "board_games"_L1) {
    m_coll = new Data::BoardGameCollection(true);
  }
  if(!m_coll) {
    myLog() << "No collection created";
    return Data::CollPtr();
  }

  // choose language key from first value in collection names
  QString langKey;
  const auto collNameObj = topObj["name"_L1].toObject();
  for(auto i = collNameObj.constBegin(); i != collNameObj.constEnd(); ++i) {
    langKey = i.key();
    m_coll->setTitle(i.value().toString());
    break;
  }
  if(langKey.isEmpty()) {
    langKey = "en_US"_L1;
  }
  myLog() << "Using language key:" << langKey;

  Data::EntryList entries;
  QHash<int, Data::BorrowerPtr> borrowerHash;

  const auto items = topObj["items"_L1].toArray();
  for(auto i = items.constBegin(); i != items.constEnd(); ++i) {
    const auto itemObj = i->toObject();
    const auto propObj = itemObj["properties"_L1].toObject();

    Data::EntryPtr entry(new Data::Entry(m_coll));

    switch(m_coll->type()) {
      case Data::Collection::Book:
        populateBooks(entry, propObj);
        break;
      case Data::Collection::Video:
        populateMovies(entry, propObj);
        break;
      case Data::Collection::ComicBook:
        populateComics(entry, propObj);
        break;
      case Data::Collection::BoardGame:
        populateBoardGames(entry, propObj);
        break;
      default:
        break;
    }

    // cut off time portion of the datestamp
    entry->setField(QStringLiteral("cdate"), objValue(itemObj, "created").left(10), false);
    entry->setField(QStringLiteral("mdate"), objValue(itemObj, "updated").left(10), false);

    const auto loansArray = itemObj["loans"_L1].toArray();
    if(!loansArray.isEmpty() && loansArray.at(0)["state"_L1].toString() == "lent"_L1) {
      const auto loanObj = loansArray.at(0).toObject();
      Data::BorrowerPtr b;
      const auto bId = loanObj["borrowerId"_L1].toInt();
      if(borrowerHash.contains(bId)) {
        b = borrowerHash[bId];
      } else {
        b = Data::BorrowerPtr(new Data::Borrower(objValue(loanObj, "borrower"), QString()));
        borrowerHash.insert(bId, b);
        m_coll->addBorrower(b);
      }
      const auto loanDate = objValue(loanObj, "date").left(10);
      Data::LoanPtr l(new Data::Loan(entry, QDate::fromString(loanDate, Qt::ISODate), QDate(), QString()));
      l->setNote(objValue(loanObj, "notes"));
      b->addLoan(l);
    }

    entries += entry;
  }
  m_coll->addEntries(entries);
  return m_coll;
}

void OnMyShelfImporter::populateBooks(Data::EntryPtr entry_, const QJsonObject& obj_) {
  const bool updateModified = false;

  entry_->setField("title"_L1,      objValue(obj_, "title"), updateModified);
  entry_->setField("subtitle"_L1,   objValue(obj_, "subtitle"), updateModified);
  entry_->setField("author"_L1,     objValue(obj_, "author"), updateModified);
  entry_->setField("editor"_L1,     objValue(obj_, "editor"), updateModified);
  entry_->setField("publisher"_L1,  objValue(obj_, "publisher"), updateModified);
  entry_->setField("pub_year"_L1,   objValue(obj_, "pub_year"), updateModified);
  entry_->setField("genre"_L1,      objValue(obj_, "genre"), updateModified);
  entry_->setField("series"_L1,     objValue(obj_, "series"), updateModified);
  entry_->setField("series_num"_L1, objValue(obj_, "series_number"), updateModified);
  entry_->setField("language"_L1,   objValue(obj_, "language"), updateModified);

  auto summary = objValue(obj_, "summary");
  summary.replace('\n'_L1, "<br/>"_L1);
  entry_->setField("plot"_L1, summary, updateModified);

  QString isbn = objValue(obj_, "isbn");
  ISBNValidator::staticFixup(isbn);
  entry_->setField("isbn"_L1, isbn, false);

  // grab first set of digits
  static const QRegularExpression digits(QStringLiteral("\\d+"));
  auto match = digits.match(objValue(obj_, "pages"));
  if(match.hasMatch()) {
    entry_->setField(QStringLiteral("pages"), match.captured(0), updateModified);
  }

  const auto read = objValue(obj_, "read");
  if(read == "1"_L1 || read == "true"_L1 || read == "yes"_L1) {
    entry_->setField(QStringLiteral("read"), "true"_L1, updateModified);
  }

  const auto format = objValue(obj_, "format");
  if(!format.isEmpty()) {
    const QString bindingName(QStringLiteral("binding"));
    if(format == QLatin1String("Paperback")) {
      entry_->setField(bindingName, i18n("Paperback"), updateModified);
    } else if(format == QLatin1String("Hardcover")) {
      entry_->setField(bindingName, i18n("Hardback"), updateModified);
    } else {
      // just in case there's a value there
      entry_->setField(bindingName, format, updateModified);
    }
  }
}

void OnMyShelfImporter::populateMovies(Data::EntryPtr entry_, const QJsonObject& obj_) {
  const bool updateModified = false;

  entry_->setField("title"_L1,    objValue(obj_, "title"), updateModified);
  entry_->setField("director"_L1, objValue(obj_, "director"), updateModified);
  entry_->setField("rating"_L1,   objValue(obj_, "rating"), updateModified);

  auto summary = objValue(obj_, "synopsis");
  summary.replace('\n'_L1, "<br/>"_L1);
  entry_->setField("plot"_L1, summary, updateModified);
}

void OnMyShelfImporter::populateComics(Data::EntryPtr entry_, const QJsonObject& obj_) {
  const bool updateModified = false;

  entry_->setField("title"_L1,  objValue(obj_, "title"), updateModified);
  entry_->setField("writer"_L1, objValue(obj_, "writer"), updateModified);
  entry_->setField("series"_L1, objValue(obj_, "series"), updateModified);
  entry_->setField("issue"_L1,  objValue(obj_, "series_number"), updateModified);

  auto summary = objValue(obj_, "synopsis");
  summary.replace('\n'_L1, "<br/>"_L1);
  entry_->setField("plot"_L1, summary, updateModified);
}

void OnMyShelfImporter::populateBoardGames(Data::EntryPtr entry_, const QJsonObject& obj_) {
  const bool updateModified = false;

  entry_->setField("title"_L1,     objValue(obj_, "name"), updateModified);
  entry_->setField("designer"_L1,  objValue(obj_, "author"), updateModified);
  entry_->setField("mechanism"_L1, objValue(obj_, "mechanism"), updateModified);

  // grab first set of digits at beginning
  static const QRegularExpression digits(QStringLiteral("^(\\d+)-?"));
  auto match = digits.match(objValue(obj_, "age"));
  if(match.hasMatch()) {
    entry_->setField(QStringLiteral("minimum-age"), match.captured(1), updateModified);
  }
}
