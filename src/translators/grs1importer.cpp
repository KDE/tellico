/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "grs1importer.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../tellico_debug.h"

#include <QTextStream>

using Tellico::Import::GRS1Importer;
GRS1Importer::TagMap* GRS1Importer::s_tagMap = 0;

// static
void GRS1Importer::initTagMap() {
  if(!s_tagMap) {
    s_tagMap =  new TagMap();
    // BT is special and is handled separately
    s_tagMap->insert(TagPair(2, 1), QLatin1String("title"));
    s_tagMap->insert(TagPair(2, 2), QLatin1String("author"));
    s_tagMap->insert(TagPair(2, 4), QLatin1String("year"));
    s_tagMap->insert(TagPair(2, 7), QLatin1String("publisher"));
    s_tagMap->insert(TagPair(2, 31), QLatin1String("publisher"));
    s_tagMap->insert(TagPair(2, 20), QLatin1String("language"));
    s_tagMap->insert(TagPair(2, 21), QLatin1String("keyword"));
    s_tagMap->insert(TagPair(3, QLatin1String("isbn/issn")), QLatin1String("isbn"));
    s_tagMap->insert(TagPair(3, QLatin1String("isbn")), QLatin1String("isbn"));
    s_tagMap->insert(TagPair(3, QLatin1String("notes")), QLatin1String("note"));
    s_tagMap->insert(TagPair(3, QLatin1String("note")), QLatin1String("note"));
    s_tagMap->insert(TagPair(3, QLatin1String("series")), QLatin1String("series"));
    s_tagMap->insert(TagPair(3, QLatin1String("physical description")), QLatin1String("note"));
    s_tagMap->insert(TagPair(3, QLatin1String("subtitle")), QLatin1String("subtitle"));
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

  Data::FieldPtr f(new Data::Field(QLatin1String("isbn"), i18n("ISBN#")));
  f->setCategory(i18n("Publishing"));
  f->setDescription(i18n("International Standard Book Number"));
  coll->addField(f);

  f = new Data::Field(QLatin1String("language"), i18n("Language"));
  f->setCategory(i18n("Publishing"));
  f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowGrouped | Data::Field::AllowMultiple);
  coll->addField(f);

  Data::EntryPtr e(new Data::Entry(coll));
  bool empty = true;

  // in format "(tag, tag) value"
  QRegExp rx(QLatin1String("\\s*\\((\\d+),\\s*(.+)\\s*\\)\\s*(.+)\\s*"));
//  rx.setMinimal(true);
  QRegExp dateRx(QLatin1String(",[^,]*\\d{3,4}[^,]*")); // remove dates from authors
  QRegExp pubRx(QLatin1String("([^:]+):([^,]+),?")); // split location and publisher

  bool ok;
  int n;
  QVariant v;
  QString tmp, field, val, str = text();
  if(str.isEmpty()) {
    return Data::CollPtr();
  }
  QTextStream t(&str, QIODevice::ReadOnly);
  for(QString line = t.readLine(); !t.atEnd(); line = t.readLine()) {
//    myDebug() << line << endl;
    if(!rx.exactMatch(line)) {
      continue;
    }
    n = rx.cap(1).toInt();
    v = rx.cap(2).toInt(&ok);
    if(!ok) {
      v = rx.cap(2).toLower();
    }
    field = (*s_tagMap)[TagPair(n, v)];
    if(field.isEmpty()) {
      continue;
    }
//    myDebug() << "field is " << field << endl;
    // assume if multiple values, it's allowed
    val = rx.cap(3).trimmed();
    if(val.isEmpty()) {
      continue;
    }
    empty = false;
    if(field == QLatin1String("title")) {
      val = val.section(QLatin1Char('/'), 0, 0).trimmed(); // only take portion of title before slash
    } else if(field == QLatin1String("author")) {
      val.remove(dateRx);
    } else if(field == QLatin1String("publisher")) {
      if(pubRx.indexIn(val) > -1) {
        e->setField(QLatin1String("address"), pubRx.cap(1));
        val = pubRx.cap(2);
      }
    }

    tmp = e->field(field);
    if(!tmp.isEmpty()) {
      tmp += QLatin1String("; ");
    }
    e->setField(field, tmp + val);
  }

  if(!empty) {
    coll->addEntries(e);
  }
  return coll;
}

#include "grs1importer.moc"
