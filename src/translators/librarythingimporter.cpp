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
#include "../utils/string_utils.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KUrlRequester>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

using Tellico::Import::LibraryThingImporter;

LibraryThingImporter::LibraryThingImporter() : Import::Importer(), m_widget(nullptr), m_URLRequester(nullptr) {
}

bool LibraryThingImporter::canImport(int type) const {
  return type == Data::Collection::Book;
}

Tellico::Data::CollPtr LibraryThingImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  if(!m_widget) {
    myWarning() << "no widget!";
    return Data::CollPtr();
  }

  QUrl jsonUrl = m_URLRequester->url();

  if(jsonUrl.isEmpty() || !jsonUrl.isValid()) {
    myDebug() << "Bad jsonUrl:" << jsonUrl;
    return Data::CollPtr();
  }


  QByteArray data = Tellico::FileHandler::readDataFile(jsonUrl, false /* quiet */);
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if(doc.isNull()) {
    myDebug() << "Bad json data:" << parseError.errorString();
    return Data::CollPtr();
  }

  m_coll = new Data::BookCollection(true);
  Data::EntryList entries;
  QVariantMap map = doc.object().toVariantMap();
  QMapIterator<QString, QVariant> i(map);
  while (i.hasNext()) {
    i.next();
    QVariantMap valueMap = i.value().toMap();
    Data::EntryPtr entry(new Data::Entry(m_coll));
    entry->setField(QStringLiteral("title"), mapValue(valueMap, "title"));
    entry->setField(QStringLiteral("pub_year"), mapValue(valueMap, "date"));
    entry->setField(QStringLiteral("keyword"), mapValue(valueMap, "tags"));
    entry->setField(QStringLiteral("genre"), mapValue(valueMap, "genre"));
    entry->setField(QStringLiteral("series"), mapValue(valueMap, "series"));
    entry->setField(QStringLiteral("language"), mapValue(valueMap, "language"));

    QJsonArray authorArray = valueMap.value(QStringLiteral("authors")).toJsonArray();
    QStringList authors;
    for(int i = 0; i < authorArray.size(); ++i) {
      QVariantMap m = authorArray.at(i).toObject().toVariantMap();
      // TODO: read config option for author formatting?
      // use first-lastname for now
      authors += mapValue(m, "fl");
    }
    entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));

    QJsonArray formatArray = valueMap.value(QStringLiteral("format")).toJsonArray();
    for(int i = 0; i < formatArray.size(); ++i) {
      QVariantMap m = formatArray.at(i).toObject().toVariantMap();
      const QString format = mapValue(m, "text");
      if(format == QLatin1String("Paperback")) {
        entry->setField(QStringLiteral("binding"), i18n("Paperback"));
      } else if(format == QLatin1String("Hardcover")) {
        entry->setField(QStringLiteral("binding"), i18n("Hardback"));
      } else {
        // just in case there's a value there
        entry->setField(QStringLiteral("binding"), format);
      }
      break;
    }

    QString isbn = mapValue(valueMap, "originalisbn");
    ISBNValidator::staticFixup(isbn);
    entry->setField(QStringLiteral("isbn"), isbn);

    // grab first set of digits
    QRegularExpression digits(QStringLiteral("\\d+"));
    QRegularExpressionMatch match = digits.match(mapValue(valueMap, "pages"));
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

  lay->addRow(new QLabel(i18n("Export your LibraryThing collection in "
                              "<a href=\"https://www.librarything.com/export.php?export_type=json\">JSON format</a>."), gbox));

  m_URLRequester = new KUrlRequester(gbox);
  // these are in the old KDE4 filter format, not the Qt5 format
  QString filter = QLatin1String("*.json|") + i18n("JSON Files")
                 + QLatin1Char('\n')
                 + QLatin1String("*|") + i18n("All Files");
  m_URLRequester->setFilter(filter);

  lay->addRow(i18n("LibraryThing file:"), m_URLRequester);

  l->addWidget(gbox);
  l->addStretch(1);

  return m_widget;
}
