/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bookcaseimporter.h"
#include "bookcasexmlexporter.h" // needed solely for syntaxVersion
#include "../collectionfactory.h"
#include "../collections/bibtexcollection.h"
#include "../imagefactory.h"
#include "../strings.h"

#include <klocale.h>
#include <kmdcodec.h>
#include <kzip.h>
#include <kdebug.h>

#include <qbuffer.h>
#if QT_VERSION < 0x030100
#include <qregexp.h> // needed for string replacement
#endif

using Bookcase::Import::BookcaseImporter;

/*
 * VERSION 2 added namespaces, changed to multiple elements,
 * and changed the "keywords" field to "keyword"
 *
 * VERSION 3 broke out the formatFlag, and changed NoComplete to AllowCompletion
 *
 * VERSION 4 added a bibtex-field name for Bibtex collections, element name was
 * changed to 'entry', field elements changed to 'field', and boolean fields are now "true"
 *
 * VERSION 5 moved the bibtex-field and any other extended field properties to property elements
 * inside the field element, and added the image element.
 */

BookcaseImporter::BookcaseImporter(const KURL& url_) : DataImporter(url_), m_coll(0)  {
}

BookcaseImporter::BookcaseImporter(const QString& text_) : DataImporter(text_), m_coll(0) {
}

Bookcase::Data::Collection* BookcaseImporter::collection() {
  if(!m_coll) {
    const QByteArray& ba = data();
    if(ba.size() < 5) {
      return 0;
    }
    // need to decide if the data is xml text, or a zip file
    // if the first 5 characters are <?xml then treat it like text
    if(ba[0] == '<' && ba[1] == '?' && ba[2] == 'x' && ba[3] == 'm' && ba[4] == 'l') {
      m_fraction = 1.0;
      loadXMLData(ba, true);
    } else {
      loadZipData(ba);
    }
  }
  return m_coll;
}

void BookcaseImporter::loadXMLData(const QByteArray& data_, bool loadImages_) {
  QDomDocument dom;
  QString errorMsg;
  int errorLine, errorColumn;
  if(!dom.setContent(data_, false, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n(loadError).arg(url().fileName()) + QString::fromLatin1("\n");
    str += i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    setStatusMessage(str);
    return;
  }

  QDomElement root = dom.documentElement();
  if(root.tagName() != QString::fromLatin1("bookcase")) {
    if(!url().isEmpty()) {
      setStatusMessage(i18n(loadError).arg(url().fileName()));
    }
    return;
  }

  // the syntax version field name changed from "version" to "syntaxVersion" in version 3
  unsigned syntaxVersion;
  if(root.hasAttribute(QString::fromLatin1("syntaxVersion"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("syntaxVersion")).toInt();
  } else if (root.hasAttribute(QString::fromLatin1("version"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("version")).toInt();
  } else {
    if(!url().isEmpty()) {
      setStatusMessage(i18n(loadError).arg(url().fileName()));
    }
    return;
  }

  if(syntaxVersion > Export::BookcaseXMLExporter::syntaxVersion) {
    if(!url().isEmpty()) {
      QString str = i18n(loadError).arg(url().fileName());
      str += QString::fromLatin1("\n");
      str += i18n("It is from a future version of Bookcase.");
      setStatusMessage(str);
    }
    return;
  } else if(syntaxVersion < Export::BookcaseXMLExporter::syntaxVersion) {
    QString str = i18n("Bookcase is converting the file to a more recent document format. "
                       "Information loss may occur if an older version of Bookcase is used "
                       "to read this file in the future.");
    kdDebug() << str <<  endl;
//    setStatusMessage(str);
  }

  QDomNodeList collelems = root.elementsByTagName(QString::fromLatin1("collection"));
  if(collelems.count() > 1) {
    kdWarning() << "BookcaseImporter::loadDomDocument() - There is more than one collection."
                   "This isn't supported at the moment. Only the first will be loaded." << endl;
  }

  QDomElement collelem = collelems.item(0).toElement();
  QString title = collelem.attribute(QString::fromLatin1("title"));
  QString unitTitle = collelem.attribute(QString::fromLatin1("unitTitle"));

  QDomNodeList fieldelems;
  if(syntaxVersion < 4) {
    fieldelems = collelem.elementsByTagName(QString::fromLatin1("attribute"));
  } else {
    fieldelems = collelem.elementsByTagName(QString::fromLatin1("field"));
  }
//  kdDebug() << "BookcaseDoc::loadDomDocument() - " << fieldelems.count() << " field(s)" << endl;

  // the dilemma is when to force the new collection to have all the default attributes
  // if there are no attributes or if the first one is not the title, then add defaults
  bool addFields = (fieldelems.count() == 0);
  if(!addFields) {
    QString name = fieldelems.item(0).toElement().attribute(QString::fromLatin1("name"));
    addFields = (name != QString::fromLatin1("title"));
  }

  QString entryName;

  // in syntax 4, the element name was changed to "entry", always, rather than depending on
  // on the entryName of the collection. A type field was added to the collection element
  // to specify what type of collection it is.
  if(syntaxVersion < 4) {
    entryName = collelem.attribute(QString::fromLatin1("unit"));
    m_coll = CollectionFactory::collection(entryName, addFields);
  } else {
    entryName = QString::fromLatin1("entry");
    QString typeStr = collelem.attribute(QString::fromLatin1("type"));
    Data::Collection::CollectionType type = static_cast<Data::Collection::CollectionType>(typeStr.toInt());
    m_coll = CollectionFactory::collection(type, addFields);
  }

  if(!title.isEmpty()) {
    m_coll->setTitle(title);
  }

  for(unsigned j = 0; j < fieldelems.count(); ++j) {
    readField(syntaxVersion, fieldelems.item(j).toElement());
  }

  if(m_coll->collectionType() == Data::Collection::Bibtex) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(m_coll);
    QDomNodeList macroelems = collelem.elementsByTagName(QString::fromLatin1("macro"));
//    kdDebug() << "BookcaseDoc::loadDomDocument() - found " << macroelems.count() << " macros" << endl;
    for(unsigned j = 0; c && j < macroelems.count(); ++j) {
      QDomElement elem = macroelems.item(j).toElement();
      c->addMacro(elem.attribute(QString::fromLatin1("name")), elem.text());
    }

    QDomNodeList preelems = collelem.elementsByTagName(QString::fromLatin1("bibtex-preamble"));
    if(preelems.count() > 0) {
      QString pre = preelems.item(0).toElement().text();
      c->setPreamble(pre);
    }
  }

  QDomNodeList entryelems = collelem.elementsByTagName(entryName);
//  kdDebug() << QString("BookcaseDoc::loadDomDocument() - There are %1 %2(s) "
//                         "in the collection.").arg(unitelems.count()).arg(entryName) << endl;

//  as a special case, for old book collections with a bibtex-id field, convert to Bibtex
  if(syntaxVersion < 4 && m_coll->collectionType() == Data::Collection::Book
     && m_coll->fieldByName(QString::fromLatin1("bibtex-id")) != 0) {
    Data::BibtexCollection* c = Data::BibtexCollection::convertBookCollection(m_coll);
    delete m_coll;
    m_coll = c;
  }

  unsigned count = entryelems.count();
  for(unsigned j = 0; j < count; ++j) {
    readEntry(syntaxVersion, entryelems.item(j));

    if(j%QMAX(s_stepSize, count/100) == 0) {
      emit signalFractionDone(m_fraction*static_cast<float>(j)/static_cast<float>(count));
    }
  } // end entry loop

  if(loadImages_) {
    // images are contained in the root element, not the collection
    QDomNodeList imgelems = root.elementsByTagName(QString::fromLatin1("image"));

    for(unsigned j = 0; j < imgelems.count(); ++j) {
      readImage(imgelems.item(j).toElement());
    }
  }

  return;
}

void BookcaseImporter::readField(unsigned syntaxVersion_, const QDomElement& elem_) {
  QString name  = elem_.attribute(QString::fromLatin1("name"), QString::fromLatin1("unknown"));
  QString title = elem_.attribute(QString::fromLatin1("title"), i18n("Unknown"));

  QString typeStr = elem_.attribute(QString::fromLatin1("type"), QString::number(Data::Field::Line));
  Data::Field::FieldType type = static_cast<Data::Field::FieldType>(typeStr.toInt());

  Data::Field* field;
  if(type == Data::Field::Choice) {
    QString allowed = elem_.attribute(QString::fromLatin1("allowed"));
    field = new Data::Field(name, title, QStringList::split(QString::fromLatin1(";"), allowed));
  } else {
    field = new Data::Field(name, title, type);
  }

  if(elem_.hasAttribute(QString::fromLatin1("category"))) {
    // at one point, the categories had keyboard accels
    QString cat = elem_.attribute(QString::fromLatin1("category"));
    if(cat.find('&') > -1) {
      // Qt 3.0.x doesn't have QString::replace(QChar, ...)
#if QT_VERSION >= 0x030100
      cat.replace('&', QString::null);
#else
      cat.replace(QRegExp(QString::fromLatin1("&")), QString::null);
#endif
    }
    field->setCategory(cat);
  }

  if(elem_.hasAttribute(QString::fromLatin1("flags"))) {
    int flags = elem_.attribute(QString::fromLatin1("flags")).toInt();
    // I also changed the enum values for syntax 3, but the only custom field
    // would have been bibtex-id
    if(syntaxVersion_ < 3 && field->name() == QString::fromLatin1("bibtex-id")) {
      flags = 0;
    }

    // in syntax version 4, added a flag to disallow deleting attributes
    // if it's a version before that and is the title, then add the flag
    if(syntaxVersion_ < 4 && field->name() == QString::fromLatin1("title")) {
      flags |= Data::Field::NoDelete;
    }
    field->setFlags(flags);
  }

  QString formatStr = elem_.attribute(QString::fromLatin1("format"), QString::number(Data::Field::FormatNone));
  Data::Field::FormatFlag format = static_cast<Data::Field::FormatFlag>(formatStr.toInt());
  field->setFormatFlag(format);

  if(elem_.hasAttribute(QString::fromLatin1("description"))) {
    field->setDescription(elem_.attribute(QString::fromLatin1("description")));
  }

  if(syntaxVersion_ >= 5) {
    QDomNodeList props = elem_.elementsByTagName(QString::fromLatin1("prop"));
    for(unsigned i = 0; i < props.count(); ++i) {
      QDomElement e = props.item(i).toElement();
      field->setProperty(e.attribute(QString::fromLatin1("name")), e.text());
    }
  } else if(elem_.hasAttribute(QString::fromLatin1("bibtex-field"))) {
    field->setProperty(QString::fromLatin1("bibtex"), elem_.attribute(QString::fromLatin1("bibtex-field")));
  }

  m_coll->addField(field);
//  kdDebug() << QString("  Added field: %1, %2").arg(field->name()).arg(field->title()) << endl;
}

void BookcaseImporter::readEntry(unsigned syntaxVersion_, const QDomNode& entryNode_) {
  Data::Entry* entry = new Data::Entry(m_coll);
  // ierate over all field value children
  for(QDomNode node = entryNode_.firstChild(); !node.isNull(); node = node.nextSibling()) {
    // Entry::setField checks to see if an field of 'name' is allowed
    // in version 3 and prior, checkbox attributes had no text(), set it to "true" now
    if(syntaxVersion_ < 4 && node.toElement().text().isEmpty()) {
      // "true" means checked
      entry->setField(node.toElement().tagName(), QString::fromLatin1("true"));
    } else {
      QString name = node.toElement().tagName();

      // if the first child of the node is a text node, just set the attribute text
      // otherwise, recurse over the node's children
      // this is the case for <authors><author>..</author></authors>
      if(node.firstChild().nodeType() == QDomNode::TextNode) {
        // if it's a derived value, no field value is added
        if(m_coll->fieldByName(name)->type() == Data::Field::Dependent) {
          continue;
        }
        // in version 2, "keywords" changed to "keyword"
        if(syntaxVersion_ < 2 && name == QString::fromLatin1("keywords")) {
          name = QString::fromLatin1("keyword");
        }
        entry->setField(name, node.toElement().text());
      } else { // if not, then it has children, iterate through them
        // the field name has the final 's', so remove it
        name.truncate(name.length() - 1);
        // if it's a derived value, no field value is added
        if(m_coll->fieldByName(name)->type() == Data::Field::Dependent) {
          continue;
        }

        QString value;
        QDomNode childNode = node.firstChild();
        // is it a 2-column table
        bool table2 = (m_coll->fieldByName(name)->type() == Data::Field::Table2);
        // concatenate values
        for( ; !childNode.isNull(); childNode = childNode.nextSibling()) {
          if(table2) {
            value += childNode.firstChild().toElement().text();
            value += QString::fromLatin1("::");
            value += childNode.lastChild().toElement().text();
          } else {
            value += childNode.toElement().text();
          }
          value += QString::fromLatin1("; ");
        }
        // remove the last semi-colon and space
        value.truncate(value.length() - 2);
        entry->setField(name, value);
      }
    }
  } // end field value loop

  // no need to call slotAddEntry, it just sends a signal and changes modified flag
  m_coll->addEntry(entry);
}

void BookcaseImporter::readImage(const QDomElement& imgelem) {
  QString format = imgelem.attribute(QString::fromLatin1("format"));
  QString id = imgelem.attribute(QString::fromLatin1("id"));

  QByteArray ba;
  KCodecs::base64Decode(QCString(imgelem.text().latin1()), ba);
  ImageFactory::addImage(ba, format, id);
//  const Data::Image& img = ImageFactory::addImage(ba, format, id, filename);
//  kdDebug() << "BookcaseImporter::readImage() - char length: " << imgelem.text().length() << endl;
//  kdDebug() << "BookcaseImporter::readImage() - image length: " << img.numBytes() << endl;
}

void BookcaseImporter::loadZipData(const QByteArray& data_) {
  QBuffer buf(data_);
  KZip zip(&buf);
  if(!zip.open(IO_ReadOnly)) {
    setStatusMessage(i18n(loadError).arg(url().fileName()));
    return;
  }

  const KArchiveDirectory* dir = zip.directory();

  const KArchiveEntry* entry = dir->entry(QString::fromLatin1("bookcase.xml"));
  if(!entry || !entry->isFile()) {
    return;
  }

  const QByteArray xmlData = static_cast<const KArchiveFile*>(entry)->data();
  m_fraction = static_cast<float>(xmlData.size())/static_cast<float>(data_.size());
  loadXMLData(xmlData, false);
  m_fraction = 1.0;
  if(!m_coll) {
    return;
  }

  const KArchiveEntry* imgDir = dir->entry(QString::fromLatin1("images"));
  if(!imgDir || !imgDir->isDirectory()) {
    return;
  }

  QStringList images = static_cast<const KArchiveDirectory*>(imgDir)->entries();
  unsigned count = images.count();
  unsigned j = 0;
  for(QStringList::ConstIterator it = images.begin(); it != images.end(); ++it, ++j) {
    const KArchiveEntry* file = static_cast<const KArchiveDirectory*>(imgDir)->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section('.', -1).upper(), *it);
    }
    if(j%QMAX(s_stepSize, count/100) == 0) {
      emit signalFractionDone(m_fraction*static_cast<float>(j)/static_cast<float>(count));
    }
  }

  zip.close();
}
