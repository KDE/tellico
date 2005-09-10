/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bibtexcollection.h"
#include "../collectionfactory.h"
#include "../latin1literal.h"
#include "../translators/bibtexhandler.h"

#include <klocale.h>
#include <kdebug.h>

#include <qfile.h>

using Tellico::Data::BibtexCollection;

namespace {
  static const char* bibtex_general = I18N_NOOP("General");
  static const char* bibtex_publishing = I18N_NOOP("Publishing");
  static const char* bibtex_misc = I18N_NOOP("Miscellaneous");
}

BibtexCollection::BibtexCollection(bool addFields_, const QString& title_ /*=null*/)
   : Collection(title_, CollectionFactory::entryName(Bibtex), i18n("Entries")) {
  setTitle(title_.isNull() ? i18n("Bibliography") : title_);
  if(addFields_) {
    addFields(defaultFields());
  }
  setDefaultGroupField(QString::fromLatin1("author"));

  // Bibtex has some default macros for the months
  addMacro(QString::fromLatin1("jan"), QString::null);
  addMacro(QString::fromLatin1("feb"), QString::null);
  addMacro(QString::fromLatin1("mar"), QString::null);
  addMacro(QString::fromLatin1("apr"), QString::null);
  addMacro(QString::fromLatin1("may"), QString::null);
  addMacro(QString::fromLatin1("jun"), QString::null);
  addMacro(QString::fromLatin1("jul"), QString::null);
  addMacro(QString::fromLatin1("aug"), QString::null);
  addMacro(QString::fromLatin1("sep"), QString::null);
  addMacro(QString::fromLatin1("oct"), QString::null);
  addMacro(QString::fromLatin1("nov"), QString::null);
  addMacro(QString::fromLatin1("dec"), QString::null);
}

Tellico::Data::FieldVec BibtexCollection::defaultFields() {
  FieldVec list;
  Field* field;

/******************* General ****************************/
  field = new Field(QString::fromLatin1("title"), i18n("Title"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("title"));
  field->setCategory(i18n("General"));
  field->setFlags(Field::NoDelete);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  QStringList types;
  types << QString::fromLatin1("article") << QString::fromLatin1("book")
        << QString::fromLatin1("booklet") << QString::fromLatin1("inbook")
        << QString::fromLatin1("incollection") << QString::fromLatin1("inproceedings")
        << QString::fromLatin1("manual") << QString::fromLatin1("mastersthesis")
        << QString::fromLatin1("misc") << QString::fromLatin1("phdthesis")
        << QString::fromLatin1("proceedings") << QString::fromLatin1("techreport")
        << QString::fromLatin1("unpublished") << QString::fromLatin1("periodical")
        << QString::fromLatin1("conference");
  field = new Field(QString::fromLatin1("entry-type"), i18n("Entry Type"), types);
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("entry-type"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowGrouped | Field::NoDelete);
  field->setDescription(i18n("These entry types are specific to bibtex. See the bibtex documentation."));
  list.append(field);

  field = new Field(QString::fromLatin1("author"), i18n("Author"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("author"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("bibtex-key"), i18n("Bibtex Key"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("key"));
  field->setCategory(i18n("General"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QString::fromLatin1("booktitle"), i18n("Book Title"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("booktitle"));
  field->setCategory(i18n(bibtex_general));
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("editor"), i18n("Editor"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("editor"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatName);
  list.append(field);

  field = new Field(QString::fromLatin1("organization"), i18n("Organization"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("organization"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

//  field = new Field(QString::fromLatin1("institution"), i18n("Institution"));
//  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("institution"));
//  field->setCategory(i18n(bibtex_general));
//  field->setFlags(Field::AllowDelete);
//  field->setFormatFlag(Field::FormatTitle);
//  list.append(field);

/******************* Publishing ****************************/
  field = new Field(QString::fromLatin1("publisher"), i18n("Publisher"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("publisher"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  field = new Field(QString::fromLatin1("address"), i18n("Address"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("address"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QString::fromLatin1("edition"), i18n("Edition"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("edition"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion);
  list.append(field);

  // don't make it a nuumber, it could have latex processing commands in it
  field = new Field(QString::fromLatin1("pages"), i18n("Pages"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("pages"));
  field->setCategory(i18n(bibtex_publishing));
  list.append(field);

  field = new Field(QString::fromLatin1("year"), i18n("Year"), Field::Number);
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("year"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

//  field = new Field(QString::fromLatin1("isbn"), i18n("ISBN#"));
//  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("isbn"));
//  field->setCategory(i18n(bibtex_publishing));
//  field->setDescription(i18n("International Standard Book Number"));
//  list.append(field);

  field = new Field(QString::fromLatin1("journal"), i18n("Journal"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("journal"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatPlain);
  list.append(field);

  // could make this a string list, but since bibtex import could have funky values
  // keep it an editbox
  field = new Field(QString::fromLatin1("month"), i18n("Month"), Field::Number);
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("month"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion);
  list.append(field);

  field = new Field(QString::fromLatin1("number"), i18n("Number"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("number"));
  field->setCategory(i18n(bibtex_publishing));
  list.append(field);

  field = new Field(QString::fromLatin1("howpublished"), i18n("How Published"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("howpublished"));
  field->setCategory(i18n(bibtex_publishing));
  list.append(field);

//  field = new Field(QString::fromLatin1("school"), i18n("School"));
//  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("school"));
//  field->setCategory(i18n(bibtex_publishing));
//  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
//  list.append(field);

/******************* Classification ****************************/
  field = new Field(QString::fromLatin1("chapter"), i18n("Chapter"), Field::Number);
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("chapter"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  field = new Field(QString::fromLatin1("series"), i18n("Series"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("series"));
  field->setCategory(i18n(bibtex_misc));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatFlag(Field::FormatTitle);
  list.append(field);

  field = new Field(QString::fromLatin1("volume"), i18n("Volume"), Field::Number);
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("volume"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  field = new Field(QString::fromLatin1("crossref"), i18n("Cross-Reference"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("crossref"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

//  field = new Field(QString::fromLatin1("annote"), i18n("Annotation"));
//  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("annote"));
//  field->setCategory(i18n(bibtex_misc));
//  list.append(field);

  field = new Field(QString::fromLatin1("note"), i18n("Notes"));
  field->setProperty(QString::fromLatin1("bibtex"), QString::fromLatin1("note"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  return list;
}

bool BibtexCollection::addField(Field* field_) {
  if(!field_) {
    return false;
  }
//  kdDebug() << "BibtexCollection::addField()" << endl;
  bool success = Collection::addField(field_);
  if(success) {
    QString bibtex = field_->property(QString::fromLatin1("bibtex"));
    if(!bibtex.isEmpty()) {
      m_bibtexFieldDict.insert(bibtex, field_);
    }
  }
  return success;
}

bool BibtexCollection::modifyField(Field* newField_) {
  if(!newField_) {
    return false;
  }
//  kdDebug() << "BibtexCollection::modifyField()" << endl;
  bool success = Collection::modifyField(newField_);
  Field* oldField = fieldByName(newField_->name());
  QString oldBibtex = oldField->property(QString::fromLatin1("bibtex"));
  QString newBibtex = newField_->property(QString::fromLatin1("bibtex"));
  if(!oldBibtex.isEmpty()) {
    success &= m_bibtexFieldDict.remove(oldBibtex);
  }
  if(!newBibtex.isEmpty()) {
    oldField->setProperty(QString::fromLatin1("bibtex"), newBibtex);
    m_bibtexFieldDict.insert(newBibtex, oldField);
  }
  return success;
}

bool BibtexCollection::deleteField(Field* field_, bool force_) {
  if(!field_) {
    return false;
  }
//  kdDebug() << "BibtexCollection::deleteField()" << endl;
  bool success = true;
  QString bibtex = field_->property(QString::fromLatin1("bibtex"));
  if(!bibtex.isEmpty()) {
    success &= m_bibtexFieldDict.remove(bibtex);
  }
  return success && Collection::removeField(field_, force_);
}

Tellico::Data::Field* const BibtexCollection::fieldByBibtexName(const QString& bibtex_) const {
  return m_bibtexFieldDict.isEmpty() ? 0 : m_bibtexFieldDict.find(bibtex_);
}

// static
BibtexCollection* BibtexCollection::convertBookCollection(const Collection* coll_) {
  const QString bibtex = QString::fromLatin1("bibtex");
  BibtexCollection* coll = new BibtexCollection(false, coll_->title());
  FieldVec fields = coll_->fields();
  for(FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
    Field* field = fIt->clone();
    QString name = field->name();

    // this first group has bibtex field names the same as their own field name
    if(name == Latin1Literal("title")
       || name == Latin1Literal("author")
       || name == Latin1Literal("editor")
       || name == Latin1Literal("edition")
       || name == Latin1Literal("publisher")
       || name == Latin1Literal("isbn")
       || name == Latin1Literal("lccn")
       || name == Latin1Literal("url")
       || name == Latin1Literal("language")
       || name == Latin1Literal("pages")
       || name == Latin1Literal("series")) {
      field->setProperty(bibtex, name);
    } else if(name == Latin1Literal("series_num")) {
      field->setProperty(bibtex, QString::fromLatin1("number"));
    } else if(name == Latin1Literal("pur_price")) {
      field->setProperty(bibtex, QString::fromLatin1("price"));
    } else if(name == Latin1Literal("cr_year")) {
      field->setProperty(bibtex, QString::fromLatin1("year"));
    } else if(name == Latin1Literal("bibtex-id")) {
      field->setProperty(bibtex, QString::fromLatin1("key"));
    } else if(name == Latin1Literal("keyword")) {
      field->setProperty(bibtex, QString::fromLatin1("keywords"));
    } else if(name == Latin1Literal("comments")) {
      field->setProperty(bibtex, QString::fromLatin1("note"));
    }
    coll->addField(field);
  }

  // also need to add required fields
  FieldVec vec = defaultFields();
  for(FieldVec::Iterator it = vec.begin(); it != vec.end(); ++it) {
    if(coll->fieldByName(it->name()) == 0 && (it->flags() & Field::NoDelete)) {
      // but don't add a Bibtex Key if there's already a bibtex-id
      if(it->property(bibtex) != Latin1Literal("key")
         || coll->fieldByName(QString::fromLatin1("bibtex-id")) == 0) {
        coll->addField(it);
      }
    }
  }

  // set the entry-type to book
  Field* field = coll->fieldByBibtexName(QString::fromLatin1("entry-type"));
  QString entryTypeName;
  if(field) {
    entryTypeName = field->name();
  }

  for(EntryVec::ConstIterator entryIt = coll_->entries().begin(); entryIt != coll_->entries().end(); ++entryIt) {
    Data::Entry* entry = new Entry(*entryIt, coll);
    coll->addEntry(entry);
    if(!entryTypeName.isEmpty()) {
      entry->setField(entryTypeName, QString::fromLatin1("book"));
    }
  }

  return coll;
}

QStringList BibtexCollection::bibtexKeys(const EntryVec& entries_) const {
  QStringList keys;
  for(Data::EntryVec::ConstIterator it = entries_.begin(); it != entries_.end(); ++it) {
    keys << BibtexHandler::bibtexKey(it);
  }
  return keys;
}

void BibtexCollection::citeEntries(QFile& lyxpipe_, const EntryVec& entries_) const {
  if(entries_.isEmpty()) {
    return;
  }
//  kdDebug() << "BibtexCollection::citeEntries()" << endl;
  const QStringList keys = bibtexKeys(entries_);

  QTextStream ts(&lyxpipe_);
//  ts << "LYXSRV:tellico:hello\n";
  ts << "LYXCMD:tellico:citation-insert:";
  QStringList::ConstIterator end = keys.end();
  for(QStringList::ConstIterator it = keys.begin(); it != end; ++it) {
    ts << (*it).latin1();
    if(it != keys.fromLast()) {
      // pybliographer uses comma-space, and pyblink expects the space there
      ts << ", ";
    }
  }
  ts << "\n";
//  ts << "LYXSRV:tellico:bye\n";
  lyxpipe_.flush();
}

#include "bibtexcollection.moc"
