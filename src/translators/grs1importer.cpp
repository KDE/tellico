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

#include "grs1importer.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"

#include <KLocalizedString>

#include <QTextStream>
#include <QIODevice>

using Tellico::Import::GRS1Importer;
GRS1Importer::TagMap* GRS1Importer::s_tagMap = nullptr;

// static
void GRS1Importer::initTagMap() {
  if(!s_tagMap) {
    s_tagMap =  new TagMap();
    // BT is special and is handled separately
    s_tagMap->insert(TagPair(2, 1), QStringLiteral("title"));
    s_tagMap->insert(TagPair(2, 2), QStringLiteral("author"));
    s_tagMap->insert(TagPair(2, 4), QStringLiteral("year"));
    s_tagMap->insert(TagPair(2, 7), QStringLiteral("publisher"));
    s_tagMap->insert(TagPair(2, 31), QStringLiteral("publisher"));
    s_tagMap->insert(TagPair(2, 20), QStringLiteral("language"));
    s_tagMap->insert(TagPair(2, 21), QStringLiteral("keyword"));
    s_tagMap->insert(TagPair(3, QLatin1String("isbn/issn")), QStringLiteral("isbn"));
    s_tagMap->insert(TagPair(3, QLatin1String("isbn")), QStringLiteral("isbn"));
    s_tagMap->insert(TagPair(3, QLatin1String("notes")), QStringLiteral("note"));
    s_tagMap->insert(TagPair(3, QLatin1String("note")), QStringLiteral("note"));
    s_tagMap->insert(TagPair(3, QLatin1String("series")), QStringLiteral("series"));
    s_tagMap->insert(TagPair(3, QLatin1String("physical description")), QStringLiteral("note"));
    s_tagMap->insert(TagPair(3, QLatin1String("subtitle")), QStringLiteral("subtitle"));
  }
}

GRS1Importer::GRS1Importer(const QString& text_) : TextImporter(text_) {
  initTagMap();
}

bool GRS1Importer::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr GRS1Importer::collection() {
  Data::CollPtr coll(new Data::BibtexCollection(true));

  coll->addField(Data::Field::createDefaultField(Data::Field::IsbnField));

  Data::FieldPtr f(new Data::Field(QStringLiteral("language"), i18n("Language")));
  f->setCategory(i18n("Publishing"));
  f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowGrouped | Data::Field::AllowMultiple);
  coll->addField(f);

  Data::EntryPtr e(new Data::Entry(coll));
  bool empty = true;

  // in format "(tag, tag) value"
  static const QRegularExpression rx(QStringLiteral("^\\s*\\((\\d+),\\s*(.+)\\s*\\)\\s*(.+)\\s*$"));
  static const QRegularExpression dateRx(QStringLiteral(",[^,]*\\d{3,4}[^,]*")); // remove dates from authors
  static const QRegularExpression pubRx(QStringLiteral("([^:]+):([^,]+),?")); // split location and publisher

  bool ok;
  int n;
  QVariant v;
  QString tmp, field, val, str = text();
  if(str.isEmpty()) {
    return Data::CollPtr();
  }
  QTextStream t(&str, QIODevice::ReadOnly);
  for(QString line = t.readLine(); !t.atEnd(); line = t.readLine()) {
//    myDebug() << line;
    QRegularExpressionMatch m = rx.match(line);
    if(!m.hasMatch()) {
      continue;
    }
    n = m.captured(1).toInt();
    v = m.captured(2).toInt(&ok);
    if(!ok) {
      v = m.captured(2).toLower();
    }
    field = (*s_tagMap)[TagPair(n, v)];
    if(field.isEmpty()) {
      continue;
    }
//    myDebug() << "field is " << field;
    // assume if multiple values, it's allowed
    val = m.captured(3).trimmed();
    if(val.isEmpty()) {
      continue;
    }
    empty = false;
    if(field == QLatin1String("title")) {
      val = val.section(QLatin1Char('/'), 0, 0).trimmed(); // only take portion of title before slash
    } else if(field == QLatin1String("author")) {
      val.remove(dateRx);
    } else if(field == QLatin1String("publisher")) {
      QRegularExpressionMatch pubMatch = pubRx.match(val);
      if(pubMatch.hasMatch()) {
        e->setField(QStringLiteral("address"), pubMatch.captured(1));
        val = pubMatch.captured(2);
      }
    }

    tmp = e->field(field);
    if(!tmp.isEmpty()) {
      tmp += FieldFormat::delimiterString();
    }
    e->setField(field, tmp + val);
  }

  if(!empty) {
    coll->addEntries(e);
  }
  return coll;
}
