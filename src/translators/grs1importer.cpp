/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
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
#include "../latin1literal.h"
#include "../tellico_debug.h"

using Tellico::Import::GRS1Importer;
GRS1Importer::TagMap* GRS1Importer::s_tagMap = 0;

// static
void GRS1Importer::initTagMap() {
  if(!s_tagMap) {
    s_tagMap =  new TagMap();
    // BT is special and is handled separately
    s_tagMap->insert(TagPair(2, 1), QString::fromLatin1("title"));
    s_tagMap->insert(TagPair(2, 2), QString::fromLatin1("author"));
    s_tagMap->insert(TagPair(2, 4), QString::fromLatin1("year"));
    s_tagMap->insert(TagPair(2, 7), QString::fromLatin1("publisher"));
    s_tagMap->insert(TagPair(2, 31), QString::fromLatin1("publisher"));
    s_tagMap->insert(TagPair(2, 20), QString::fromLatin1("language"));
    s_tagMap->insert(TagPair(2, 21), QString::fromLatin1("keyword"));
    s_tagMap->insert(TagPair(3, QString::fromLatin1("isbn/issn")), QString::fromLatin1("isbn"));
    s_tagMap->insert(TagPair(3, QString::fromLatin1("isbn")), QString::fromLatin1("isbn"));
    s_tagMap->insert(TagPair(3, QString::fromLatin1("notes")), QString::fromLatin1("note"));
    s_tagMap->insert(TagPair(3, QString::fromLatin1("note")), QString::fromLatin1("note"));
    s_tagMap->insert(TagPair(3, QString::fromLatin1("series")), QString::fromLatin1("series"));
    s_tagMap->insert(TagPair(3, QString::fromLatin1("physical description")), QString::fromLatin1("note"));
    s_tagMap->insert(TagPair(3, QString::fromLatin1("subtitle")), QString::fromLatin1("subtitle"));
  }
}

GRS1Importer::GRS1Importer(const QString& text_) : TextImporter(text_) {
  initTagMap();
}

bool GRS1Importer::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr GRS1Importer::collection() {
  Data::CollPtr coll = new Data::BibtexCollection(true);

  Data::FieldPtr f = new Data::Field(QString::fromLatin1("isbn"), i18n("ISBN#"));
  f->setCategory(i18n("Publishing"));
  f->setDescription(i18n("International Standard Book Number"));
  coll->addField(f);

  f = new Data::Field(QString::fromLatin1("keyword"), i18n("Keywords"));
  f->setCategory(i18n("Classification"));
  f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
  coll->addField(f);

  f = new Data::Field(QString::fromLatin1("language"), i18n("Language"));
  f->setCategory(i18n("Publishing"));
  f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowGrouped | Data::Field::AllowMultiple);
  coll->addField(f);

  Data::EntryPtr e = new Data::Entry(coll);
  bool empty = true;

  // in format "(tag, tag) value"
  QRegExp rx(QString::fromLatin1("\\s*\\((\\d+),\\s*(.+)\\s*\\)\\s*(.+)\\s*"));
//  rx.setMinimal(true);
  QRegExp dateRx(QString::fromLatin1(",[^,]*\\d{3,4}[^,]*")); // remove dates from authors
  QRegExp pubRx(QString::fromLatin1("([^:]+):([^,]+),?")); // split location and publisher

  bool ok;
  int n;
  QVariant v;
  QString tmp, field, val, str = text();
  if(str.isEmpty()) {
    return 0;
  }
  QTextStream t(&str, IO_ReadOnly);
  for(QString line = t.readLine(); !line.isNull(); line = t.readLine()) {
//    myDebug() << line << endl;
    if(!rx.exactMatch(line)) {
      continue;
    }
    n = rx.cap(1).toInt();
    v = rx.cap(2).toInt(&ok);
    if(!ok) {
      v = rx.cap(2).lower();
    }
    field = (*s_tagMap)[TagPair(n, v)];
    if(field.isEmpty()) {
      continue;
    }
//    myDebug() << "field is " << field << endl;
    // assume if multiple values, it's allowed
    val = rx.cap(3).stripWhiteSpace();
    if(val.isEmpty()) {
      continue;
    }
    empty = false;
    if(field == Latin1Literal("title")) {
      val = val.section('/', 0, 0).stripWhiteSpace(); // only take portion of title before slash
    } else if(field == Latin1Literal("author")) {
      val.replace(dateRx, QString::null);
    } else if(field == Latin1Literal("publisher")) {
      int pos = val.find(pubRx);
      if(pos > -1) {
        e->setField(QString::fromLatin1("address"), pubRx.cap(1));
        val = pubRx.cap(2);
      }
    }

    tmp = e->field(field);
    if(!tmp.isEmpty()) {
      tmp += QString::fromLatin1("; ");
    }
    e->setField(field, tmp + val);
  }

  if(!empty) {
    coll->addEntries(e);
  }
  return coll;
}

#include "grs1importer.moc"
