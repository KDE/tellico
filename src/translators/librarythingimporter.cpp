/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#include "librarythingimporter.h"
#include "../collections/bookcollection.h"
#include "../core/filehandler.h"
#include "../utils/objvalue.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KUrlRequester>
#include <kio_version.h>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

using Tellico::Import::LibraryThingImporter;

LibraryThingImporter::LibraryThingImporter() : Import::Importer()
  , m_widget(nullptr)
  , m_URLRequester(nullptr) {
}

LibraryThingImporter::LibraryThingImporter(const QUrl& url) : Import::Importer(url)
  , m_widget(nullptr)
  , m_URLRequester(nullptr) {
}

bool LibraryThingImporter::canImport(int type) const {
  return type == Data::Collection::Book;
}

Tellico::Data::CollPtr LibraryThingImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  QUrl jsonUrl;
  if(m_widget && m_URLRequester) {
    jsonUrl = m_URLRequester->url();
  } else if(!url().isEmpty()) {
    jsonUrl = url();
  } else {
    myWarning() << "no widget!";
    return Data::CollPtr();
  }

  if(jsonUrl.isEmpty() || !jsonUrl.isValid()) {
    myDebug() << "Bad jsonUrl:" << jsonUrl;
    return Data::CollPtr();
  }

  const QByteArray data = Tellico::FileHandler::readDataFile(jsonUrl, false /* quiet */);
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if(doc.isNull()) {
    myDebug() << "Bad json data:" << parseError.errorString();
    return Data::CollPtr();
  }

  static const QRegularExpression digits(QStringLiteral("\\d+"));
  static const QRegularExpression pubRx(QStringLiteral("^(.+?)[\\(,]"));

  m_coll = new Data::BookCollection(true);
  bool defaultTitle = true;
  Data::EntryList entries;
  const auto topObj = doc.object();
  for(auto i = topObj.constBegin(); i != topObj.constEnd(); ++i) {
    const auto obj = i.value().toObject();
    Data::EntryPtr entry(new Data::Entry(m_coll));
    entry->setField(QStringLiteral("title"), objValue(obj, "title"));
    entry->setField(QStringLiteral("pub_year"), objValue(obj, "date"));
    entry->setField(QStringLiteral("keyword"), objValue(obj, "tags"));
    entry->setField(QStringLiteral("genre"), objValue(obj, "genre"));
    entry->setField(QStringLiteral("series"), objValue(obj, "series"));
    entry->setField(QStringLiteral("language"), objValue(obj, "language"));

    const QString collName = objValue(obj, "collections");
    // default to using a collection title based on the first entry
    if(defaultTitle && !collName.isEmpty()) {
      defaultTitle = false;
      m_coll->setTitle(collName);
    }

    const QString cdate = objValue(obj, "entrydate");
    if(!cdate.isEmpty()) {
      entry->setField(QStringLiteral("cdate"), cdate);
    }

    QString pub = objValue(obj, "publication");
    // try to clean up the publisher
    QRegularExpressionMatch pubMatch = pubRx.match(pub);
    if(pubMatch.hasMatch()) {
      pub = pubMatch.captured(1).simplified();
    }
    entry->setField(QStringLiteral("publisher"), pub);

    const auto authorArray = obj[QLatin1StringView("authors")].toArray();
    QStringList authors;
    for(int i = 0; i < authorArray.size(); ++i) {
      const auto m = authorArray.at(i).toObject();
      // TODO: read config option for author formatting?
      // use first-lastname for now
      const QString role = objValue(m, "role");
      if(role.isEmpty() || role == QLatin1String("Author")) {
        authors += objValue(m, "fl");
      }
    }
    entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));

    const auto formatArray = obj[QLatin1StringView("format")].toArray();
    if(!formatArray.isEmpty()) {
      // use the first one
      const auto m = formatArray.at(0).toObject();
      const QString format = objValue(m, "text");
      const QString bindingName(QStringLiteral("binding"));
      if(format == QLatin1String("Paperback")) {
        entry->setField(bindingName, i18n("Paperback"));
      } else if(format == QLatin1String("Hardcover")) {
        entry->setField(bindingName, i18n("Hardback"));
      } else {
        // just in case there's a value there
        entry->setField(bindingName, format);
      }
    }

    QString isbn = objValue(obj, "originalisbn");
    ISBNValidator::staticFixup(isbn);
    entry->setField(QStringLiteral("isbn"), isbn);

    // grab first set of digits
    QRegularExpressionMatch match = digits.match(objValue(obj, "pages"));
    if(match.hasMatch()) {
      entry->setField(QStringLiteral("pages"), match.captured(0));
    }
    entries += entry;
  }
  m_coll->addEntries(entries);
  return m_coll;
}

QWidget* LibraryThingImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("LibraryThing Options"), m_widget);
  QFormLayout* lay = new QFormLayout(gbox);

  auto label = new QLabel(i18n("Export your LibraryThing collection in "
                               "<a href=\"https://www.librarything.com/export.php?export_type=json\">JSON format</a>."), gbox);
  label->setOpenExternalLinks(true);
  lay->addRow(label);

  m_URLRequester = new KUrlRequester(gbox);
  const QStringList filters = {i18n("JSON Files") + QLatin1String(" (*.json)"),
                               i18n("All Files") + QLatin1String(" (*)")};
  m_URLRequester->setNameFilters(filters);

  lay->addRow(i18n("LibraryThing file:"), m_URLRequester);

  l->addWidget(gbox);
  l->addStretch(1);

  return m_widget;
}
