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

#include "tellicoimporter.h"
#include "../collectionfactory.h"
#include "../collections/bibtexcollection.h"
#include "../imagefactory.h"
#include "../isbnvalidator.h"
#include "../latin1literal.h"
#include "../tellico_strings.h"
#include "tellico_xml.h"

#include <klocale.h>
#include <kmdcodec.h>
#include <kzip.h>
#include <kdebug.h>
#include <kglobal.h> // for KMAX

#include <qbuffer.h>

using Tellico::Import::TellicoImporter;

TellicoImporter::TellicoImporter(const KURL& url_) : DataImporter(url_), m_coll(0)  {
}

TellicoImporter::TellicoImporter(const QString& text_) : DataImporter(text_), m_coll(0) {
}

Tellico::Data::Collection* TellicoImporter::collection() {
  if(!m_coll) {
    const QByteArray& ba = data();
    if(ba.size() < 5) {
      return 0;
    }
    // need to decide if the data is xml text, or a zip file
    // if the first 5 characters are <?xml then treat it like text
    if(ba[0] == '<' && ba[1] == '?' && ba[2] == 'x' && ba[3] == 'm' && ba[4] == 'l') {
      loadXMLData(ba, true);
    } else {
      loadZipData(ba);
    }
  }
  return m_coll;
}

void TellicoImporter::loadXMLData(const QByteArray& data_, bool loadImages_) {
  QDomDocument dom;
  QString errorMsg;
  int errorLine, errorColumn;
  if(!dom.setContent(data_, true, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
    str += i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    setStatusMessage(str);
    return;
  }

  QDomElement root = dom.documentElement();

  // the syntax version field name changed from "version" to "syntaxVersion" in version 3
  uint syntaxVersion;
  if(root.hasAttribute(QString::fromLatin1("syntaxVersion"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("syntaxVersion")).toInt();
  } else if (root.hasAttribute(QString::fromLatin1("version"))) {
    syntaxVersion = root.attribute(QString::fromLatin1("version")).toInt();
  } else {
    if(!url().isEmpty()) {
      setStatusMessage(i18n(errorLoad).arg(url().fileName()));
    }
    return;
  }
//  kdDebug() << "TellicoImporter::loadXMLData() - syntaxVersion = " << syntaxVersion << endl;

  if((syntaxVersion > 6 && root.tagName() != Latin1Literal("tellico"))
     || (syntaxVersion < 7 && root.tagName() != Latin1Literal("bookcase"))) {
    if(!url().isEmpty()) {
      setStatusMessage(i18n(errorLoad).arg(url().fileName()));
    }
    return;
  }

  if(syntaxVersion > XML::syntaxVersion) {
    if(!url().isEmpty()) {
      QString str = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
      str += i18n("It is from a future version of Tellico.");
      setStatusMessage(str);
    }
    return;
  } else if(syntaxVersion < XML::syntaxVersion) {
    QString str = i18n("Tellico is converting the file to a more recent document format. "
                       "Information loss may occur if an older version of Tellico is used "
                       "to read this file in the future.");
    kdDebug() << str <<  endl;
//    setStatusMessage(str);
  }

  m_namespace = syntaxVersion > 6 ? XML::nsTellico : XML::nsBookcase;
  QDomNodeList collelems = root.elementsByTagNameNS(m_namespace, QString::fromLatin1("collection"));
  if(collelems.count() > 1) {
    kdWarning() << "TellicoImporter::loadDomDocument() - There is more than one collection."
                   "This isn't supported at the moment. Only the first will be loaded." << endl;
  }

  QDomElement collelem = collelems.item(0).toElement();
  QString title = collelem.attribute(QString::fromLatin1("title"));
  QString entryTitle = collelem.attribute((syntaxVersion > 6) ? QString::fromLatin1("entryTitle")
                                           : QString::fromLatin1("unitTitle"));

  QDomNodeList fieldelems = collelem.elementsByTagNameNS(m_namespace, (syntaxVersion > 3) ? QString::fromLatin1("field")
                                                : QString::fromLatin1("attribute"));
//  kdDebug() << "TellicoImporter::loadXMLData() - " << fieldelems.count() << " field(s)" << endl;

  // the dilemma is when to force the new collection to have all the default attributes
  // if there are no attributes or if the first one has the special name of _default
  bool addFields = (fieldelems.count() == 0);
  if(!addFields) {
    QString name = fieldelems.item(0).toElement().attribute(QString::fromLatin1("name"));
    addFields = (name == Latin1Literal("_default"));
    // removeChild only works for immediate children
    // remove _default field
    if(addFields) {
      fieldelems.item(0).parentNode().removeChild(fieldelems.item(0));
    }
  }

  QString entryName;
  // in syntax 4, the element name was changed to "entry", always, rather than depending on
  // on the entryName of the collection. A type field was added to the collection element
  // to specify what type of collection it is.
  if(syntaxVersion > 3) {
    entryName = QString::fromLatin1("entry");
    QString typeStr = collelem.attribute(QString::fromLatin1("type"));
    Data::Collection::Type type = static_cast<Data::Collection::Type>(typeStr.toInt());
    m_coll = CollectionFactory::collection(type, addFields, entryTitle);
  } else {
    entryName = collelem.attribute(QString::fromLatin1("unit"));
    m_coll = CollectionFactory::collection(entryName, addFields);
  }

  if(!title.isEmpty()) {
    m_coll->setTitle(title);
  }

  for(unsigned j = 0; j < fieldelems.count(); ++j) {
    readField(syntaxVersion, fieldelems.item(j).toElement());
  }

  if(m_coll->type() == Data::Collection::Bibtex) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(m_coll);
    QDomNodeList macroelems = collelem.elementsByTagNameNS(m_namespace, QString::fromLatin1("macro"));
//    kdDebug() << "TellicoImporter::loadXMLData() - found " << macroelems.count() << " macros" << endl;
    for(unsigned j = 0; c && j < macroelems.count(); ++j) {
      QDomElement elem = macroelems.item(j).toElement();
      c->addMacro(elem.attribute(QString::fromLatin1("name")), elem.text());
    }

    QDomNodeList preelems = collelem.elementsByTagNameNS(m_namespace, QString::fromLatin1("bibtex-preamble"));
    if(preelems.count() > 0) {
      QString pre = preelems.item(0).toElement().text();
      c->setPreamble(pre);
    }
  }

  QDomNodeList entryelems = collelem.elementsByTagNameNS(m_namespace, entryName);
//  kdDebug() << QString("TellicoImporter::loadXMLData() - There are %1 %2(s) "
//                         "in the collection.").arg(entryelems.count()).arg(entryName) << endl;

//  as a special case, for old book collections with a bibtex-id field, convert to Bibtex
  if(syntaxVersion < 4 && m_coll->type() == Data::Collection::Book
     && m_coll->fieldByName(QString::fromLatin1("bibtex-id")) != 0) {
    Data::BibtexCollection* c = Data::BibtexCollection::convertBookCollection(m_coll);
    delete m_coll;
    m_coll = c;
  }

  const uint count = entryelems.count();
  for(unsigned j = 0; j < count; ++j) {
    readEntry(syntaxVersion, entryelems.item(j));

    if(j%KMAX(s_stepSize, count/100) == 0) {
      emit signalFractionDone(static_cast<float>(j)/static_cast<float>(count));
    }
  } // end entry loop

  if(loadImages_) {
    // images are contained in the root element, not the collection
    QDomNodeList imgelems = root.elementsByTagNameNS(m_namespace, QString::fromLatin1("image"));

    for(unsigned j = 0; j < imgelems.count(); ++j) {
      readImage(imgelems.item(j).toElement());
    }
  }
}

void TellicoImporter::readField(unsigned syntaxVersion_, const QDomElement& elem_) {
  // special case: if the i18n attribute equals true, then translate the title, description, and category
  bool isI18n = elem_.attribute(QString::fromLatin1("i18n")) == Latin1Literal("true");

  QString name  = elem_.attribute(QString::fromLatin1("name"), QString::fromLatin1("unknown"));
  QString title = elem_.attribute(QString::fromLatin1("title"), i18n("Unknown"));
  if(isI18n) {
    title = i18n(title.utf8());
  }

  QString typeStr =elem_.attribute(QString::fromLatin1("type"), QString::number(Data::Field::Line));
  Data::Field::Type type = static_cast<Data::Field::Type>(typeStr.toInt());

  Data::Field* field;
  if(type == Data::Field::Choice) {
    QStringList allowed = QStringList::split(QString::fromLatin1(";"),
                                             elem_.attribute(QString::fromLatin1("allowed")));
    if(isI18n) {
      for(QStringList::Iterator it = allowed.begin(); it != allowed.end(); ++it) {
        (*it) = i18n((*it).utf8());
      }
    }
    field = new Data::Field(name, title, allowed);
  } else {
    field = new Data::Field(name, title, type);
  }

  if(elem_.hasAttribute(QString::fromLatin1("category"))) {
    // at one point, the categories had keyboard accels
    QString cat = elem_.attribute(QString::fromLatin1("category"));
    if(cat.find('&') > -1) {
      cat.remove('&');
    }
    if(isI18n) {
      cat = i18n(cat.utf8());
    }
    field->setCategory(cat);
  }

  if(elem_.hasAttribute(QString::fromLatin1("flags"))) {
    int flags = elem_.attribute(QString::fromLatin1("flags")).toInt();
    // I also changed the enum values for syntax 3, but the only custom field
    // would have been bibtex-id
    if(syntaxVersion_ < 3 && field->name() == Latin1Literal("bibtex-id")) {
      flags = 0;
    }

    // in syntax version 4, added a flag to disallow deleting attributes
    // if it's a version before that and is the title, then add the flag
    if(syntaxVersion_ < 4 && field->name() == Latin1Literal("title")) {
      flags |= Data::Field::NoDelete;
    }
    field->setFlags(flags);
  }

  QString formatStr = elem_.attribute(QString::fromLatin1("format"), QString::number(Data::Field::FormatNone));
  Data::Field::FormatFlag format = static_cast<Data::Field::FormatFlag>(formatStr.toInt());
  field->setFormatFlag(format);

  if(elem_.hasAttribute(QString::fromLatin1("description"))) {
    QString desc = elem_.attribute(QString::fromLatin1("description"));
    if(isI18n) {
      desc = i18n(desc.utf8());
    }
    field->setDescription(desc);
  }

  if(syntaxVersion_ >= 5) {
    QDomNodeList props = elem_.elementsByTagNameNS(m_namespace, QString::fromLatin1("prop"));
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

void TellicoImporter::readEntry(unsigned syntaxVersion_, const QDomNode& entryNode_) {
  Data::Entry* entry = new Data::Entry(m_coll);
  // ierate over all field value children
  for(QDomNode node = entryNode_.firstChild(); !node.isNull(); node = node.nextSibling()) {
    QDomElement elem = node.toElement();
    if(elem.isNull()) {
      continue;
    }

    bool isI18n = elem.attribute(QString::fromLatin1("i18n")) == Latin1Literal("true");

    // Entry::setField checks to see if an field of 'name' is allowed
    // in version 3 and prior, checkbox attributes had no text(), set it to "true" now
    if(syntaxVersion_ < 4 && elem.text().isEmpty()) {
      // "true" means checked
      entry->setField(elem.tagName(), QString::fromLatin1("true"));
    } else {
      QString name = elem.tagName();
      Data::Field* f = m_coll->fieldByName(name);

      // if the first child of the node is a text node, just set the attribute text
      // otherwise, recurse over the node's children
      // this is the case for <authors><author>..</author></authors>
      // but if there's nothing but white space, then it's a BaseNode for some reason
//      if(node.firstChild().nodeType() == QDomNode::TextNode) {
      if(f) {
        // if it's a derived value, no field value is added
        if(f->type() == Data::Field::Dependent) {
          continue;
        }
        // special case for Date fields
        if(f->type() == Data::Field::Date) {
          if(elem.hasChildNodes()) {
            QString value;
            QDomNode yNode = elem.elementsByTagNameNS(m_namespace, QString::fromLatin1("year")).item(0);
            if(!yNode.isNull()) {
              value += yNode.toElement().text();
            }
            value += '-';
            QDomNode mNode = elem.elementsByTagNameNS(m_namespace, QString::fromLatin1("month")).item(0);
            if(!mNode.isNull()) {
              value += mNode.toElement().text();
            }
            value += '-';
            QDomNode dNode = elem.elementsByTagNameNS(m_namespace, QString::fromLatin1("day")).item(0);
            if(!dNode.isNull()) {
              value += dNode.toElement().text();
            }
            entry->setField(name, value);
          } else {
            // if no child nodes, the code will later assume the value to be the year
            entry->setField(name, elem.text());
          }
          // go to next value in loop
          continue;
        }
        QString value = elem.text();
        if(value.isEmpty()) {
          continue;
        }
        // in version 2, "keywords" changed to "keyword"
        if(syntaxVersion_ < 2 && name == Latin1Literal("keywords")) {
          name = QString::fromLatin1("keyword");
        }
        // special case: if the i18n attribute equals true, then translate the title, description, and category
        if(isI18n) {
          entry->setField(name, i18n(value.utf8()));
        } else {
          // special case for isbn fields, go ahead and validate
          if(name == Latin1Literal("isbn")) {
            static const ISBNValidator val(0);
            if(elem.attribute(QString::fromLatin1("validate")) != Latin1Literal("no")) {
              val.fixup(value);
            }
          }
          entry->setField(name, value);
        }
      } else { // if not, then it has children, iterate through them
        // the field name has the final 's', so remove it
        name.truncate(name.length() - 1);
        f = m_coll->fieldByName(name);

         // if it's a derived value, no field value is added
        if(!f || f->type() == Data::Field::Dependent) {
          continue;
        }

        QString value;
        // is it a 2-column table
        bool isTable2 = (f->type() == Data::Field::Table2);
        // concatenate values
        for(QDomNode childNode = node.firstChild(); !childNode.isNull(); childNode = childNode.nextSibling()) {
          // don't worry about i18n here, Tables are never translated
          if(isTable2) {
            value += childNode.firstChild().toElement().text();
            if(childNode.childNodes().count() == 2) {
              value += QString::fromLatin1("::");
              value += childNode.lastChild().toElement().text();
            }
          } else {
            // this causes problems with some characters
            // japanese, for instance, where i18n(text) != text
            if(isI18n) {
              value += i18n(childNode.toElement().text().utf8());
            } else {
              value += childNode.toElement().text();
            }
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

void TellicoImporter::readImage(const QDomElement& imgelem) {
  QString format = imgelem.attribute(QString::fromLatin1("format"));
  QString id = imgelem.attribute(QString::fromLatin1("id"));

  QByteArray ba;
  KCodecs::base64Decode(QCString(imgelem.text().latin1()), ba);
  ImageFactory::addImage(ba, format, id);
}

void TellicoImporter::loadZipData(const QByteArray& data_) {
  QBuffer buf(data_);
  KZip zip(&buf);
  if(!zip.open(IO_ReadOnly)) {
    setStatusMessage(i18n(errorLoad).arg(url().fileName()));
    return;
  }

  const KArchiveDirectory* dir = zip.directory();
  if(!dir) {
    QString str = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
    str += i18n("The file is empty.");
    setStatusMessage(str);
    zip.close();
    return;
  }

  // main file was changed from bookcase.xml to tellico.xml as of version 0.13
  const KArchiveEntry* entry = dir->entry(QString::fromLatin1("tellico.xml"));
  if(!entry) {
    entry = dir->entry(QString::fromLatin1("bookcase.xml"));
  }
  if(!entry || !entry->isFile()) {
    QString str = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
    str += i18n("The file contains no collection data.");
    setStatusMessage(str);
    zip.close();
    return;
  }

  const QByteArray xmlData = static_cast<const KArchiveFile*>(entry)->data();
  loadXMLData(xmlData, false);
  if(!m_coll) {
    zip.close();
    return;
  }

  const KArchiveEntry* imgDir = dir->entry(QString::fromLatin1("images"));
  if(!imgDir || !imgDir->isDirectory()) {
    zip.close();
    return;
  }

  QStringList images = static_cast<const KArchiveDirectory*>(imgDir)->entries();
  for(QStringList::ConstIterator it = images.begin(); it != images.end(); ++it) {
    const KArchiveEntry* file = static_cast<const KArchiveDirectory*>(imgDir)->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section('.', -1).upper(), *it);
    }
  }

  zip.close();
}

#include "tellicoimporter.moc"
