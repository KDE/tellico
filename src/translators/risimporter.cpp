/***************************************************************************
    copyright            : (C) 2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "risimporter.h"
#include "../collections/bibtexcollection.h"
#include "../latin1literal.h"
#include "../tellico_kernel.h"

#include <klocale.h>

#include <qdict.h>
#include <qregexp.h>
#include <qmap.h>

QMap<QString, QString>* Tellico::Import::RISImporter::s_tagMap = 0;
QMap<QString, QString>* Tellico::Import::RISImporter::s_typeMap = 0;

using Tellico::Import::RISImporter;

// static
void RISImporter::initTagMap() {
  if(!s_tagMap) {
    s_tagMap =  new QMap<QString, QString>();
    // BT is special and is handled separately
    s_tagMap->insert(QString::fromLatin1("TY"), QString::fromLatin1("entry-type"));
    s_tagMap->insert(QString::fromLatin1("ID"), QString::fromLatin1("bibtex-key"));
    s_tagMap->insert(QString::fromLatin1("T1"), QString::fromLatin1("title"));
    s_tagMap->insert(QString::fromLatin1("TI"), QString::fromLatin1("title"));
    s_tagMap->insert(QString::fromLatin1("T2"), QString::fromLatin1("booktitle"));
    s_tagMap->insert(QString::fromLatin1("A1"), QString::fromLatin1("author"));
    s_tagMap->insert(QString::fromLatin1("AU"), QString::fromLatin1("author"));
    s_tagMap->insert(QString::fromLatin1("ED"), QString::fromLatin1("editor"));
    s_tagMap->insert(QString::fromLatin1("YR"), QString::fromLatin1("year"));
    s_tagMap->insert(QString::fromLatin1("PY"), QString::fromLatin1("year"));
    s_tagMap->insert(QString::fromLatin1("N1"), QString::fromLatin1("note"));
    s_tagMap->insert(QString::fromLatin1("AB"), QString::fromLatin1("abstract")); // should be note?
    s_tagMap->insert(QString::fromLatin1("N2"), QString::fromLatin1("abstract"));
    s_tagMap->insert(QString::fromLatin1("KW"), QString::fromLatin1("keyword"));
    s_tagMap->insert(QString::fromLatin1("JF"), QString::fromLatin1("journal"));
    s_tagMap->insert(QString::fromLatin1("JO"), QString::fromLatin1("journal"));
    s_tagMap->insert(QString::fromLatin1("JA"), QString::fromLatin1("journal"));
    s_tagMap->insert(QString::fromLatin1("VL"), QString::fromLatin1("number"));
    s_tagMap->insert(QString::fromLatin1("IS"), QString::fromLatin1("number"));
    s_tagMap->insert(QString::fromLatin1("PB"), QString::fromLatin1("publisher"));
    s_tagMap->insert(QString::fromLatin1("SN"), QString::fromLatin1("isbn"));
    s_tagMap->insert(QString::fromLatin1("AD"), QString::fromLatin1("address"));
    s_tagMap->insert(QString::fromLatin1("UR"), QString::fromLatin1("url"));
    s_tagMap->insert(QString::fromLatin1("L1"), QString::fromLatin1("pdf"));
  }
}

// static
void RISImporter::initTypeMap() {
  if(!s_typeMap) {
    s_typeMap = new QMap<QString, QString>();
    // leave capitalized, except for bibtex types
    s_typeMap->insert(QString::fromLatin1("ABST"),   QString::fromLatin1("Abstract"));
    s_typeMap->insert(QString::fromLatin1("ADVS"),   QString::fromLatin1("Audiovisual material"));
    s_typeMap->insert(QString::fromLatin1("ART"),    QString::fromLatin1("Art Work"));
    s_typeMap->insert(QString::fromLatin1("BILL"),   QString::fromLatin1("Bill/Resolution"));
    s_typeMap->insert(QString::fromLatin1("BOOK"),   QString::fromLatin1("book")); // bibtex
    s_typeMap->insert(QString::fromLatin1("CASE"),   QString::fromLatin1("Case"));
    s_typeMap->insert(QString::fromLatin1("CHAP"),   QString::fromLatin1("Book chapter")); // == "inbook" ?
    s_typeMap->insert(QString::fromLatin1("COMP"),   QString::fromLatin1("Computer program"));
    s_typeMap->insert(QString::fromLatin1("CONF"),   QString::fromLatin1("proceedings")); // == "conference" ?
    s_typeMap->insert(QString::fromLatin1("CTLG"),   QString::fromLatin1("Catalog"));
    s_typeMap->insert(QString::fromLatin1("DATA"),   QString::fromLatin1("Data file"));
    s_typeMap->insert(QString::fromLatin1("ELEC"),   QString::fromLatin1("Electronic Citation"));
    s_typeMap->insert(QString::fromLatin1("GEN"),    QString::fromLatin1("Generic"));
    s_typeMap->insert(QString::fromLatin1("HEAR"),   QString::fromLatin1("Hearing"));
    s_typeMap->insert(QString::fromLatin1("ICOMM"),  QString::fromLatin1("Internet Communication"));
    s_typeMap->insert(QString::fromLatin1("INPR"),   QString::fromLatin1("In Press"));
    s_typeMap->insert(QString::fromLatin1("JFULL"),  QString::fromLatin1("Journal (full)")); // = "periodical" ?
    s_typeMap->insert(QString::fromLatin1("JOUR"),   QString::fromLatin1("Journal"));
    s_typeMap->insert(QString::fromLatin1("MAP"),    QString::fromLatin1("Map"));
    s_typeMap->insert(QString::fromLatin1("MGZN"),   QString::fromLatin1("article")); // bibtex
    s_typeMap->insert(QString::fromLatin1("MPCT"),   QString::fromLatin1("Motion picture"));
    s_typeMap->insert(QString::fromLatin1("MUSIC"),  QString::fromLatin1("Music score"));
    s_typeMap->insert(QString::fromLatin1("NEWS"),   QString::fromLatin1("Newspaper"));
    s_typeMap->insert(QString::fromLatin1("PAMP"),   QString::fromLatin1("Pamphlet")); // = "booklet" ?
    s_typeMap->insert(QString::fromLatin1("PAT"),    QString::fromLatin1("Patent"));
    s_typeMap->insert(QString::fromLatin1("PCOMM"),  QString::fromLatin1("Personal communication"));
    s_typeMap->insert(QString::fromLatin1("RPRT"),   QString::fromLatin1("Report")); // = "techreport" ?
    s_typeMap->insert(QString::fromLatin1("SER"),    QString::fromLatin1("Serial (BookMonograph)"));
    s_typeMap->insert(QString::fromLatin1("SLIDE"),  QString::fromLatin1("Slide"));
    s_typeMap->insert(QString::fromLatin1("SOUND"),  QString::fromLatin1("Sound recording"));
    s_typeMap->insert(QString::fromLatin1("STAT"),   QString::fromLatin1("Statute"));
    s_typeMap->insert(QString::fromLatin1("THES"),   QString::fromLatin1("phdthesis")); // "mastersthesis" ?
    s_typeMap->insert(QString::fromLatin1("UNBILL"), QString::fromLatin1("Unenacted bill/resolution"));
    s_typeMap->insert(QString::fromLatin1("UNPB"),   QString::fromLatin1("unpublished")); // bibtex
    s_typeMap->insert(QString::fromLatin1("VIDEO"),  QString::fromLatin1("Video recording"));
  }
}

RISImporter::RISImporter(const KURL& url_) : Tellico::Import::TextImporter(url_), m_coll(0) {
  initTagMap();
  initTypeMap();
}

Tellico::Data::Collection* RISImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  m_coll = new Data::BibtexCollection(true);

  QDict<Data::Field> risFields;

  // need to know if any extended properties in current collection point to RIS
  // if so, add to collection
  const Data::Collection* const m_currColl = Kernel::self()->collection();
  for(Data::FieldListIterator it(m_currColl->fieldList()); it.current(); ++it) {
    // continue if property is empty
    QString ris = it.current()->property(QString::fromLatin1("RIS"));
    if(ris.isEmpty()) {
      continue;
    }
    // if current collection has one with the same name, set the property
    Data::Field* f = m_coll->fieldByName(it.current()->name());
    if(f) {
      f->setProperty(QString::fromLatin1("RIS"), ris);
      risFields.insert(ris, f);
    } else {
      // else, add it
      m_coll->addField(it.current()->clone());
      risFields.insert(ris, m_coll->fieldByName(it.current()->name()));
    }
  }

  QString str = text();
  QTextIStream t(&str);

  bool deleteEntry = true;
  // TODO: this might not be the smartest thing since it parses the input twice
  const int totalChars = str.length();
  int j = 0;
  Data::Entry* entry = new Data::Entry(m_coll);
  // technically, the spec requires a space immediately after the hyphen
  // however, at least one website (Springer) outputs RIS with no space after the final "ER -"
  QRegExp rx(QString::fromLatin1("^(\\w\\w)\\s\\s-\\s?(.*)$"));
  QString currLine, nextLine;
  for(currLine = t.readLine(); !currLine.isNull(); currLine = nextLine, j += currLine.length()) {
    nextLine = t.readLine();
    rx.search(currLine);
    QString tag = rx.cap(1);
    QString value = rx.cap(2);
    if(tag.isEmpty()) {
      continue;
    }

    // if the next line is not empty and does not match start regexp, append to value
    while(!nextLine.isEmpty() && nextLine.find(rx) == -1) {
      value += nextLine.stripWhiteSpace();
      nextLine = t.readLine();
    }

    // every entry ends with "ER"
    if(tag == Latin1Literal("ER")) {
      m_coll->addEntry(entry);
      entry = new Data::Entry(m_coll);
      deleteEntry = true;
      continue;
    } else if(tag == Latin1Literal("TY")) { // for entry-type, switch it to normalized type name
      value = (*s_typeMap)[value];
      // fall back to value if empty
      if(value.isEmpty()) {
        value = rx.cap(2);
      }
    } else if(tag == Latin1Literal("YR") || tag == Latin1Literal("PY")) {  // for now, just grab the year
      value = value.section('/', 0, 0);
    }

    // the lookup scheme is:
    // 1. any field has an RIS property that matches the tag name
    // 2. default field mapping tag -> field name
    Data::Field* f = risFields.find(tag);
    if(!f) {
      // special case for BT
      // primary title for books, secondary for everything else
      if(tag == Latin1Literal("BT")) {
        if(entry->field(QString::fromLatin1("entry-type")) == Latin1Literal("book")) {
          f = m_coll->fieldByName(QString::fromLatin1("title"));
        } else {
          f = m_coll->fieldByName(QString::fromLatin1("booktitle"));
        }
      } else {
        f = fieldByTag(tag);
      }
    }
    if(!f) {
      continue;
    }

    // harmless for non-choice fields
    // for entry-type, want it in lower case
    f->addAllowed(value);
    // if the field can have multiple values, append current values to new value
    if((f->flags() & Data::Field::AllowMultiple) && !entry->field(f->name()).isEmpty()) {
      value.prepend(entry->field(f->name()) + QString::fromLatin1("; "));
    }
    entry->setField(f->name(), value);
    deleteEntry = false;

    if(j%s_stepSize == 0) {
      emit signalFractionDone(static_cast<float>(j)/static_cast<float>(totalChars));
    }
  }

  if(deleteEntry) {
    delete entry;
  }
  return m_coll;
}

Tellico::Data::Field* RISImporter::fieldByTag(const QString& tag_) {
  Data::Field* f = 0;
  const QString& fieldTag = (*s_tagMap)[tag_];
  if(!fieldTag.isEmpty()) {
    f = m_coll->fieldByName(fieldTag);
    if(f) {
      f->setProperty(QString::fromLatin1("ris"), tag_);
      return f;
    }
  }

  // add some non-default fields if not already there
  if(tag_== Latin1Literal("AB") || tag_== Latin1Literal("N2")) {
    f = new Data::Field(QString::fromLatin1("abstract"), i18n("Abstract"), Data::Field::Para);
    f->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("abstract"));
    f->setProperty(QString::fromLatin1("ris"), QString::fromLatin1("AB"));
    f->setCategory(i18n("Miscellaneous"));
  } else if(tag_== Latin1Literal("KW")) {
    f = new Data::Field(QString::fromLatin1("keyword"), i18n("Keywords"));
    f->setProperty(QString::fromLatin1("ris"), QString::fromLatin1("KW"));
    f->setCategory(i18n("Classification"));
    f->setFlags(Data::Field::AllowCompletion | Data::Field::AllowMultiple | Data::Field::AllowGrouped);
  } else if(tag_== Latin1Literal("SN")) {
    f = new Data::Field(QString::fromLatin1("isbn"), i18n("ISBN#"));
    f->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("isbn"));
    f->setProperty(QString::fromLatin1("ris"), QString::fromLatin1("SN"));
    f->setCategory(i18n("Publishing"));
    f->setDescription(i18n("International Standard Book Number"));
  } else if(tag_== Latin1Literal("UR")) {
    f = new Data::Field(QString::fromLatin1("url"), i18n("URL"), Data::Field::URL);
    f->setProperty(QString::fromLatin1("ris"), QString::fromLatin1("UR"));
    f->setCategory(i18n("Unknown"));
  } else if(tag_== Latin1Literal("L1")) {
    f = new Data::Field(QString::fromLatin1("pdf"), i18n("PDF"), Data::Field::URL);
    f->setProperty(QString::fromLatin1("ris"), QString::fromLatin1("L1"));
    f->setCategory(i18n("Unknown"));
  }
  m_coll->addField(f);
  return f;
}

#include "risimporter.moc"
