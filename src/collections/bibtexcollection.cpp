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
#include "../utils/bibtexhandler.h"
#include "../fieldformat.h"
#include "../core/tellico_strings.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KStringHandler>

using namespace Tellico;
using Tellico::Data::BibtexCollection;

BibtexCollection::BibtexCollection(bool addDefaultFields_, const QString& title_)
   : Collection(title_.isEmpty() ? i18n("Bibliography") : title_) {
  setDefaultGroupField(QStringLiteral("author"));
  if(addDefaultFields_) {
    addFields(defaultFields());
  }

  // Bibtex has some default macros for the months
  addMacro(QStringLiteral("jan"), QString());
  addMacro(QStringLiteral("feb"), QString());
  addMacro(QStringLiteral("mar"), QString());
  addMacro(QStringLiteral("apr"), QString());
  addMacro(QStringLiteral("may"), QString());
  addMacro(QStringLiteral("jun"), QString());
  addMacro(QStringLiteral("jul"), QString());
  addMacro(QStringLiteral("aug"), QString());
  addMacro(QStringLiteral("sep"), QString());
  addMacro(QStringLiteral("oct"), QString());
  addMacro(QStringLiteral("nov"), QString());
  addMacro(QStringLiteral("dec"), QString());
}

Tellico::Data::FieldList BibtexCollection::defaultFields() {
  FieldList list;
  FieldPtr field;

  const QString bibtex = QStringLiteral("bibtex");

/******************* General ****************************/

  field = Field::createDefaultField(Field::TitleField);
  field->setProperty(bibtex, QStringLiteral("title"));
  list.append(field);

  QStringList types;
  types << QStringLiteral("article") << QStringLiteral("book")
        << QStringLiteral("booklet") << QStringLiteral("inbook")
        << QStringLiteral("incollection") << QStringLiteral("inproceedings")
        << QStringLiteral("manual") << QStringLiteral("mastersthesis")
        << QStringLiteral("misc") << QStringLiteral("phdthesis")
        << QStringLiteral("proceedings") << QStringLiteral("techreport")
        << QStringLiteral("unpublished") << QStringLiteral("periodical")
        << QStringLiteral("conference");
  field = new Field(QStringLiteral("entry-type"), i18n("Entry Type"), types);
  field->setProperty(bibtex, QStringLiteral("entry-type"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowGrouped | Field::NoDelete);
  field->setDescription(i18n("These entry types are specific to bibtex. See the bibtex documentation."));
  list.append(field);

  field = new Field(QStringLiteral("author"), i18n("Author"));
  field->setProperty(bibtex, QStringLiteral("author"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("bibtex-key"), i18n("Bibtex Key"));
  field->setProperty(bibtex, QStringLiteral("key"));
  field->setCategory(i18n("General"));
  field->setFlags(Field::NoDelete);
  list.append(field);

  field = new Field(QStringLiteral("booktitle"), i18n("Book Title"));
  field->setProperty(bibtex, QStringLiteral("booktitle"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QStringLiteral("editor"), i18n("Editor"));
  field->setProperty(bibtex, QStringLiteral("editor"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatName);
  list.append(field);

  field = new Field(QStringLiteral("organization"), i18n("Organization"));
  field->setProperty(bibtex, QStringLiteral("organization"));
  field->setCategory(TC_I18N1(categoryGeneral));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

//  field = new Field(QLatin1String("institution"), i18n("Institution"));
//  field->setProperty(QLatin1String("bibtex"), QLatin1String("institution"));
//  field->setCategory(TC_I18N1(categoryGeneral));
//  field->setFlags(Field::AllowDelete);
//  field->setFormatType(FieldFormat::FormatTitle);
//  list.append(field);

/******************* Publishing ****************************/
  field = new Field(QStringLiteral("publisher"), i18n("Publisher"));
  field->setProperty(bibtex, QStringLiteral("publisher"));
  field->setCategory(TC_I18N1(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("address"), i18n("Address"));
  field->setProperty(bibtex, QStringLiteral("address"));
  field->setCategory(TC_I18N1(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("edition"), i18n("Edition"));
  field->setProperty(bibtex, QStringLiteral("edition"));
  field->setCategory(TC_I18N1(categoryPublishing));
  field->setFlags(Field::AllowCompletion);
  list.append(field);

  // don't make it a number, it could have latex processing commands in it
  field = new Field(QStringLiteral("pages"), i18n("Pages"));
  field->setProperty(bibtex, QStringLiteral("pages"));
  field->setCategory(TC_I18N1(categoryPublishing));
  list.append(field);

  field = new Field(QStringLiteral("year"), i18n("Year"), Field::Number);
  field->setProperty(bibtex, QStringLiteral("year"));
  field->setCategory(TC_I18N1(categoryPublishing));
  field->setFlags(Field::AllowGrouped);
  list.append(field);

  field = Field::createDefaultField(Field::IsbnField);
  field->setProperty(bibtex, QStringLiteral("isbn"));
  field->setCategory(TC_I18N1(categoryPublishing));
  list.append(field);

  field = new Field(QStringLiteral("journal"), i18n("Journal"));
  field->setProperty(bibtex, QStringLiteral("journal"));
  field->setCategory(TC_I18N1(categoryPublishing));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatPlain);
  list.append(field);

  field = new Field(QStringLiteral("doi"), i18n("DOI"));
  field->setProperty(bibtex, QStringLiteral("doi"));
  field->setCategory(TC_I18N1(categoryPublishing));
  field->setDescription(i18n("Digital Object Identifier"));
  list.append(field);

  // could make this a string list, but since bibtex import could have funky values
  // keep it an editbox
  field = new Field(QStringLiteral("month"), i18n("Month"));
  field->setProperty(bibtex, QStringLiteral("month"));
  field->setCategory(TC_I18N1(categoryPublishing));
  field->setFlags(Field::AllowCompletion);
  list.append(field);

  field = new Field(QStringLiteral("number"), i18n("Number"), Field::Number);
  field->setProperty(bibtex, QStringLiteral("number"));
  field->setCategory(TC_I18N1(categoryPublishing));
  list.append(field);

  field = new Field(QStringLiteral("howpublished"), i18n("How Published"));
  field->setProperty(bibtex, QStringLiteral("howpublished"));
  field->setCategory(TC_I18N1(categoryPublishing));
  list.append(field);

//  field = new Field(QLatin1String("school"), i18n("School"));
//  field->setProperty(QLatin1String("bibtex"), QLatin1String("school"));
//  field->setCategory(TC_I18N1(categoryPublishing));
//  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
//  list.append(field);

/******************* Classification ****************************/
  field = new Field(QStringLiteral("chapter"), i18n("Chapter"), Field::Number);
  field->setProperty(bibtex, QStringLiteral("chapter"));
  field->setCategory(TC_I18N1(categoryMisc));
  list.append(field);

  field = new Field(QStringLiteral("series"), i18n("Series"));
  field->setProperty(bibtex, QStringLiteral("series"));
  field->setCategory(TC_I18N1(categoryMisc));
  field->setFlags(Field::AllowCompletion | Field::AllowGrouped);
  field->setFormatType(FieldFormat::FormatTitle);
  list.append(field);

  field = new Field(QStringLiteral("volume"), i18nc("A number field in a bibliography", "Volume"), Field::Number);
  field->setProperty(bibtex, QStringLiteral("volume"));
  field->setCategory(TC_I18N1(categoryMisc));
  list.append(field);

  field = new Field(QStringLiteral("crossref"), i18n("Cross-Reference"));
  field->setProperty(bibtex, QStringLiteral("crossref"));
  field->setCategory(TC_I18N1(categoryMisc));
  list.append(field);

//  field = new Field(QLatin1String("annote"), i18n("Annotation"));
//  field->setProperty(QLatin1String("bibtex"), QLatin1String("annote"));
//  field->setCategory(TC_I18N1(categoryMisc));
//  list.append(field);

  field = new Field(QStringLiteral("keyword"), i18n("Keywords"));
  field->setProperty(bibtex, QStringLiteral("keywords"));
  field->setCategory(TC_I18N1(categoryMisc));
  field->setFlags(Field::AllowCompletion | Field::AllowMultiple | Field::AllowGrouped);
  list.append(field);

  field = new Field(QStringLiteral("url"), i18n("URL"), Field::URL);
  field->setProperty(bibtex, QStringLiteral("url"));
  field->setCategory(TC_I18N1(categoryMisc));
  list.append(field);

  field = new Field(QStringLiteral("abstract"), i18n("Abstract"), Field::Para);
  field->setProperty(bibtex, QStringLiteral("abstract"));
  list.append(field);

  field = new Field(QStringLiteral("note"), i18n("Notes"), Field::Para);
  field->setProperty(bibtex, QStringLiteral("note"));
  list.append(field);

  field = Field::createDefaultField(Field::IDField);
  field->setCategory(TC_I18N1(categoryMisc));
  list.append(field);

  field = Field::createDefaultField(Field::CreatedDateField);
  field->setCategory(TC_I18N1(categoryMisc));
  list.append(field);

  field = Field::createDefaultField(Field::ModifiedDateField);
  field->setCategory(TC_I18N1(categoryMisc));
  list.append(field);

  return list;
}

bool BibtexCollection::addField(Tellico::Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
  bool success = Collection::addField(field_);
  if(success) {
    const QString bibtex = field_->property(QStringLiteral("bibtex"));
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
  const QString bibtex = QStringLiteral("bibtex");
  bool success = Collection::modifyField(newField_);
  FieldPtr oldField = fieldByName(newField_->name());
  QString oldBibtex = oldField->property(bibtex);
  // if the field was edited in place, can't just look at the property value
  if(oldField == newField_) {
    // have to look at all fields in the hash to update the key
    auto i = m_bibtexFieldDict.constBegin();
    for( ; i != m_bibtexFieldDict.constEnd(); ++i) {
      if(oldField == i.value()) {
        oldBibtex = i.key();
      }
    }
  }
  success &= (m_bibtexFieldDict.remove(oldBibtex) > 0);

  const QString newBibtex = newField_->property(bibtex);
  if(!newBibtex.isEmpty()) {
    oldField->setProperty(bibtex, newBibtex);
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
  const QString bibtex = field_->property(QStringLiteral("bibtex"));
  if(!bibtex.isEmpty()) {
    success &= (m_bibtexFieldDict.remove(bibtex) != 0);
  }
  return success && Collection::removeField(field_, force_);
}

bool BibtexCollection::removeField(const QString& name_, bool force_) {
  return removeField(fieldByName(name_), force_);
}

Tellico::Data::FieldPtr BibtexCollection::fieldByBibtexName(const QString& bibtex_) const {
  return FieldPtr(m_bibtexFieldDict.contains(bibtex_) ? m_bibtexFieldDict.value(bibtex_) : nullptr);
}

Tellico::Data::EntryPtr BibtexCollection::entryByBibtexKey(const QString& key_) const {
  EntryPtr entry;
  // we do assume unique keys
  foreach(EntryPtr e, entries()) {
    if(e->field(QStringLiteral("bibtex-key")) == key_) {
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

int BibtexCollection::sameEntry(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_) const {
  // equal identifiers are easy, give it a weight of 100
  if(EntryComparison::score(entry1_, entry2_, QStringLiteral("isbn"),  this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QStringLiteral("lccn"),  this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QStringLiteral("doi"),   this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QStringLiteral("pmid"),  this) > 0 ||
     EntryComparison::score(entry1_, entry2_, QStringLiteral("arxiv"), this) > 0) {
    return EntryComparison::ENTRY_PERFECT_MATCH;
  }
  int res = 3*EntryComparison::score(entry1_, entry2_, QStringLiteral("title"), this);
  res += EntryComparison::score(entry1_, entry2_, QStringLiteral("author"),   this);
  res += EntryComparison::score(entry1_, entry2_, QStringLiteral("entry-type"),   this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::score(entry1_, entry2_, QStringLiteral("year"),  this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::score(entry1_, entry2_, QStringLiteral("publisher"), this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;

  res += EntryComparison::score(entry1_, entry2_, QStringLiteral("binding"),  this);
  if(res >= EntryComparison::ENTRY_PERFECT_MATCH) return res;
  return res;
}

// static
Tellico::Data::CollPtr BibtexCollection::convertBookCollection(Tellico::Data::CollPtr coll_) {
  const QString bibtex = QStringLiteral("bibtex");
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
      field->setProperty(bibtex, QStringLiteral("number"));
    } else if(name == QLatin1String("pur_price")) {
      field->setProperty(bibtex, QStringLiteral("price"));
    } else if(name == QLatin1String("cr_year")) {
      field->setProperty(bibtex, QStringLiteral("year"));
    } else if(name == QLatin1String("bibtex-id")) {
      field->setProperty(bibtex, QStringLiteral("key"));
    } else if(name == QLatin1String("keyword")) {
      field->setProperty(bibtex, QStringLiteral("keywords"));
    } else if(name == QLatin1String("comments")) {
      field->setProperty(bibtex, QStringLiteral("note"));
    }
    coll->addField(field);
  }

  // also need to add required fields, those with NoDelete set
  foreach(FieldPtr defaultField, coll->defaultFields()) {
    if(!coll->hasField(defaultField->name()) && defaultField->hasFlag(Field::NoDelete)) {
      // but don't add a Bibtex Key if there's already a bibtex-id
      if(defaultField->property(bibtex) != QLatin1String("key")
         || !coll->hasField(QStringLiteral("bibtex-id"))) {
        coll->addField(defaultField);
      }
    }
  }

  // set the entry-type to book
  FieldPtr field = coll->fieldByBibtexName(QStringLiteral("entry-type"));
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
      newEntry->setField(entryTypeName, QStringLiteral("book"));
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
    field = c->fieldByBibtexName(QStringLiteral("keywords"));
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
    field->setProperty(QStringLiteral("bibtex"), bibtexField_);
    c->addField(field);
  }
  // special case keywords, replace commas with semi-colons so they get separated
  QString value = value_;
  Q_ASSERT(field);
  static const QRegularExpression spaceCommaRx(QLatin1String("\\s*,\\s*"));
  if(bibtexField_.startsWith(QLatin1String("keyword"))) {
    value.replace(spaceCommaRx, FieldFormat::delimiterString());
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

  const QString keyField = QStringLiteral("bibtex-key");
  QString keyValue;
  foreach(EntryPtr entry, entries()) {
    keyValue = entry->field(keyField);
    if(keyHash.contains(keyValue)) {
      dupes << keyHash.value(keyValue) << entry;
    } else {
      keyHash.insert(keyValue, entry);
    }
  }
  return dupes.values();
}
