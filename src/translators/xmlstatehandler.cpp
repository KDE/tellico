/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "xmlstatehandler.h"
#include "tellico_xml.h"
#include "../collection.h"
#include "../collectionfactory.h"
#include "../collections/bibtexcollection.h"
#include "../fieldformat.h"
#include "../images/image.h"
#include "../images/imageinfo.h"
#include "../images/imagefactory.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kcodecs.h>

namespace {

inline
QString attValue(const QXmlAttributes& atts, const char* name, const QString& defaultValue=QString()) {
  int idx = atts.index(QLatin1String(name));
  return idx < 0 ? defaultValue : atts.value(idx);
}

inline
QString attValue(const QXmlAttributes& atts, const char* name, const char* defaultValue) {
  Q_ASSERT(defaultValue);
  return attValue(atts, name, QLatin1String(defaultValue));
}

inline
QString realFieldName(int syntaxVersion, const QString& localName) {
  QString fieldName = localName;
  if(syntaxVersion < 2 && fieldName == QLatin1String("keywords")) {
    // in version 2, "keywords" changed to "keyword"
    fieldName = QLatin1String("keyword");
  }
  return fieldName;
}

}

using Tellico::Import::SAX::StateHandler;
using Tellico::Import::SAX::NullHandler;
using Tellico::Import::SAX::RootHandler;
using Tellico::Import::SAX::DocumentHandler;
using Tellico::Import::SAX::CollectionHandler;
using Tellico::Import::SAX::FieldsHandler;
using Tellico::Import::SAX::FieldHandler;
using Tellico::Import::SAX::FieldPropertyHandler;
using Tellico::Import::SAX::BibtexPreambleHandler;
using Tellico::Import::SAX::BibtexMacrosHandler;
using Tellico::Import::SAX::BibtexMacroHandler;
using Tellico::Import::SAX::EntryHandler;
using Tellico::Import::SAX::FieldValueContainerHandler;
using Tellico::Import::SAX::FieldValueHandler;
using Tellico::Import::SAX::DateValueHandler;
using Tellico::Import::SAX::TableColumnHandler;
using Tellico::Import::SAX::ImagesHandler;
using Tellico::Import::SAX::ImageHandler;
using Tellico::Import::SAX::FiltersHandler;
using Tellico::Import::SAX::FilterHandler;
using Tellico::Import::SAX::FilterRuleHandler;
using Tellico::Import::SAX::BorrowersHandler;
using Tellico::Import::SAX::BorrowerHandler;
using Tellico::Import::SAX::LoanHandler;

StateHandler* StateHandler::nextHandler(const QString& ns_, const QString& localName_, const QString& qName_) {
  StateHandler* handler = nextHandlerImpl(ns_, localName_, qName_);
  if(!handler) {
    myWarning() << "no handler for" << localName_;
  }
  return handler ? handler : new NullHandler(d);
}

StateHandler* RootHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("tellico") || localName_ == QLatin1String("bookcase")) {
    return new DocumentHandler(d);
  }
  return new RootHandler(d);
}

StateHandler* DocumentHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("collection")) {
    return new CollectionHandler(d);
  } else if(localName_ == QLatin1String("filters")) {
    return new FiltersHandler(d);
  } else if(localName_ == QLatin1String("borrowers")) {
    return new BorrowersHandler(d);
  }
  return 0;
}

bool DocumentHandler::start(const QString&, const QString& localName_, const QString&, const QXmlAttributes& atts_) {
  // the syntax version field name changed from "version" to "syntaxVersion" in version 3
  int idx = atts_.index(QLatin1String("syntaxVersion"));
  if(idx < 0) {
    idx = atts_.index(QLatin1String("version"));
  }
  if(idx < 0) {
    myWarning() << "no syntax version";
    return false;
  }
  d->syntaxVersion = atts_.value(idx).toUInt();
  if(d->syntaxVersion > Tellico::XML::syntaxVersion) {
    d->error = i18n("It is from a future version of Tellico.");
    return false;
  }
  if((d->syntaxVersion > 6 && localName_ != QLatin1String("tellico")) ||
     (d->syntaxVersion < 7 && localName_ != QLatin1String("bookcase"))) {
    // no error message
    myWarning() << "bad root element name";
    return false;
  }
  d->ns = d->syntaxVersion > 6 ? Tellico::XML::nsTellico : Tellico::XML::nsBookcase;
  return true;
}

bool DocumentHandler::end(const QString&, const QString&, const QString&) {
  return true;
}

StateHandler* CollectionHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if((d->syntaxVersion > 3 && localName_ == QLatin1String("fields")) ||
     (d->syntaxVersion < 4 && localName_ == QLatin1String("attributes"))) {
    return new FieldsHandler(d);
  } else if(localName_ == QLatin1String("bibtex-preamble")) {
    return new BibtexPreambleHandler(d);
  } else if(localName_ == QLatin1String("macros")) {
    return new BibtexMacrosHandler(d);
  } else if(localName_ == d->entryName) {
    return new EntryHandler(d);
  } else if(localName_ == QLatin1String("images")) {
    return new ImagesHandler(d);
  }
  return 0;
}

bool CollectionHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  d->collTitle = attValue(atts_, "title");
  d->collType = attValue(atts_, "type").toInt();
  d->entryName = attValue(atts_, "unit");
  return true;
}

bool CollectionHandler::end(const QString&, const QString&, const QString&) {
  d->coll->addEntries(d->entries);

  // a little hidden capability was to just have a local path as an image file name
  // and on reading the xml file, Tellico would load the image file, too
  // here, we need to scan all the image values in all the entries and check
  // maybe this is too costly, especially since the capability wasn't advertised?
  const bool hasMDate = d->coll->hasField(QLatin1String("mdate"));

  const int maxImageWarnings = 3;
  int imageWarnings = 0;

  Data::FieldList fields = d->coll->imageFields();
  foreach(Data::EntryPtr entry, d->entries) {
    foreach(Data::FieldPtr field, fields) {
      QString value = entry->field(field, false);
      if(value.isEmpty()) {
        continue;
      }
      // image info should have already been loaded
      // if not, then there was no <image> in the XML
      // so it's a url, but maybe link only
      if(!ImageFactory::hasImageInfo(value)) {
        KUrl u(value);
        // the image file name is a valid URL, but I want it to be a local URL or non empty remote one
        if(u.isValid() && (u.isLocalFile() || !u.host().isEmpty())) {
          QString result = ImageFactory::addImage(u, imageWarnings >= maxImageWarnings /* quiet */);
          if(result.isEmpty()) {
            // clear value for the field in this case
            value.clear();
            ++imageWarnings;
          } else {
            value = result;
          }
        } else {
          value = Data::Image::idClean(value);
        }
        if(hasMDate) {
          // since the modified date gets reset, keep a copy
          const QString mdate = entry->field(QLatin1String("mdate"));
          entry->setField(field->name(), value);
          entry->setField(QLatin1String("mdate"), mdate);
        } else {
          entry->setField(field->name(), value);
        }
      }
    }
  }
  return true;
}

StateHandler* FieldsHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if((d->syntaxVersion > 3 && localName_ == QLatin1String("field")) ||
     (d->syntaxVersion < 4 && localName_ == QLatin1String("attribute"))) {
    return new FieldHandler(d);
  }
  return 0;
}

bool FieldsHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  d->defaultFields = false;
  return true;
}

bool FieldsHandler::end(const QString&, const QString&, const QString&) {
  // add default fields if there was a default field name, or no names at all
  const bool addFields = d->defaultFields || d->fields.isEmpty();
  // in syntax 4, the element name was changed to "entry", always, rather than depending on
  // on the entryName of the collection.
  if(d->syntaxVersion > 3) {
    d->entryName = QLatin1String("entry");
    Data::Collection::Type type = static_cast<Data::Collection::Type>(d->collType);
    d->coll = CollectionFactory::collection(type, addFields);
  } else {
    d->coll = CollectionFactory::collection(d->entryName, addFields);
  }

  if(!d->collTitle.isEmpty()) {
    d->coll->setTitle(d->collTitle);
  }

  // add a default field for ID
  // checking the defaultFields bool since if it is true, we already added these default fields
  // even for old syntax versions
  if(d->syntaxVersion < 11 && !d->defaultFields) {
    d->coll->addField(Data::Collection::createDefaultField(Data::Collection::IDField));
  }
  // now add all the new fields
  d->coll->addFields(d->fields);
  if(d->syntaxVersion < 11 && !d->defaultFields) {
    d->coll->addField(Data::Collection::createDefaultField(Data::Collection::CreatedDateField));
    d->coll->addField(Data::Collection::createDefaultField(Data::Collection::ModifiedDateField));
  }

//  as a special case, for old book collections with a bibtex-id field, convert to Bibtex
  if(d->syntaxVersion < 4 && d->collType == Data::Collection::Book
     && d->coll->hasField(QLatin1String("bibtex-id"))) {
    d->coll = Data::BibtexCollection::convertBookCollection(d->coll);
  }

  return true;
}

StateHandler* FieldHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("prop")) {
    return new FieldPropertyHandler(d);
  }
  return 0;
}

bool FieldHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  // special case: if the i18n attribute equals true, then translate the title, description, and category
  const bool isI18n = attValue(atts_, "i18n") == QLatin1String("true");

  QString name = attValue(atts_, "name", "unknown");
  if(name == QLatin1String("_default")) {
    d->defaultFields = true;
    return true;
  }

  QString title  = attValue(atts_, "title", i18n("Unknown"));
  if(isI18n) {
    title = i18n(title.toUtf8());
  }

  QString typeStr = attValue(atts_, "type", QString::number(Data::Field::Line));
  Data::Field::Type type = static_cast<Data::Field::Type>(typeStr.toInt());

  Data::FieldPtr field;
  if(type == Data::Field::Choice) {
    QStringList allowed = attValue(atts_, "allowed").split(QRegExp(QLatin1String("\\s*;\\s*")));
    if(isI18n) {
      for(QStringList::Iterator word = allowed.begin(); word != allowed.end(); ++word) {
        (*word) = i18n((*word).toUtf8());
      }
    }
    field = new Data::Field(name, title, allowed);
  } else {
    field = new Data::Field(name, title, type);
  }

  int idx = atts_.index(QLatin1String("category"));
  if(idx > -1) {
    // at one point, the categories had keyboard accels
    QString cat = atts_.value(idx);
    if(d->syntaxVersion < 9) {
      cat.remove(QLatin1Char('&'));
    }
    if(isI18n) {
      cat = i18n(cat.toUtf8());
    }
    field->setCategory(cat);
  }

  idx = atts_.index(QLatin1String("flags"));
  if(idx > -1) {
    int flags = atts_.value(idx).toInt();
    // I also changed the enum values for syntax 3, but the only custom field
    // would have been bibtex-id
    if(d->syntaxVersion < 3 && name == QLatin1String("bibtex-id")) {
      flags = 0;
    }

    // in syntax version 4, added a flag to disallow deleting attributes
    // if it's a version before that and is the title, then add the flag
    if(d->syntaxVersion < 4 && name == QLatin1String("title")) {
      flags |= Data::Field::NoDelete;
    }
    // some of the flags may have been set in the constructor
    // in the case of old Dependent fields changing, for example
    // so combine with the existing flags
    field->setFlags(field->flags() | flags);
  }

  QString formatStr = attValue(atts_, "format", QString::number(Data::Field::FormatNone));
  Data::Field::FormatFlag format = static_cast<Data::Field::FormatFlag>(formatStr.toInt());
  field->setFormatFlag(format);

  idx = atts_.index(QLatin1String("description"));
  if(idx > -1) {
    QString desc = atts_.value(idx);
    if(isI18n) {
      desc = i18n(desc.toUtf8());
    }
    field->setDescription(desc);
  }

  if(d->syntaxVersion < 5 && atts_.index(QLatin1String("bibtex-field")) > -1) {
    field->setProperty(QLatin1String("bibtex"), attValue(atts_, "bibtex-field"));
  }

  // for syntax 8, rating fields got their own type
  if(d->syntaxVersion < 8) {
    Data::Field::convertOldRating(field); // does all its own checking
  }
  d->fields.append(field);

  return true;
}

bool FieldHandler::end(const QString&, const QString&, const QString&) {
  // the value template for derived values used to be the field description
  // now it is the 'template' property
  // for derived value fields, if there is no property and the description has a '%'
  // move it to the property
  //
  // might be empty is we're only adding default fields
  if(!d->fields.isEmpty()) {
    Data::FieldPtr field = d->fields.back();
    if(field->hasFlag(Data::Field::Derived) &&
       field->property(QLatin1String("template")).isEmpty() &&
       field->description().contains(QLatin1Char('%'))) {
      field->setProperty(QLatin1String("template"), field->description());
      field->setDescription(QString());
    }
  }

  return true;
}

bool FieldPropertyHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  // there should be at least one field already so we can add properties to it
  Q_ASSERT(!d->fields.isEmpty());
  Data::FieldPtr field = d->fields.back();

  m_propertyName = attValue(atts_, "name");

  // all track fields in music collections prior to version 9 get converted to three columns
  if(d->syntaxVersion < 9) {
    if(d->collType == Data::Collection::Album && field->name() == QLatin1String("track")) {
      field->setProperty(QLatin1String("columns"), QLatin1String("3"));
      field->setProperty(QLatin1String("column1"), i18n("Title"));
      field->setProperty(QLatin1String("column2"), i18n("Artist"));
      field->setProperty(QLatin1String("column3"), i18n("Length"));
    } else if(d->collType == Data::Collection::Video && field->name() == QLatin1String("cast")) {
      field->setProperty(QLatin1String("column1"), i18n("Actor/Actress"));
      field->setProperty(QLatin1String("column2"), i18n("Role"));
    }
  }

  return true;
}

bool FieldPropertyHandler::end(const QString&, const QString&, const QString&) {
  Q_ASSERT(!m_propertyName.isEmpty());
  // add the previous property
  Data::FieldPtr field = d->fields.back();
  field->setProperty(m_propertyName, d->text);
  return true;
}

bool BibtexPreambleHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  return true;
}

bool BibtexPreambleHandler::end(const QString&, const QString&, const QString&) {
  Q_ASSERT(d->coll);
  if(d->coll && d->collType == Data::Collection::Bibtex && !d->text.isEmpty()) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(d->coll.data());
    c->setPreamble(d->text);
  }
  return true;
}

StateHandler* BibtexMacrosHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("macro")) {
    return new BibtexMacroHandler(d);
  }
  return 0;
}

bool BibtexMacrosHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  return true;
}

bool BibtexMacrosHandler::end(const QString&, const QString&, const QString&) {
  return true;
}

bool BibtexMacroHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  m_macroName = attValue(atts_, "name");
  return true;
}

bool BibtexMacroHandler::end(const QString&, const QString&, const QString&) {
  if(d->coll && d->collType == Data::Collection::Bibtex && !m_macroName.isEmpty() && !d->text.isEmpty()) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(d->coll.data());
    c->addMacro(m_macroName, d->text);
  }
  return true;
}

StateHandler* EntryHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(d->coll->hasField(realFieldName(d->syntaxVersion, localName_))) {
    return new FieldValueHandler(d);
  }
  return new FieldValueContainerHandler(d);
}

bool EntryHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  // the entries must come after the fields
  if(!d->coll || d->coll->fields().isEmpty()) {
    // special case for very old versions which did not have user-editable fields
    if(d->syntaxVersion < 3) {
      d->defaultFields = true;
      FieldsHandler handler(d);
      // fake the end of a fields element, which will add the default fields
      handler.end(QString(), QString(), QString());
    } else {
      myWarning() << "entries must come after fields are defined";
      d->error = i18n("File format error: entries must come after fields are defined.");
      return false;
    }
  }
  int id = attValue(atts_, "id").toInt();
  Data::EntryPtr entry;
  if(id > 0) {
    entry = new Data::Entry(d->coll, id);
  } else {
    entry = new Data::Entry(d->coll);
  }
  d->entries.append(entry);
  return true;
}

bool EntryHandler::end(const QString&, const QString&, const QString&) {
  Data::EntryPtr entry = d->entries.back();
  Q_ASSERT(entry);
  if(!d->modifiedDate.isEmpty() && d->coll->hasField(QLatin1String("mdate"))) {
    entry->setField(QLatin1String("mdate"), d->modifiedDate);
    d->modifiedDate.clear();
  }
  return true;
}

StateHandler* FieldValueContainerHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(d->coll->hasField(realFieldName(d->syntaxVersion, localName_))) {
    return new FieldValueHandler(d);
  }
  return new FieldValueContainerHandler(d);
}

bool FieldValueContainerHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  return true;
}

bool FieldValueContainerHandler::end(const QString&, const QString&, const QString&) {
  return true;
}

StateHandler* FieldValueHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("year") ||
     localName_ == QLatin1String("month") ||
     localName_ == QLatin1String("day")) {
    return new DateValueHandler(d);
  } else if(localName_ == QLatin1String("column")) {
    return new TableColumnHandler(d);
  }
  return 0;
}

bool FieldValueHandler::start(const QString&, const QString&, const QString& localName_, const QXmlAttributes& atts_) {
  d->currentField = d->coll->fieldByName(realFieldName(d->syntaxVersion, localName_));
  m_i18n = attValue(atts_, "i18n") == QLatin1String("true");
  m_validateISBN = (localName_ == QLatin1String("isbn")) &&
                   (attValue(atts_, "validate") != QLatin1String("no"));
  return true;
}

bool FieldValueHandler::end(const QString&, const QString& localName_, const QString&) {
  Data::EntryPtr entry = d->entries.back();
  Q_ASSERT(entry);
  QString fieldName = realFieldName(d->syntaxVersion, localName_);
  QString fieldValue = d->text;

  Data::FieldPtr f = d->coll->fieldByName(fieldName);
  if(!f) {
    myWarning() << "no field named " << fieldName;
    return true;
  }
  // if it's a derived value, no field value is added
  if(f->hasFlag(Data::Field::Derived)) {
    return true;
  }

  if(d->syntaxVersion < 4 && f->type() == Data::Field::Bool) {
    // in version 3 and prior, checkbox attributes had no text(), set it to "true"
    fieldValue = QLatin1String("true");
  } else if(d->syntaxVersion < 8 && f->type() == Data::Field::Rating) {
    // in version 8, old rating fields get changed
    bool ok;
    uint i = Tellico::toUInt(fieldValue, &ok);
    if(ok) {
      fieldValue = QString::number(i);
    }
  } else if(!d->textBuffer.isEmpty()) {
    // for dates and tables, the value is built up from child elements
#ifndef NDEBUG
    if(!d->text.isEmpty()) {
      myWarning() << "ignoring value for field" << localName_ << ":" << d->text;
    }
#endif
    fieldValue = d->textBuffer;
    d->textBuffer.clear();
  }
  // this is not an else branch, the data may be in the textBuffer
  if(d->syntaxVersion < 9 && d->coll->type() == Data::Collection::Album && fieldName == QLatin1String("track")) {
    // yes, this assumes the artist has already been set
    fieldValue += FieldFormat::columnDelimiterString();
    fieldValue += entry->field(QLatin1String("artist"));
  }
  // special case: if the i18n attribute equals true, then translate the title, description, and category
  if(m_i18n) {
    fieldValue = i18n(fieldValue.toUtf8());
  }
  // special case for isbn fields, go ahead and validate
  if(m_validateISBN) {
    ISBNValidator val(0);
    val.fixup(fieldValue);
  }
  if(fieldValue.isEmpty()) {
    return true;
  }
  // for fields with multiple values, we need to add on the new value
  QString oldValue = entry->field(fieldName);
  if(!oldValue.isEmpty()) {
    if(f->type() == Data::Field::Table) {
      fieldValue = oldValue + FieldFormat::rowDelimiterString() + fieldValue;
    } else if(f->hasFlag(Data::Field::AllowMultiple)) {
      fieldValue = oldValue + FieldFormat::delimiterString() + fieldValue;
    }
  }
  // since the modified date value in the entry gets changed everytime we set a new value
  // we have to save it and set it after changing all the others
  if(fieldName == QLatin1String("mdate")) {
    d->modifiedDate = fieldValue;
  } else {
    entry->setField(fieldName, fieldValue);
  }
  return true;
}

bool DateValueHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  return true;
}

bool DateValueHandler::end(const QString&, const QString& localName_, const QString&) {
  // the data value is y-m-d even if there are no date values
  if(d->textBuffer.isEmpty()) {
    d->textBuffer = QLatin1String("--");
  }
  QStringList tokens = d->textBuffer.split(QLatin1Char('-'), QString::KeepEmptyParts);
  Q_ASSERT(tokens.size() == 3);
  if(localName_ == QLatin1String("year")) {
    tokens[0] = d->text;
  } else if(localName_ == QLatin1String("month")) {
    tokens[1] = d->text;
  } else if(localName_ == QLatin1String("day")) {
    tokens[2] = d->text;
  }
  d->textBuffer = tokens.join(QLatin1String("-"));
  return true;
}

bool TableColumnHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  return true;
}

bool TableColumnHandler::end(const QString&, const QString&, const QString&) {
  // for old collections, if the second column holds the track length, bump it to next column
  if(d->syntaxVersion < 9 &&
     d->coll->type() == Data::Collection::Album &&
     d->currentField->name() == QLatin1String("track") &&
     !d->textBuffer.isEmpty() &&
     d->textBuffer.contains(FieldFormat::columnDelimiterString()) == 0) {
    QRegExp rx(QLatin1String("\\d+:\\d\\d"));
    if(rx.exactMatch(d->text)) {
      d->text += FieldFormat::columnDelimiterString();
      d->text += d->entries.back()->field(QLatin1String("artist"));
    }
  }

  if(!d->textBuffer.isEmpty()) {
    d->textBuffer += FieldFormat::columnDelimiterString();
  }
  d->textBuffer += d->text;
  return true;
}

StateHandler* ImagesHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("image")) {
    return new ImageHandler(d);
  }
  return 0;
}

bool ImagesHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  // reset variable that gets updated in the image handler
  d->hasImages = false;
  return true;
}

bool ImagesHandler::end(const QString&, const QString&, const QString&) {
  return true;
}

bool ImageHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  m_format = attValue(atts_, "format");
  m_link = attValue(atts_, "link") == QLatin1String("true");
  // idClean() already calls shareString()
  m_imageId = m_link ? shareString(attValue(atts_, "id"))
                     : Data::Image::idClean(attValue(atts_, "id"));
  m_width = attValue(atts_, "width").toInt();
  m_height = attValue(atts_, "height").toInt();
  return true;
}

bool ImageHandler::end(const QString&, const QString&, const QString&) {
  bool needToAddInfo = true;
  if(d->loadImages && !d->text.isEmpty()) {
    QByteArray ba;
    KCodecs::base64Decode(QByteArray(d->text.toLatin1()), ba);
    if(!ba.isEmpty()) {
      QString result = ImageFactory::addImage(ba, m_format, m_imageId);
      if(result.isEmpty()) {
        myDebug() << "null image for" << m_imageId;
      }
      d->hasImages = true;
      needToAddInfo = false;
    }
  }
  if(needToAddInfo) {
    // a width or height of 0 is ok here
    Data::ImageInfo info(m_imageId, m_format.toLatin1(), m_width, m_height, m_link);
    ImageFactory::cacheImageInfo(info);
  }
  return true;
}

StateHandler* FiltersHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("filter")) {
    return new FilterHandler(d);
  }
  return 0;
}

bool FiltersHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  return true;
}

bool FiltersHandler::end(const QString&, const QString&, const QString&) {
  return true;
}

StateHandler* FilterHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("rule")) {
    return new FilterRuleHandler(d);
  }
  return 0;
}

bool FilterHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  d->filter = new Filter(Filter::MatchAny);
  d->filter->setName(attValue(atts_, "name"));

  if(attValue(atts_, "match") == QLatin1String("all")) {
    d->filter->setMatch(Filter::MatchAll);
  }
  return true;
}

bool FilterHandler::end(const QString&, const QString&, const QString&) {
  if(d->coll && !d->filter->isEmpty()) {
    d->coll->addFilter(d->filter);
  }
  d->filter = FilterPtr();
  return true;
}

bool FilterRuleHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  QString field = attValue(atts_, "field");
  // empty field means match any of them
  QString pattern = attValue(atts_, "pattern");
  // empty pattern is bad
  if(pattern.isEmpty()) {
    myWarning() << "empty rule!";
    return true;
  }
  QString function = attValue(atts_, "function").toLower();
  FilterRule::Function func;
  if(function == QLatin1String("contains")) {
    func = FilterRule::FuncContains;
  } else if(function == QLatin1String("notcontains")) {
    func = FilterRule::FuncNotContains;
  } else if(function == QLatin1String("equals")) {
    func = FilterRule::FuncEquals;
  } else if(function == QLatin1String("notequals")) {
    func = FilterRule::FuncNotEquals;
  } else if(function == QLatin1String("regexp")) {
    func = FilterRule::FuncRegExp;
  } else if(function == QLatin1String("notregexp")) {
    func = FilterRule::FuncNotRegExp;
  } else {
    myWarning() << "invalid rule function:" << function;
    return true;
  }
  d->filter->append(new FilterRule(field, pattern, func));
  return true;
}

bool FilterRuleHandler::end(const QString&, const QString&, const QString&) {
  return true;
}

StateHandler* BorrowersHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("borrower")) {
    return new BorrowerHandler(d);
  }
  return 0;
}

bool BorrowersHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes&) {
  return true;
}

bool BorrowersHandler::end(const QString&, const QString&, const QString&) {
  return true;
}

StateHandler* BorrowerHandler::nextHandlerImpl(const QString&, const QString& localName_, const QString&) {
  if(localName_ == QLatin1String("loan")) {
    return new LoanHandler(d);
  }
  return 0;
}

bool BorrowerHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  QString name = attValue(atts_, "name");
  QString uid = attValue(atts_, "uid");
  d->borrower = new Data::Borrower(name, uid);

  return true;
}

bool BorrowerHandler::end(const QString&, const QString&, const QString&) {
  if(d->coll && !d->borrower->isEmpty()) {
    d->coll->addBorrower(d->borrower);
  }
  d->borrower = Data::BorrowerPtr();
  return true;
}

bool LoanHandler::start(const QString&, const QString&, const QString&, const QXmlAttributes& atts_) {
  m_id = attValue(atts_, "entryRef").toInt();
  m_uid = attValue(atts_, "uid");
  m_loanDate = attValue(atts_, "loanDate");
  m_dueDate = attValue(atts_, "dueDate");
  m_inCalendar = attValue(atts_, "calendar") == QLatin1String("true");
  return true;
}

bool LoanHandler::end(const QString&, const QString&, const QString&) {
  Data::EntryPtr entry = d->coll->entryById(m_id);
  if(!entry) {
    myWarning() << "no entry with id = " << m_id;
    return true;
  }
  QDate loanDate, dueDate;
  if(!m_loanDate.isEmpty()) {
    loanDate = QDate::fromString(m_loanDate, Qt::ISODate);
  }
  if(!m_dueDate.isEmpty()) {
    dueDate = QDate::fromString(m_dueDate, Qt::ISODate);
  }

  Data::LoanPtr loan(new Data::Loan(entry, loanDate, dueDate, d->text));
  loan->setUID(m_uid);
  loan->setInCalendar(m_inCalendar);
  d->borrower->addLoan(loan);
  return true;
}

