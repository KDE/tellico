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
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QRegularExpression>

namespace {

inline
QString attValue(const QXmlStreamAttributes& atts, const char* name, const QString& defaultValue=QString()) {
  return atts.hasAttribute(QLatin1String(name)) ? atts.value(QLatin1String(name)).toString() : defaultValue;
}

inline
QString attValue(const QXmlStreamAttributes& atts, const char* name, const char* defaultValue) {
  Q_ASSERT(defaultValue);
  return attValue(atts, name, QLatin1String(defaultValue));
}

inline
QString realFieldName(int syntaxVersion, const QStringRef& localName) {
  return (syntaxVersion < 2 && localName == QLatin1String("keywords")) ?
    QStringLiteral("keyword") :
    localName.toString();
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

StateHandler* StateHandler::nextHandler(const QStringRef& ns_, const QStringRef& localName_) {
  StateHandler* handler = nextHandlerImpl(ns_, localName_);
  if(!handler) {
    myWarning() << "no handler for" << localName_;
  }
  return handler ? handler : new NullHandler(d);
}

StateHandler* RootHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("tellico") || localName_ == QLatin1String("bookcase")) {
    return new DocumentHandler(d);
  }
  return new RootHandler(d);
}

StateHandler* DocumentHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("collection")) {
    return new CollectionHandler(d);
  } else if(localName_ == QLatin1String("filters")) {
    return new FiltersHandler(d);
  } else if(localName_ == QLatin1String("borrowers")) {
    return new BorrowersHandler(d);
  }
  return nullptr;
}

bool DocumentHandler::start(const QStringRef&, const QStringRef& localName_, const QXmlStreamAttributes& atts_) {
  // the syntax version field name changed from "version" to "syntaxVersion" in version 3
  QStringRef syntaxVersion = atts_.value(QLatin1String("syntaxVersion"));
  if(syntaxVersion.isEmpty()) {
    syntaxVersion = atts_.value(QLatin1String("version"));
    if(syntaxVersion.isEmpty()) {
      myWarning() << "no syntax version";
      return false;
    }
  }
  d->syntaxVersion = syntaxVersion.toUInt();
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

bool DocumentHandler::end(const QStringRef&, const QStringRef&) {
  return true;
}

StateHandler* CollectionHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
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
  return nullptr;
}

bool CollectionHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  d->collTitle = attValue(atts_, "title");
  d->collType = atts_.value(QLatin1String("type")).toInt();
  if(d->syntaxVersion > 6) {
    d->entryName = QStringLiteral("entry");
  } else {
    // old attribute
    d->entryName = attValue(atts_, "unit");
    // for error recovery, assume entry name is default if empty for now
    if(d->entryName.isEmpty()) {
      d->entryName = QLatin1String("entry");
    }
  }
  return true;
}

bool CollectionHandler::end(const QStringRef&, const QStringRef&) {
  if(!d->coll) {
    myWarning() << "no collection created";
    return false;
  }
  d->coll->addEntries(d->entries);

  // a little hidden capability was to just have a local path as an image file name
  // and on reading the xml file, Tellico would load the image file, too
  // here, we need to scan all the image values in all the entries and check
  // maybe this is too costly, especially since the capability wasn't advertised?

  const int maxImageWarnings = 3;
  int imageWarnings = 0;

  Data::FieldList fields = d->coll->imageFields();
  foreach(Data::EntryPtr entry, d->entries) {
    foreach(Data::FieldPtr field, fields) {
      QString value = entry->field(field);
      if(value.isEmpty()) {
        continue;
      }
      // image info should have already been loaded
      // if not, then there was no <image> in the XML
      // so it's a url, but maybe link only
      if(!ImageFactory::hasImageInfo(value)) {
        QUrl u(value); // previously used QUrl::fromUserInput but now expect proper url
        // also allow relative image urls
        if(u.isRelative()) {
          if(d->baseUrl.isEmpty()) {
            // assume a local file, as fromUserInput() would do
            u = QUrl::fromLocalFile(value);
          } else {
            u = d->baseUrl.resolved(u);
          }
        }
        // the image file name is a valid URL, but I want it to be a local URL or non empty remote one
        if(u.isValid()) {
          if(u.scheme() == QLatin1String("data")) {
            const QByteArray ba = QByteArray::fromPercentEncoding(u.path(QUrl::FullyEncoded).toLatin1());
            const int pos = ba.indexOf(',');
            if(ba.startsWith("image/") && pos > -1) {
              const QByteArray header = ba.left(pos);
              const int pos2 = header.indexOf(';');
              // we can only read images in base64
              if(header.contains("base64") && pos2 > -1) {
                const QByteArray format = header.left(pos2).mid(6); // remove "image/";
                const QString result = ImageFactory::addImage(QByteArray::fromBase64(ba.mid(pos+1)),
                                                              QString::fromLatin1(format));
                if(result.isEmpty()) {
                  // clear value for the field in this case
                  value.clear();
                  ++imageWarnings;
                } else {
                  value = result;
                }
              }
            }
          } else if(u.isLocalFile() || !u.host().isEmpty()) {
            const QString result = ImageFactory::addImage(u, !d->showImageLoadErrors || imageWarnings >= maxImageWarnings /* quiet */);
            if(result.isEmpty()) {
              // clear value for the field in this case
              value.clear();
              ++imageWarnings;
            } else {
              value = result;
            }
          }
        } else {
          value = Data::Image::idClean(value);
        }
        // reset the image id to be whatever was loaded
        entry->setField(field->name(), value, false /* no modified date update */);
      }
    }
  }
  return true;
}

StateHandler* FieldsHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if((d->syntaxVersion > 3 && localName_ == QLatin1String("field")) ||
     (d->syntaxVersion < 4 && localName_ == QLatin1String("attribute"))) {
    return new FieldHandler(d);
  }
  return nullptr;
}

bool FieldsHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  d->defaultFields = false;
  return true;
}

bool FieldsHandler::end(const QStringRef&, const QStringRef&) {
  // add default fields if there was a default field name, or no names at all
  const bool addFields = d->defaultFields || d->fields.isEmpty();
  // in syntax 4, the element name was changed to "entry", always, rather than depending on
  // on the entryName of the collection.
  if(d->syntaxVersion > 3) {
    d->entryName = QStringLiteral("entry");
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
    d->coll->addField(Data::Field::createDefaultField(Data::Field::IDField));
  }
  // now add all the new fields
  d->coll->addFields(d->fields);
  if(d->syntaxVersion < 11 && !d->defaultFields) {
    d->coll->addField(Data::Field::createDefaultField(Data::Field::CreatedDateField));
    d->coll->addField(Data::Field::createDefaultField(Data::Field::ModifiedDateField));
  }

//  as a special case, for old book collections with a bibtex-id field, convert to Bibtex
  if(d->syntaxVersion < 4 && d->collType == Data::Collection::Book
     && d->coll->hasField(QStringLiteral("bibtex-id"))) {
    d->coll = Data::BibtexCollection::convertBookCollection(d->coll);
  }

  return true;
}

StateHandler* FieldHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("prop")) {
    return new FieldPropertyHandler(d);
  }
  return nullptr;
}

bool FieldHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  // special case: if the i18n attribute equals true, then translate the title, description, category, and allowed
  isI18n = atts_.value(QLatin1String("i18n")) == QLatin1String("true");

  const QString name = attValue(atts_, "name", "unknown");
  if(name == QLatin1String("_default")) {
    d->defaultFields = true;
    return true;
  }

  QString title  = attValue(atts_, "title", i18n("Unknown"));
  if(isI18n && !title.isEmpty()) {
    title = i18n(title.toUtf8().constData());
  }

  QString typeStr = attValue(atts_, "type", QString::number(Data::Field::Line));
  Data::Field::Type type = static_cast<Data::Field::Type>(typeStr.toInt());

  Data::FieldPtr field;
  if(type == Data::Field::Choice) {
    QStringList allowed = FieldFormat::splitValue(attValue(atts_, "allowed"), FieldFormat::RegExpSplit);
    if(isI18n) {
      for(QStringList::Iterator word = allowed.begin(); word != allowed.end(); ++word) {
        (*word) = i18n((*word).toUtf8().constData());
      }
    }
    field = new Data::Field(name, title, allowed);
  } else {
    field = new Data::Field(name, title, type);
  }

  QString cat = attValue(atts_, "category");
  // at one point, the categories had keyboard accels
  if(d->syntaxVersion < 9) {
    cat.remove(QLatin1Char('&'));
  }
  if(isI18n && !cat.isEmpty()) {
    cat = i18n(cat.toUtf8().constData());
  }
  field->setCategory(cat);

  int flags = atts_.value(QLatin1String("flags")).toInt();
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

  QString formatStr = attValue(atts_, "format", QString::number(FieldFormat::FormatNone));
  FieldFormat::Type formatType = static_cast<FieldFormat::Type>(formatStr.toInt());
  field->setFormatType(formatType);

  QString desc = attValue(atts_, "description");
  if(isI18n && !desc.isEmpty()) {
    desc = i18n(desc.toUtf8().constData());
  }
  field->setDescription(desc);

  if(d->syntaxVersion < 5 && atts_.hasAttribute(QLatin1String("bibtex-field"))) {
    field->setProperty(QStringLiteral("bibtex"), attValue(atts_, "bibtex-field"));
  }

  // for syntax 8, rating fields got their own type
  if(d->syntaxVersion < 8) {
    Data::Field::convertOldRating(field); // does all its own checking
  }
  d->fields.append(field);

  return true;
}

bool FieldHandler::end(const QStringRef&, const QStringRef&) {
  // the value template for derived values used to be the field description
  // now it is the 'template' property
  // for derived value fields, if there is no property and the description has a '%'
  // move it to the property
  //
  // might be empty is we're only adding default fields
  if(!d->fields.isEmpty()) {
    Data::FieldPtr field = d->fields.back();
    if(field->hasFlag(Data::Field::Derived) &&
       field->property(QStringLiteral("template")).isEmpty() &&
       field->description().contains(QLatin1Char('%'))) {
      field->setProperty(QStringLiteral("template"), field->description());
      field->setDescription(QString());
    } else if(isI18n && field->type() == Data::Field::Table) {
      // translate table column headers if requestsed (such as Title, Artist, etc.
      const auto props = field->propertyList();
      for(auto i = props.constBegin(); i != props.constEnd(); ++i) {
        if(i.key().startsWith(QLatin1String("column"))) {
          field->setProperty(i.key(), i18n(i.value().toUtf8().constData()));
        }
      }
    }
  }

  return true;
}

bool FieldPropertyHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  // there should be at least one field already so we can add properties to it
  Q_ASSERT(!d->fields.isEmpty());
  Data::FieldPtr field = d->fields.back();

  m_propertyName = attValue(atts_, "name");

  // all track fields in music collections prior to version 9 get converted to three columns
  if(d->syntaxVersion < 9) {
    if(d->collType == Data::Collection::Album && field->name() == QLatin1String("track")) {
      field->setProperty(QStringLiteral("columns"), QStringLiteral("3"));
      field->setProperty(QStringLiteral("column1"), i18n("Title"));
      field->setProperty(QStringLiteral("column2"), i18n("Artist"));
      field->setProperty(QStringLiteral("column3"), i18n("Length"));
    } else if(d->collType == Data::Collection::Video && field->name() == QLatin1String("cast")) {
      field->setProperty(QStringLiteral("column1"), i18n("Actor/Actress"));
      field->setProperty(QStringLiteral("column2"), i18n("Role"));
    }
  }

  return true;
}

bool FieldPropertyHandler::end(const QStringRef&, const QStringRef&) {
  Q_ASSERT(!m_propertyName.isEmpty());
  // add the previous property
  Data::FieldPtr field = d->fields.back();
  field->setProperty(m_propertyName, d->text);
  return true;
}

bool BibtexPreambleHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  return true;
}

bool BibtexPreambleHandler::end(const QStringRef&, const QStringRef&) {
  Q_ASSERT(d->coll);
  if(d->coll && d->collType == Data::Collection::Bibtex && !d->text.isEmpty()) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(d->coll.data());
    c->setPreamble(d->text);
  }
  return true;
}

StateHandler* BibtexMacrosHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("macro")) {
    return new BibtexMacroHandler(d);
  }
  return nullptr;
}

bool BibtexMacrosHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  return true;
}

bool BibtexMacrosHandler::end(const QStringRef&, const QStringRef&) {
  return true;
}

bool BibtexMacroHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  m_macroName = attValue(atts_, "name");
  return true;
}

bool BibtexMacroHandler::end(const QStringRef&, const QStringRef&) {
  if(d->coll && d->collType == Data::Collection::Bibtex && !m_macroName.isEmpty() && !d->text.isEmpty()) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(d->coll.data());
    c->addMacro(m_macroName, d->text);
  }
  return true;
}

StateHandler* EntryHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(d->coll->hasField(realFieldName(d->syntaxVersion, localName_))) {
    return new FieldValueHandler(d);
  }
  return new FieldValueContainerHandler(d);
}

bool EntryHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  // the entries must come after the fields
  if(!d->coll || d->coll->fields().isEmpty()) {
    // special case for very old versions which did not have user-editable fields
    // also maybe a new version has bad formatting, try to recover by assuming default fields
    d->defaultFields = true;
    FieldsHandler handler(d);
    // fake the end of a fields element, which will add the default fields
    handler.end(QStringRef(), QStringRef());
    myWarning() << "entries should come after fields are defined, attempting to recover";
  }
  bool ok;
  const int id = atts_.value(QLatin1String("id")).toInt(&ok);
  Data::EntryPtr entry;
  if(ok && id > -1) {
    entry = new Data::Entry(d->coll, id);
  } else {
    entry = new Data::Entry(d->coll);
  }
  d->entries.append(entry);
  return true;
}

bool EntryHandler::end(const QStringRef&, const QStringRef&) {
  Data::EntryPtr entry = d->entries.back();
  Q_ASSERT(entry);
  if(!d->modifiedDate.isEmpty() && d->coll->hasField(QStringLiteral("mdate"))) {
    entry->setField(QStringLiteral("mdate"), d->modifiedDate);
    d->modifiedDate.clear();
  }
  return true;
}

StateHandler* FieldValueContainerHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(d->coll->hasField(realFieldName(d->syntaxVersion, localName_))) {
    return new FieldValueHandler(d);
  }
  return new FieldValueContainerHandler(d);
}

bool FieldValueContainerHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  return true;
}

bool FieldValueContainerHandler::end(const QStringRef&, const QStringRef&) {
  Data::FieldPtr f = d->currentField;
  if(f && f->type() == Data::Field::Table) {
    Data::EntryPtr entry = d->entries.back();
    Q_ASSERT(entry);
    QString fieldValue = entry->field(f->name());
    // don't allow table value to end with empty row
    while(fieldValue.endsWith(FieldFormat::rowDelimiterString())) {
      fieldValue.chop(FieldFormat::rowDelimiterString().length());
      // no need to update the modified date when setting the entry's field value
      entry->setField(f->name(), fieldValue, false /* no modified date update */);
    }
  }

  return true;
}

StateHandler* FieldValueHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("year") ||
     localName_ == QLatin1String("month") ||
     localName_ == QLatin1String("day")) {
    return new DateValueHandler(d);
  } else if(localName_ == QLatin1String("column")) {
    return new TableColumnHandler(d);
  }
  return nullptr;
}

bool FieldValueHandler::start(const QStringRef&, const QStringRef& localName_, const QXmlStreamAttributes& atts_) {
  d->currentField = d->coll->fieldByName(realFieldName(d->syntaxVersion, localName_));
  Q_ASSERT(d->currentField);
  m_i18n = atts_.value(QLatin1String("i18n")) == QLatin1String("true");
  m_validateISBN = (localName_ == QLatin1String("isbn")) &&
                   (atts_.value(QLatin1String("validate")) != QLatin1String("no"));
  return true;
}

bool FieldValueHandler::end(const QStringRef&, const QStringRef& localName_) {
  Data::EntryPtr entry = d->entries.back();
  Q_ASSERT(entry);
  QString fieldName = d->currentField ? d->currentField->name() : realFieldName(d->syntaxVersion, localName_);

  Data::FieldPtr f = d->currentField;
  if(!f) {
    myWarning() << "no field named " << fieldName;
    return true;
  }
  // if it's a derived value, no field value is added
  if(f->hasFlag(Data::Field::Derived)) {
    return true;
  }

  QString fieldValue = d->text;
  if(d->syntaxVersion < 4 && f->type() == Data::Field::Bool) {
    // in version 3 and prior, checkbox attributes had no text(), set it to "true"
    fieldValue = QStringLiteral("true");
  } else if(d->syntaxVersion < 8 && f->type() == Data::Field::Rating) {
    // in version 8, old rating fields get changed
    bool ok;
    uint i = Tellico::toUInt(fieldValue, &ok);
    if(ok) {
      fieldValue = QString::number(i);
    }
  } else if(!d->textBuffer.isEmpty()) {
    // for dates and tables, the value is built up from child elements
    if(!d->text.isEmpty()) {
      myWarning() << "ignoring value for field" << localName_ << ":" << d->text;
    }
    fieldValue = d->textBuffer;
    // the text buffer has the column delimiter at the end, remove it
    if(f->type() == Data::Field::Table) {
      fieldValue.chop(FieldFormat::columnDelimiterString().length());
    }
    d->textBuffer.clear();
  } else if(fieldValue.isEmpty() && f->type() == Data::Field::Table) {
    // allow for empty table rows
    fieldValue = FieldFormat::rowDelimiterString();
  }
  // this is not an else branch, the data may be in the textBuffer
  if(d->syntaxVersion < 9 && d->coll->type() == Data::Collection::Album && fieldName == QLatin1String("track")) {
    // yes, this assumes the artist has already been set
    fieldValue += FieldFormat::columnDelimiterString();
    fieldValue += entry->field(QStringLiteral("artist"));
  }
  if(fieldValue.isEmpty()) {
    return true;
  }

  // special case: if the i18n attribute equals true, then translate the title, description, and category
  if(m_i18n) {
    fieldValue = i18n(fieldValue.toUtf8().constData());
  }
  // special case for isbn fields, go ahead and validate
  if(m_validateISBN) {
    ISBNValidator val(nullptr);
    val.fixup(fieldValue);
  }
  if(f->type() == Data::Field::Table) {
    QString oldValue = entry->field(fieldName);
    if(!oldValue.isEmpty()) {
      if(!oldValue.endsWith(FieldFormat::rowDelimiterString())) {
        oldValue += FieldFormat::rowDelimiterString();
      }
      fieldValue.prepend(oldValue);
    }
  } else if(f->hasFlag(Data::Field::AllowMultiple)) {
    // for fields with multiple values, we need to add on the new value
    const QString oldValue = entry->field(fieldName);
    if(!oldValue.isEmpty()) {
      fieldValue = oldValue + FieldFormat::delimiterString() + fieldValue;
    }
  }

  // since the modified date value in the entry gets changed every time we set a new value
  // we have to save it and set it after changing all the others
  if(fieldName == QLatin1String("mdate")) {
    d->modifiedDate = fieldValue;
  } else {
    // no need to update the modified date when setting the entry's field value
    entry->setField(fieldName, fieldValue, false /* no modified date update */);
  }
  return true;
}

bool DateValueHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  return true;
}

bool DateValueHandler::end(const QStringRef&, const QStringRef& localName_) {
  QStringList tokens;
  if(d->textBuffer.isEmpty()) {
    // the data value is y-m-d even if there are no date values, so create list of blank tokens
    tokens = QStringList() << QString() << QString() << QString();
  } else {
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    tokens = d->textBuffer.split(QLatin1Char('-'), QString::KeepEmptyParts);
#else
    tokens = d->textBuffer.split(QLatin1Char('-'), Qt::KeepEmptyParts);
#endif
  }
  Q_ASSERT(tokens.size() == 3);
  while(tokens.size() < 3) {
    tokens += QString();
  }
  if(localName_ == QLatin1String("year")) {
    tokens[0] = d->text;
  } else if(localName_ == QLatin1String("month")) {
    // enforce two digits for month
    while(d->text.length() < 2) {
      d->text.prepend(QLatin1Char('0'));
    }
    tokens[1] = d->text;
  } else if(localName_ == QLatin1String("day")) {
    // enforce two digits for day
    while(d->text.length() < 2) {
      d->text.prepend(QLatin1Char('0'));
    }
    tokens[2] = d->text;
  }
  d->textBuffer = tokens.join(QLatin1String("-"));
  return true;
}

bool TableColumnHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  return true;
}

bool TableColumnHandler::end(const QStringRef&, const QStringRef&) {
  // for old collections, if the second column holds the track length, bump it to next column
  if(d->syntaxVersion < 9 &&
     d->coll->type() == Data::Collection::Album &&
     d->currentField->name() == QLatin1String("track") &&
     !d->textBuffer.isEmpty() &&
     d->textBuffer.contains(FieldFormat::columnDelimiterString()) == 0) {
    static const QRegularExpression rx(QLatin1String("^\\d+:\\d\\d$"));
    if(rx.match(d->text).hasMatch()) {
      d->text += FieldFormat::columnDelimiterString();
      d->text += d->entries.back()->field(QStringLiteral("artist"));
    }
  }

  d->textBuffer += d->text + FieldFormat::columnDelimiterString();
  return true;
}

StateHandler* ImagesHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("image")) {
    return new ImageHandler(d);
  }
  return nullptr;
}

bool ImagesHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  // reset variable that gets updated in the image handler
  d->hasImages = false;
  return true;
}

bool ImagesHandler::end(const QStringRef&, const QStringRef&) {
  return true;
}

bool ImageHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  m_format = attValue(atts_, "format");
  m_link = atts_.value(QLatin1String("link")) == QLatin1String("true");
  // idClean() already calls shareString()
  m_imageId = m_link ? shareString(attValue(atts_, "id"))
                     : Data::Image::idClean(attValue(atts_, "id"));
  m_width = atts_.value(QLatin1String("width")).toInt();
  m_height = atts_.value(QLatin1String("height")).toInt();
  return true;
}

bool ImageHandler::end(const QStringRef&, const QStringRef&) {
  bool needToAddInfo = true;
  if(d->loadImages && !d->text.isEmpty()) {
    QByteArray ba = QByteArray::fromBase64(d->text.toLatin1());
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

StateHandler* FiltersHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("filter")) {
    return new FilterHandler(d);
  }
  return nullptr;
}

bool FiltersHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  return true;
}

bool FiltersHandler::end(const QStringRef&, const QStringRef&) {
  return true;
}

StateHandler* FilterHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("rule")) {
    return new FilterRuleHandler(d);
  }
  return nullptr;
}

bool FilterHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  d->filter = new Filter(Filter::MatchAny);
  d->filter->setName(attValue(atts_, "name"));

  if(atts_.value(QLatin1String("match")) == QLatin1String("all")) {
    d->filter->setMatch(Filter::MatchAll);
  }
  return true;
}

bool FilterHandler::end(const QStringRef&, const QStringRef&) {
  if(d->coll && !d->filter->isEmpty()) {
    d->coll->addFilter(d->filter);
  }
  d->filter = FilterPtr();
  return true;
}

bool FilterRuleHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  QString field = attValue(atts_, "field");
  // empty field means match any of them
  QString pattern = attValue(atts_, "pattern");
  // empty pattern is bad
  if(pattern.isEmpty()) {
    myWarning() << "empty rule!";
    return true;
  }
  /* If anything is updated here, be sure to update tellicoxmlexporter */
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
  } else if(function == QLatin1String("before")) {
    func = FilterRule::FuncBefore;
  } else if(function == QLatin1String("after")) {
    func = FilterRule::FuncAfter;
  } else if(function == QLatin1String("greaterthan")) {
    func = FilterRule::FuncGreater;
  } else if(function == QLatin1String("lessthan")) {
    func = FilterRule::FuncLess;
  } else {
    myWarning() << "invalid rule function:" << function;
    return true;
  }
  d->filter->append(new FilterRule(field, pattern, func));
  return true;
}

bool FilterRuleHandler::end(const QStringRef&, const QStringRef&) {
  return true;
}

StateHandler* BorrowersHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("borrower")) {
    return new BorrowerHandler(d);
  }
  return nullptr;
}

bool BorrowersHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) {
  return true;
}

bool BorrowersHandler::end(const QStringRef&, const QStringRef&) {
  return true;
}

StateHandler* BorrowerHandler::nextHandlerImpl(const QStringRef&, const QStringRef& localName_) {
  if(localName_ == QLatin1String("loan")) {
    return new LoanHandler(d);
  }
  return nullptr;
}

bool BorrowerHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  QString name = attValue(atts_, "name");
  QString uid = attValue(atts_, "uid");
  d->borrower = new Data::Borrower(name, uid);

  return true;
}

bool BorrowerHandler::end(const QStringRef&, const QStringRef&) {
  if(d->coll && !d->borrower->isEmpty()) {
    d->coll->addBorrower(d->borrower);
  }
  d->borrower = Data::BorrowerPtr();
  return true;
}

bool LoanHandler::start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes& atts_) {
  m_id = attValue(atts_, "entryRef").toInt();
  m_uid = attValue(atts_, "uid");
  m_loanDate = attValue(atts_, "loanDate");
  m_dueDate = attValue(atts_, "dueDate");
  m_inCalendar = atts_.value(QLatin1String("calendar")) == QLatin1String("true");
  return true;
}

bool LoanHandler::end(const QStringRef&, const QStringRef&) {
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
