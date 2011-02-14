/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "bibtexcollection.h"
#include "../entrycomparison.h"
#include "../translators/bibtexhandler.h"
#include "../fieldformat.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kstringhandler.h>

using namespace Tellico;
using Tellico::Data::BibtexCollection;

namespace {
  static const char* bibtex_general = I18N_NOOP("General");
  static const char* bibtex_publishing = I18N_NOOP("Publishing");
  static const char* bibtex_misc = I18N_NOOP("Miscellaneous");
}

BibtexCollection::BibtexCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("Bibliography") : title_) {
  setDefaultGroupField(QLatin1String("author"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }

  // Bibtex has some default macros for the months
  addMacro(QLatin1String("jan"), QString());
  addMacro(QLatin1String("feb"), QString());
  addMacro(QLatin1String("mar"), QString());
  addMacro(QLatin1String("apr"), QString());
  addMacro(QLatin1String("may"), QString());
  addMacro(QLatin1String("jun"), QString());
  addMacro(QLatin1String("jul"), QString());
  addMacro(QLatin1String("aug"), QString());
  addMacro(QLatin1String("sep"), QString());
  addMacro(QLatin1String("oct"), QString());
  addMacro(QLatin1String("nov"), QString());
  addMacro(QLatin1String("dec"), QString());
}

Tellico::Data::FieldList BibtexCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

/******************* General ****************************/

  field = createDefaultField(TitleField);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("title"));
  list.append(field);

  QStringList types;
  types << QLatin1String("article") << QLatin1String("book")
        << QLatin1String("booklet") << QLatin1String("inbook")
        << QLatin1String("incollection") << QLatin1String("inproceedings")
        << QLatin1String("manual") << QLatin1String("mastersthesis")
        << QLatin1String("misc") << QLatin1String("phdthesis")
        << QLatin1String("proceedings") << QLatin1String("techreport")
        << QLatin1String("unpublished") << QLatin1String("periodical")
        << QLatin1String("conference");
  field = new Field(QLatin1String("entry-type"), i18n("Entry Type"), types);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("entry-type"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowGrouped | Field::NoDelete);
  field->setDescription(i18n("These entry types are specific to bibtex. See the bibtex documentation."));
  list.append(field);

  field = new Field(QLatin1String("author"), i18n("Author"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("author"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QLatin1String("bibtex-key"), i18n("Bibtex Key"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("key"));
  field->setCategory(i18n("General"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QLatin1String("booktitle"), i18n("Book Title"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("booktitle"));
  field->setCategory(i18n(bibtex_general));
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("editor"), i18n("Editor"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("editor"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QLatin1String("organization"), i18n("Organization"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("organization"));
  field->setCategory(i18n(bibtex_general));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

//  field = new Field(QLatin1String("institution"), i18n("Institution"));
//  field->setProperty(QLatin1String("bibtex"), QLatin1String("institution"));
//  field->setCategory(i18n(bibtex_general));
//  field->setFlags(Field::AllowDelete);
//  field->setFormatType(FieldFormat::FormatTitle);
//  list.append(field);

/******************* Publishing ****************************/
  field = new Field(QLatin1String("publisher"), i18n("Publisher"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("publisher"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("address"), i18n("Address"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("address"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("edition"), i18n("Edition"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("edition"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion);
  list.append(field);

  // don't make it a nuumber, it could have latex processing commands in it
  field = new Field(QLatin1String("pages"), i18n("Pages"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("pages"));
  field->setCategory(i18n(bibtex_publishing));
  list.append(field);

  field = new Field(QLatin1String("year"), i18n("Year"), Field::Number);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("year"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("isbn"), i18n("ISBN#"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("isbn"));
  field->setCategory(i18n(bibtex_publishing));
  field->setDescription(i18n("International Standard Book Number"));
  list.append(field);

  field = new Field(QLatin1String("journal"), i18n("Journal"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("journal"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QLatin1String("doi"), i18n("DOI"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("doi"));
  field->setCategory(i18n(bibtex_publishing));
  field->setDescription(i18n("Digital Object Identifier"));
  list.append(field);

  // could make this a string list, but since bibtex import could have funky values
  // keep it an editbox
  field = new Field(QLatin1String("month"), i18n("Month"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("month"));
  field->setCategory(i18n(bibtex_publishing));
  field->setFlags(Field::AllowCompletion);
  list.append(field);

  field = new Field(QLatin1String("number"), i18n("Number"), Field::Number);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("number"));
  field->setCategory(i18n(bibtex_publishing));
  list.append(field);

  field = new Field(QLatin1String("howpublished"), i18n("How Published"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("howpublished"));
  field->setCategory(i18n(bibtex_publishing));
  list.append(field);

//  field = new Field(QLatin1String("school"), i18n("School"));
//  field->setProperty(QLatin1String("bibtex"), QLatin1String("school"));
//  field->setCategory(i18n(bibtex_publishing));
//  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
//  list.append(field);

/******************* Classification ****************************/
  field = new Field(QLatin1String("chapter"), i18n("Chapter"), Field::Number);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("chapter"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  field = new Field(QLatin1String("series"), i18n("Series"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("series"));
  field->setCategory(i18n(bibtex_misc));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QLatin1String("volume"), i18n("Volume"), Field::Number);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("volume"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  field = new Field(QLatin1String("crossref"), i18n("Cross-Reference"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("crossref"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

//  field = new Field(QLatin1String("annote"), i18n("Annotation"));
//  field->setProperty(QLatin1String("bibtex"), QLatin1String("annote"));
//  field->setCategory(i18n(bibtex_misc));
//  list.append(field);

  field = new Field(QLatin1String("keyword"), i18n("Keywords"));
  field->setProperty(QLatin1String("bibtex"), QLatin1String("keywords"));
  field->setCategory(i18n(bibtex_misc));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QLatin1String("url"), i18n("URL"), Field::URL);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("url"));
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  field = new Field(QLatin1String("abstract"), i18n("Abstract"), Field::Para);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("abstract"));
  list.append(field);

  field = new Field(QLatin1String("note"), i18n("Notes"), Field::Para);
  field->setProperty(QLatin1String("bibtex"), QLatin1String("note"));
  list.append(field);

  field = createDefaultField(IDField);
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  field = createDefaultField(CreatedDateField);
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  field = createDefaultField(ModifiedDateField);
  field->setCategory(i18n(bibtex_misc));
  list.append(field);

  return list;
}

bool BibtexCollection::addField(Tellico::Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
//  myDebug();
  bool success = Collection::addField(field_);
  if(success) {
    QString bibtex = field_->property(QLatin1String("bibtex"));
    if(!bibtex.isEmpty()) {
      m_bibtexFieldDict.insert(bibtex, field_.data());
    }
  }
  return success;
}

bool BibtexCollection::modifyField(Tellico::Data::FieldPtr newField_) {
  if(!newField_) {
    return false;
  }
//  myDebug();
  bool success = Collection::modifyField(newField_);
  FieldPtr oldField = fieldByName(newField_->name());
  QString oldBibtex = oldField->property(QLatin1String("bibtex"));
  QString newBibtex = newField_->property(QLatin1String("bibtex"));
  if(!oldBibtex.isEmpty()) {
    success &= (m_bibtexFieldDict.remove(oldBibtex) != 0);
  }
  if(!newBibtex.isEmpty()) {
    oldField->setProperty(QLatin1String("bibtex"), newBibtex);
    m_bibtexFieldDict.insert(newBibtex, oldField.data());
  }
  return success;
}

bool BibtexCollection::removeField(Tellico::Data::FieldPtr field_, bool force_) {
  if(!field_) {
    return false;
  }
//  myDebug();
  bool success = true;
  QString bibtex = field_->property(QLatin1String("bibtex"));
  if(!bibtex.isEmpty()) {
    success &= (m_bibtexFieldDict.remove(bibtex) != 0);
  }
  return success && Collection::removeField(field_, force_);
}

bool BibtexCollection::removeField(const QString& name_, bool force_) {
  return removeField(fieldByName(name_), force_);
}

Tellico::Data::FieldPtr BibtexCollection::fieldByBibtexName(const QString& bibtex_) const {
  return FieldPtr(m_bibtexFieldDict.contains(bibtex_) ? m_bibtexFieldDict.value(bibtex_) : 0);
}

Tellico::Data::EntryPtr BibtexCollection::entryByBibtexKey(const QString& key_) const {
  EntryPtr entry;
  // we do assume unique keys
  foreach(EntryPtr e, entries()) {
    if(e->field(QLatin1String("bibtex-key")) == key_) {
      entry = e;
      break;
    }
  }
  return entry;
}

QString BibtexCollection::prepareText(const QString& text_) const {
  QString text = text_;
  BibtexHandler::cleanText(text);
  return text;
}

// same as BookCollection::sameEntry()
int BibtexCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // equal identifiers are easy, give it a weight of 100
  if(EntryComparison::score(entry1_, entry2_, QLatin1String("isbn"),  this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QLatin1String("lccn"),  this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QLatin1String("doi"),   this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QLatin1String("pmid"),  this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QLatin1String("arxiv"), this) > 0) {
    return 100; // good match
  }
  int res = 3*EntryComparison::score(entry1_, entry2_, QLatin1String("title"), this);
//  if(res == 0) {
//    myDebug() << "different titles for " << entry1_->title() << " vs. "
//              << entry2_->title();
//  }
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("author"),   this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("cr_year"),  this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("pub_year"), this);
  res += EntryComparison::score(entry1_, entry2_, QLatin1String("binding"),  this);
  return res;
}

// static
Tellico::Data::CollPtr BibtexCollection::convertBookCollection(Tellico::Data::CollPtr coll_) {
  const QString bibtex = QLatin1String("bibtex");
  BibtexCollection* coll = new BibtexCollection(false, coll_->title());
  CollPtr collPtr(coll);
  FieldList fields = coll_->fields();
  foreach(FieldPtr fIt, fields) {
    FieldPtr field(new Field(*fIt));

    // if it already has a bibtex property, skip it
    if(!field->property(bibtex).isEmpty()) {
      coll->addField(field);
      continue;
    }

    // be sure to set bibtex property before adding it though
    QString name = field->name();
    // this first group has bibtex field names the same as their own field name
    if(name == QLatin1String("title")
       || name == QLatin1String("author")
       || name == QLatin1String("editor")
       || name == QLatin1String("edition")
       || name == QLatin1String("publisher")
       || name == QLatin1String("isbn")
       || name == QLatin1String("lccn")
       || name == QLatin1String("url")
       || name == QLatin1String("language")
       || name == QLatin1String("pages")
       || name == QLatin1String("series")) {
      field->setProperty(bibtex, name);
    } else if(name == QLatin1String("series_num")) {
      field->setProperty(bibtex, QLatin1String("number"));
    } else if(name == QLatin1String("pur_price")) {
      field->setProperty(bibtex, QLatin1String("price"));
    } else if(name == QLatin1String("cr_year")) {
      field->setProperty(bibtex, QLatin1String("year"));
    } else if(name == QLatin1String("bibtex-id")) {
      field->setProperty(bibtex, QLatin1String("key"));
    } else if(name == QLatin1String("keyword")) {
      field->setProperty(bibtex, QLatin1String("keywords"));
    } else if(name == QLatin1String("comments")) {
      field->setProperty(bibtex, QLatin1String("note"));
    }
    coll->addField(field);
  }

  // also need to add required fields, those with NoDelete set
  foreach(FieldPtr defaultField, coll->defaultFields()) {
    if(!coll->hasField(defaultField->name()) && defaultField->hasFlag(Field::NoDelete)) {
      // but don't add a Bibtex Key if there's already a bibtex-id
      if(defaultField->property(bibtex) != QLatin1String("key")
         || !coll->hasField(QLatin1String("bibtex-id"))) {
        coll->addField(defaultField);
      }
    }
  }

  // set the entry-type to book
  FieldPtr field = coll->fieldByBibtexName(QLatin1String("entry-type"));
  QString entryTypeName;
  if(field) {
    entryTypeName = field->name();
  } else {
    myWarning() << "there must be an entry type field";
  }

  EntryList newEntries;
  foreach(EntryPtr entry, coll_->entries()) {
    EntryPtr newEntry(new Entry(*entry));
    newEntry->setCollection(collPtr);
    if(!entryTypeName.isEmpty()) {
      newEntry->setField(entryTypeName, QLatin1String("book"));
    }
    newEntries.append(newEntry);
  }
  coll->addEntries(newEntries);

  return collPtr;
}

bool BibtexCollection::setFieldValue(Data::EntryPtr entry_, const QString& bibtexField_, const QString& value_, Data::CollPtr existingColl_) {
  Q_ASSERT(entry_->collection()->type() == Collection::Bibtex);
  BibtexCollection* c = static_cast<BibtexCollection*>(entry_->collection().data());
  FieldPtr field = c->fieldByBibtexName(bibtexField_);
  // special-case: "keyword" and "keywords" should be the same field.
  if(!field && bibtexField_ == QLatin1String("keyword")) {
    field = c->fieldByBibtexName(QLatin1String("keywords"));
  }
  if(!field) {
    // it was the case that the default bibliography did not have a bibtex property for keywords
    // so a "keywords" field would get created in the imported collection
    // but the existing collection had a field "keyword" so the values would not get imported
    // here, check to see if the current collection has a field with the same bibtex name and
    // use it instead of creating a new one
    BibtexCollection* existingColl = dynamic_cast<BibtexCollection*>(existingColl_.data());
    FieldPtr existingField;
    if(existingColl && existingColl->type() == Collection::Bibtex) {
      existingField = existingColl->fieldByBibtexName(bibtexField_);
    }
    if(existingField) {
      field = new Field(*existingField);
    } else if(value_.length() < 100) {
      // arbitrarily say if the value has more than 100 chars, then it's a paragraph
      QString vlower = value_.toLower();
      // special case, try to detect URLs
      if(bibtexField_ == QLatin1String("url")
         || vlower.startsWith(QLatin1String("http")) // may also be https
         || vlower.startsWith(QLatin1String("ftp:/"))
         || vlower.startsWith(QLatin1String("file:/"))
         || vlower.startsWith(QLatin1String("/"))) { // assume this indicates a local path
        myDebug() << "creating a URL field for" << bibtexField_;
        field = new Field(bibtexField_, KStringHandler::capwords(bibtexField_), Field::URL);
      } else {
        myDebug() << "creating a LINE field for" << bibtexField_;
        field = new Field(bibtexField_, KStringHandler::capwords(bibtexField_), Field::Line);
      }
      field->setCategory(i18n("Unknown"));
    } else {
      myDebug() << "creating a PARA field for" << bibtexField_;
      field = new Field(bibtexField_, KStringHandler::capwords(bibtexField_), Field::Para);
    }
    field->setProperty(QLatin1String("bibtex"), bibtexField_);
    c->addField(field);
  }
  // special case keywords, replace commas with semi-colons so they get separated
  QString value = value_;
  Q_ASSERT(field);
  if(bibtexField_.startsWith(QLatin1String("keyword"))) {
    value.replace(QRegExp(QLatin1String("\\s*,\\s*")), FieldFormat::delimiterString());
    // special case refbase bibtex export, with multiple keywords fields
    QString oValue = entry_->field(field);
    if(!oValue.isEmpty()) {
      value = oValue + FieldFormat::delimiterString() + value;
    }
  // special case for tilde, since it's a non-breaking space in LateX
  // replace it EXCEPT for URL or DOI fields
  } else if(bibtexField_ != QLatin1String("doi") && field->type() != Field::URL) {
    value.replace(QLatin1Char('~'), QChar(0xA0));
  } else if(field->type() == Field::URL || bibtexField_ == QLatin1String("url")) {
    // special case for url package
    if(value.startsWith(QLatin1String("\\url{")) && value.endsWith(QLatin1Char('}'))) {
      value.remove(0, 5).chop(1);
    }
  }
  return entry_->setField(field, value);
}

Tellico::Data::EntryList BibtexCollection::duplicateBibtexKeys() const {
  QSet<EntryPtr> dupes;
  QHash<QString, EntryPtr> keyHash;
  
  const QString keyField = QLatin1String("bibtex-key");
  QString keyValue;
  foreach(EntryPtr entry, entries()) {
    keyValue = entry->field(keyField);
    if(keyHash.contains(keyValue)) {
      dupes << keyHash.value(keyValue) << entry;
     } else {
       keyHash.insert(keyValue, entry);
     }
  }
  return dupes.toList();
}

#include "bibtexcollection.moc"
