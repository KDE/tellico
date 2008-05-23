/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "tellico_xml.h"
#include "../collectionfactory.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../imagefactory.h"
#include "../image.h"
#include "../isbnvalidator.h"
#include "../latin1literal.h"
#include "../tellico_strings.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"
#include "../progressmanager.h"

#include <klocale.h>
#include <kmdcodec.h>
#include <kzip.h>
#include <kapplication.h>

#include <qdom.h>
#include <qbuffer.h>
#include <qfile.h>
#include <qtimer.h>

using Tellico::Import::TellicoImporter;

bool TellicoImporter::versionConversion(uint from, uint to) {
  // version 10 only added board games to version 9
  return from < to && (from != 9 || to != 10);
}

TellicoImporter::TellicoImporter(const KURL& url_, bool loadAllImages_) : DataImporter(url_),
    m_coll(0), m_loadAllImages(loadAllImages_), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(0), m_zip(0), m_imgDir(0) {
}

TellicoImporter::TellicoImporter(const QString& text_) : DataImporter(text_),
    m_coll(0), m_loadAllImages(true), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(0), m_zip(0), m_imgDir(0) {
}

TellicoImporter::~TellicoImporter() {
  if(m_zip) {
    m_zip->close();
  }
  delete m_zip;
  m_zip = 0;
  delete m_buffer;
  m_buffer = 0;
}

Tellico::Data::CollPtr TellicoImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  QCString s; // read first 5 characters
  if(source() == URL) {
    if(!fileRef().open()) {
      return 0;
    }
    QIODevice* f = fileRef().file();
    for(uint i = 0; i < 5; ++i) {
      s += static_cast<char>(f->getch());
    }
    f->reset();
  } else {
    if(data().size() < 5) {
      m_format = Error;
      return 0;
    }
    s = QCString(data(), 6);
  }

  // need to decide if the data is xml text, or a zip file
  // if the first 5 characters are <?xml then treat it like text
  if(s[0] == '<' && s[1] == '?' && s[2] == 'x' && s[3] == 'm' && s[4] == 'l') {
    m_format = XML;
    loadXMLData(source() == URL ? fileRef().file()->readAll() : data(), true);
  } else {
    m_format = Zip;
    loadZipData();
  }
  return m_coll;
}

void TellicoImporter::loadXMLData(const QByteArray& data_, bool loadImages_) {
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  QDomDocument dom;
  QString errorMsg;
  int errorLine, errorColumn;
  if(!dom.setContent(data_, true, &errorMsg, &errorLine, &errorColumn)) {
    QString str = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
    str += i18n("There is an XML parsing error in line %1, column %2.").arg(errorLine).arg(errorColumn);
    str += QString::fromLatin1("\n");
    str += i18n("The error message from Qt is:");
    str += QString::fromLatin1("\n\t") + errorMsg;
    myDebug() << str << endl;
    setStatusMessage(str);
    m_format = Error;
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
    m_format = Error;
    return;
  }
//  myDebug() << "TellicoImporter::loadXMLData() - syntaxVersion = " << syntaxVersion << endl;

  if((syntaxVersion > 6 && root.tagName() != Latin1Literal("tellico"))
     || (syntaxVersion < 7 && root.tagName() != Latin1Literal("bookcase"))) {
    if(!url().isEmpty()) {
      setStatusMessage(i18n(errorLoad).arg(url().fileName()));
    }
    m_format = Error;
    return;
  }

  if(syntaxVersion > XML::syntaxVersion) {
    if(!url().isEmpty()) {
      QString str = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
      str += i18n("It is from a future version of Tellico.");
      myDebug() << str << endl;
      setStatusMessage(str);
    } else {
      myDebug() << "Unable to load collection, from a future version (" << syntaxVersion << ")" << endl;
    }
    m_format = Error;
    return;
  } else if(versionConversion(syntaxVersion, XML::syntaxVersion)) {
    // going form version 9 to 10, there's no conversion needed
    QString str = i18n("Tellico is converting the file to a more recent document format. "
                       "Information loss may occur if an older version of Tellico is used "
                       "to read this file in the future.");
    myDebug() << str <<  endl;
//    setStatusMessage(str);
    m_modified = true; // mark as modified
  }

  m_namespace = syntaxVersion > 6 ? XML::nsTellico : XML::nsBookcase;

  // the collection item should be the first dom element child of the root
  QDomElement collelem;
  for(QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if(n.namespaceURI() != m_namespace) {
      continue;
    }
    if(n.isElement() && n.localName() == Latin1Literal("collection")) {
      collelem = n.toElement();
      break;
    }
  }
  if(collelem.isNull()) {
    kdWarning() << "TellicoImporter::loadDomDocument() - No collection item found." << endl;
    return;
  }

  QString title = collelem.attribute(QString::fromLatin1("title"));

  // be careful not to have element name collision
  // for fields, each true field element is a child of a fields element
  QDomNodeList fieldelems;
  for(QDomNode n = collelem.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if(n.namespaceURI() != m_namespace) {
      continue;
    }
    // Latin1Literal is a macro, so can't say Latin1Literal(syntaxVersion > 3 ? "fields" : "attributes")
    if((syntaxVersion > 3 && n.localName() == Latin1Literal("fields"))
       || (syntaxVersion < 4 && n.localName() == Latin1Literal("attributes"))) {
      QDomElement e = n.toElement();
      fieldelems = e.elementsByTagNameNS(m_namespace, (syntaxVersion > 3) ? QString::fromLatin1("field")
                                                                          : QString::fromLatin1("attribute"));
      break;
    }
  }
//  myDebug() << "TellicoImporter::loadXMLData() - " << fieldelems.count() << " field(s)" << endl;

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
    m_coll = CollectionFactory::collection(type, addFields);
  } else {
    entryName = collelem.attribute(QString::fromLatin1("unit"));
    m_coll = CollectionFactory::collection(entryName, addFields);
  }

  if(!title.isEmpty()) {
    m_coll->setTitle(title);
  }

  for(uint j = 0; j < fieldelems.count(); ++j) {
    readField(syntaxVersion, fieldelems.item(j).toElement());
  }

  if(m_coll->type() == Data::Collection::Bibtex) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(m_coll.data());
    QDomNodeList macroelems;
    for(QDomNode n = collelem.firstChild(); !n.isNull(); n = n.nextSibling()) {
      if(n.namespaceURI() != m_namespace) {
        continue;
      }
      if(n.localName() == Latin1Literal("macros")) {
        macroelems = n.toElement().elementsByTagNameNS(m_namespace, QString::fromLatin1("macro"));
        break;
      }
    }
//    myDebug() << "TellicoImporter::loadXMLData() - found " << macroelems.count() << " macros" << endl;
    for(uint j = 0; c && j < macroelems.count(); ++j) {
      QDomElement elem = macroelems.item(j).toElement();
      c->addMacro(elem.attribute(QString::fromLatin1("name")), elem.text());
    }

    for(QDomNode n = collelem.firstChild(); !n.isNull(); n = n.nextSibling()) {
      if(n.namespaceURI() != m_namespace) {
        continue;
      }
      if(n.localName() == Latin1Literal("bibtex-preamble")) {
        c->setPreamble(n.toElement().text());
        break;
      }
    }
  }

  if(m_cancelled) {
    m_coll = 0;
    return;
  }

//  as a special case, for old book collections with a bibtex-id field, convert to Bibtex
  if(syntaxVersion < 4 && m_coll->type() == Data::Collection::Book
     && m_coll->hasField(QString::fromLatin1("bibtex-id"))) {
    m_coll = Data::BibtexCollection::convertBookCollection(m_coll);
  }

  const uint count = collelem.childNodes().count();
  const uint stepSize = QMAX(s_stepSize, count/100);
  const bool showProgress = options() & ImportProgress;

  item.setTotalSteps(count);

  // have to read images before entries so we can figure out if
  // linkOnly() is true
  // m_loadAllImages only pertains to zip files
  QDomNodeList imgelems;
  for(QDomNode n = collelem.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if(n.namespaceURI() != m_namespace) {
      continue;
    }
    if(n.localName() == Latin1Literal("images")) {
      imgelems = n.toElement().elementsByTagNameNS(m_namespace, QString::fromLatin1("image"));
      break;
    }
  }
  for(uint j = 0; j < imgelems.count(); ++j) {
    readImage(imgelems.item(j).toElement(), loadImages_);
  }

  if(m_cancelled) {
    m_coll = 0;
    return;
  }

  uint j = 0;
  for(QDomNode n = collelem.firstChild(); !n.isNull() && !m_cancelled; n = n.nextSibling(), ++j) {
    if(n.namespaceURI() != m_namespace) {
      continue;
    }
    if(n.localName() == entryName) {
      readEntry(syntaxVersion, n.toElement());

      // not exactly right, but close enough
      if(showProgress && j%stepSize == 0) {
        ProgressManager::self()->setProgress(this, j);
        kapp->processEvents();
      }
    } else {
//      myDebug() << "...skipping " << n.localName() << " (" << n.namespaceURI() << ")" << endl;
    }
  } // end entry loop

  if(m_cancelled) {
    m_coll = 0;
    return;
  }

  // filters and borrowers are at document root level, not collection
  for(QDomNode n = root.firstChild(); !n.isNull() && !m_cancelled; n = n.nextSibling()) {
    if(n.namespaceURI() != m_namespace) {
      continue;
    }
    if(n.localName() == Latin1Literal("borrowers")) {
      QDomNodeList borrowerElems = n.toElement().elementsByTagNameNS(m_namespace, QString::fromLatin1("borrower"));
      for(uint j = 0; j < borrowerElems.count(); ++j) {
        readBorrower(borrowerElems.item(j).toElement());
      }
    } else if(n.localName() == Latin1Literal("filters")) {
      QDomNodeList filterElems = n.toElement().elementsByTagNameNS(m_namespace, QString::fromLatin1("filter"));
      for(uint j = 0; j < filterElems.count(); ++j) {
        readFilter(filterElems.item(j).toElement());
      }
    }
  }

  // special for user, if using an older document format, add some nice new filters
  if(syntaxVersion < 8) {
    addDefaultFilters();
  }

  if(m_cancelled) {
    m_coll = 0;
  }
}

void TellicoImporter::readField(uint syntaxVersion_, const QDomElement& elem_) {
  // special case: if the i18n attribute equals true, then translate the title, description, and category
  bool isI18n = elem_.attribute(QString::fromLatin1("i18n")) == Latin1Literal("true");

  QString name  = elem_.attribute(QString::fromLatin1("name"), QString::fromLatin1("unknown"));
  QString title = elem_.attribute(QString::fromLatin1("title"), i18n("Unknown"));
  if(isI18n) {
    title = i18n(title.utf8());
  }

  QString typeStr = elem_.attribute(QString::fromLatin1("type"), QString::number(Data::Field::Line));
  Data::Field::Type type = static_cast<Data::Field::Type>(typeStr.toInt());

  Data::FieldPtr field;
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
    if(syntaxVersion_ < 9 && cat.find('&') > -1) {
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
    for(uint i = 0; i < props.count(); ++i) {
      QDomElement e = props.item(i).toElement();
      field->setProperty(e.attribute(QString::fromLatin1("name")), e.text());
    }
    // all track fields in music collections prior to version 9 get converted to three columns
    if(syntaxVersion_ < 9) {
      if(m_coll->type() == Data::Collection::Album && field->name() == Latin1Literal("track")) {
        field->setProperty(QString::fromLatin1("columns"), QChar('3'));
        field->setProperty(QString::fromLatin1("column1"), i18n("Title"));
        field->setProperty(QString::fromLatin1("column2"), i18n("Artist"));
        field->setProperty(QString::fromLatin1("column3"), i18n("Length"));
      } else if(m_coll->type() == Data::Collection::Video && field->name() == Latin1Literal("cast")) {
        field->setProperty(QString::fromLatin1("column1"), i18n("Actor/Actress"));
        field->setProperty(QString::fromLatin1("column2"), i18n("Role"));
      }
    }
  } else if(elem_.hasAttribute(QString::fromLatin1("bibtex-field"))) {
    field->setProperty(QString::fromLatin1("bibtex"), elem_.attribute(QString::fromLatin1("bibtex-field")));
  }

  // Table2 is deprecated
  if(field->type() == Data::Field::Table2) {
    field->setType(Data::Field::Table);
    field->setProperty(QString::fromLatin1("columns"), QChar('2'));
  }
  // for syntax 8, rating fields got their own type
  if(syntaxVersion_ < 8) {
    Data::Field::convertOldRating(field); // does all its own checking
  }
  m_coll->addField(field);
//  myDebug() << QString("  Added field: %1, %2").arg(field->name()).arg(field->title()) << endl;
}

void TellicoImporter::readEntry(uint syntaxVersion_, const QDomElement& entryElem_) {
  const int id = entryElem_.attribute(QString::fromLatin1("id")).toInt();
  Data::EntryPtr entry;
  if(id > 0) {
    entry = new Data::Entry(m_coll, id);
  } else {
    entry = new Data::Entry(m_coll);
  }

  bool oldMusic = (syntaxVersion_ < 9 && m_coll->type() == Data::Collection::Album);

  // iterate over all field value children
  for(QDomNode node = entryElem_.firstChild(); !node.isNull(); node = node.nextSibling()) {
    QDomElement elem = node.toElement();
    if(elem.isNull()) {
      continue;
    }

    bool isI18n = elem.attribute(QString::fromLatin1("i18n")) == Latin1Literal("true");

    // Entry::setField checks to see if an field of 'name' is allowed
    // in version 3 and prior, checkbox attributes had no text(), set it to "true" now
    if(syntaxVersion_ < 4 && elem.text().isEmpty()) {
      // "true" means checked
      entry->setField(elem.localName(), QString::fromLatin1("true"));
      continue;
    }

    QString name = elem.localName();
    Data::FieldPtr f = m_coll->fieldByName(name);

    // if the first child of the node is a text node, just set the attribute text
    // otherwise, recurse over the node's children
    // this is the case for <authors><author>..</author></authors>
    // but if there's nothing but white space, then it's a BaseNode for some reason
//    if(node.firstChild().nodeType() == QDomNode::TextNode) {
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

      // this may be a performance hit to be stripping white space all the time
      // unfortunately, text() will include a carriage-return in cases like
      // <value>
      // text
      // </value
      // so we arbitrarily decide that only paragraphs get to have CRs?
      QString value = elem.text();
      if(f->type() != Data::Field::Para) {
        value = value.stripWhiteSpace();
      }

      if(value.isEmpty()) {
        continue;
      }

      if(f->type() == Data::Field::Image) {
        // image info should have already been loaded
        const Data::ImageInfo& info = ImageFactory::imageInfo(value);
        // possible that value needs to be cleaned first in which case info is null
        if(info.isNull() || !info.linkOnly) {
          // for local files only, allow paths here
          KURL u = KURL::fromPathOrURL(value);
          if(u.isValid() && u.isLocalFile()) {
            QString result = ImageFactory::addImage(u, false /* quiet */);
            if(!result.isEmpty()) {
              value = result;
            }
          }
          value = Data::Image::idClean(value);
        }
      }

      // in version 8, old rating fields get changed
      if(syntaxVersion_ < 8 && f->type() == Data::Field::Rating) {
        bool ok;
        uint i = Tellico::toUInt(value, &ok);
        if(ok) {
          value = QString::number(i);
        }
      } else if(syntaxVersion_ < 2 && name == Latin1Literal("keywords")) {
        // in version 2, "keywords" changed to "keyword"
        name = QString::fromLatin1("keyword");
      }
      // special case: if the i18n attribute equals true, then translate the title, description, and category
      if(isI18n) {
        entry->setField(name, i18n(value.utf8()));
      } else {
        // special case for isbn fields, go ahead and validate
        if(name == Latin1Literal("isbn")) {
          const ISBNValidator val(0);
          if(elem.attribute(QString::fromLatin1("validate")) != Latin1Literal("no")) {
            val.fixup(value);
          }
        }
        entry->setField(name, value);
      }
    } else { // if no field by the tag name, then it has children, iterate through them
      // the field name has the final 's', so remove it
      name.truncate(name.length() - 1);
      f = m_coll->fieldByName(name);

      // if it's a derived value, no field value is added
      if(!f || f->type() == Data::Field::Dependent) {
        continue;
      }

      const bool oldTracks = (oldMusic && name == Latin1Literal("track"));

      QStringList values;
      // concatenate values
      for(QDomNode childNode = node.firstChild(); !childNode.isNull(); childNode = childNode.nextSibling()) {
        QString value;
        // don't worry about i18n here, Tables are never translated
        QDomNodeList cols = childNode.toElement().elementsByTagNameNS(m_namespace, QString::fromLatin1("column"));
        if(cols.count() > 0) {
          for(uint i = 0; i < cols.count(); ++i) {
            // special case for old tracks
            if(oldTracks && i == 1) {
              // if the second column holds the track length, bump it to next column
              QRegExp rx(QString::fromLatin1("\\d+:\\d\\d"));
              if(rx.exactMatch(cols.item(i).toElement().text())) {
                value += entry->field(QString::fromLatin1("artist"));
                value += QString::fromLatin1("::");
              }
            }
            value += cols.item(i).toElement().text().stripWhiteSpace();
            if(i < cols.count()-1) {
              value += QString::fromLatin1("::");
            } else if(oldTracks && cols.count() == 1) {
              value += QString::fromLatin1("::");
              value += entry->field(QString::fromLatin1("artist"));
            }
          }
          values += value;
        } else {
          // really loose here, we don't even check that the element name
          // is what we think it is
          QString s = childNode.toElement().text().stripWhiteSpace();
          if(isI18n && !s.isEmpty()) {
            value += i18n(s.utf8());
          } else {
            value += s;
          }
          if(oldTracks) {
            value += QString::fromLatin1("::");
            value += entry->field(QString::fromLatin1("artist"));
          }
          if(values.findIndex(value) == -1) {
            values += value;
          }
        }
      }
      entry->setField(name, values.join(QString::fromLatin1("; ")));
    }
  } // end field value loop

  m_coll->addEntries(entry);
}

void TellicoImporter::readImage(const QDomElement& elem_, bool loadImage_) {
  QString format = elem_.attribute(QString::fromLatin1("format"));
  const bool link = elem_.attribute(QString::fromLatin1("link")) == Latin1Literal("true");
  QString id = shareString(link ? elem_.attribute(QString::fromLatin1("id"))
                                : Data::Image::idClean(elem_.attribute(QString::fromLatin1("id"))));

  bool readInfo = true;
  if(loadImage_) {
    QByteArray ba;
    KCodecs::base64Decode(QCString(elem_.text().latin1()), ba);
    if(!ba.isEmpty()) {
      QString result = ImageFactory::addImage(ba, format, id);
      if(result.isEmpty()) {
        myDebug() << "TellicoImporter::readImage(XML) - null image for " << id << endl;
      }
      m_hasImages = true;
      readInfo = false;
    }
  }
  if(readInfo) {
    // a width or height of 0 is ok here
    int width = elem_.attribute(QString::fromLatin1("width")).toInt();
    int height = elem_.attribute(QString::fromLatin1("height")).toInt();
    Data::ImageInfo info(id, format.latin1(), width, height, link);
    ImageFactory::cacheImageInfo(info);
  }
}

void TellicoImporter::readFilter(const QDomElement& elem_) {
  FilterPtr f = new Filter(Filter::MatchAny);
  f->setName(elem_.attribute(QString::fromLatin1("name")));

  QString match = elem_.attribute(QString::fromLatin1("match"));
  if(match == Latin1Literal("all")) {
    f->setMatch(Filter::MatchAll);
  }

  QDomNodeList rules = elem_.elementsByTagNameNS(m_namespace, QString::fromLatin1("rule"));
  for(uint i = 0; i < rules.count(); ++i) {
    QDomElement e = rules.item(i).toElement();
    if(e.isNull()) {
      continue;
    }

    QString field = e.attribute(QString::fromLatin1("field"));
    // empty field means match any of them
    QString pattern = e.attribute(QString::fromLatin1("pattern"));
    // empty pattern is bad
    if(pattern.isEmpty()) {
      kdWarning() << "TellicoImporter::readFilter() - empty rule!" << endl;
      continue;
    }
    QString function = e.attribute(QString::fromLatin1("function")).lower();
    FilterRule::Function func;
    if(function == Latin1Literal("contains")) {
      func = FilterRule::FuncContains;
    } else if(function == Latin1Literal("notcontains")) {
      func = FilterRule::FuncNotContains;
    } else if(function == Latin1Literal("equals")) {
      func = FilterRule::FuncEquals;
    } else if(function == Latin1Literal("notequals")) {
      func = FilterRule::FuncNotEquals;
    } else if(function == Latin1Literal("regexp")) {
      func = FilterRule::FuncRegExp;
    } else if(function == Latin1Literal("notregexp")) {
      func = FilterRule::FuncNotRegExp;
    } else {
      kdWarning() << "TellicoImporter::readFilter() - invalid rule function: " << function << endl;
      continue;
    }
    f->append(new FilterRule(field, pattern, func));
  }

  if(!f->isEmpty()) {
    m_coll->addFilter(f);
  }
}

void TellicoImporter::readBorrower(const QDomElement& elem_) {
  QString name = elem_.attribute(QString::fromLatin1("name"));
  QString uid = elem_.attribute(QString::fromLatin1("uid"));
  Data::BorrowerPtr b = new Data::Borrower(name, uid);

  QDomNodeList loans = elem_.elementsByTagNameNS(m_namespace, QString::fromLatin1("loan"));
  for(uint i = 0; i < loans.count(); ++i) {
    QDomElement e = loans.item(i).toElement();
    if(e.isNull()) {
      continue;
    }
    long id = e.attribute(QString::fromLatin1("entryRef")).toLong();
    Data::EntryPtr entry = m_coll->entryById(id);
    if(!entry) {
      myDebug() << "TellicoImporter::readBorrower() - no entry with id = " << id << endl;
      continue;
    }
    QString uid = e.attribute(QString::fromLatin1("uid"));
    QDate loanDate, dueDate;
    QString s = e.attribute(QString::fromLatin1("loanDate"));
    if(!s.isEmpty()) {
      loanDate = QDate::fromString(s, Qt::ISODate);
    }
    s = e.attribute(QString::fromLatin1("dueDate"));
    if(!s.isEmpty()) {
      dueDate = QDate::fromString(s, Qt::ISODate);
    }
    Data::LoanPtr loan = new Data::Loan(entry, loanDate, dueDate, e.text());
    loan->setUID(uid);
    b->addLoan(loan);
    s = e.attribute(QString::fromLatin1("calendar"));
    loan->setInCalendar(s == Latin1Literal("true"));
  }
  if(!b->isEmpty()) {
    m_coll->addBorrower(b);
  }
}

void TellicoImporter::loadZipData() {
  delete m_buffer;
  delete m_zip;
  if(source() == URL) {
    m_buffer = 0;
    m_zip = new KZip(fileRef().fileName());
  } else {
    m_buffer = new QBuffer(data());
    m_zip = new KZip(m_buffer);
  }
  if(!m_zip->open(IO_ReadOnly)) {
    setStatusMessage(i18n(errorLoad).arg(url().fileName()));
    m_format = Error;
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const KArchiveDirectory* dir = m_zip->directory();
  if(!dir) {
    QString str = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
    str += i18n("The file is empty.");
    setStatusMessage(str);
    m_format = Error;
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
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
    m_format = Error;
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const QByteArray xmlData = static_cast<const KArchiveFile*>(entry)->data();
  loadXMLData(xmlData, false);
  if(!m_coll) {
    m_format = Error;
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  if(m_cancelled) {
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QString::fromLatin1("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }
  m_imgDir = static_cast<const KArchiveDirectory*>(imgDirEntry);
  m_images.clear();
  m_images.add(m_imgDir->entries());
  m_hasImages = !m_images.isEmpty();

  // if all the images are not to be loaded, then we're done
  if(!m_loadAllImages) {
//    myLog() << "TellicoImporter::loadZipData() - delayed loading for " << m_images.count() << " images" << endl;
    return;
  }

  const QStringList images = static_cast<const KArchiveDirectory*>(imgDirEntry)->entries();
  const uint stepSize = QMAX(s_stepSize, images.count()/100);

  uint j = 0;
  for(QStringList::ConstIterator it = images.begin(); !m_cancelled && it != images.end(); ++it, ++j) {
    const KArchiveEntry* file = m_imgDir->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section('.', -1).upper(), (*it));
      m_images.remove(*it);
    }
    if(j%stepSize == 0) {
      kapp->processEvents();
    }
  }

  if(m_images.isEmpty()) {
    // give it some time
    QTimer::singleShot(3000, this, SLOT(deleteLater()));
  }
}

bool TellicoImporter::loadImage(const QString& id_) {
//  myLog() << "TellicoImporter::loadImage() - id =  " << id_ << endl;
  if(m_format != Zip || !m_imgDir) {
    return false;
  }
  const KArchiveEntry* file = m_imgDir->entry(id_);
  if(!file || !file->isFile()) {
    return false;
  }
  QString newID = ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                                         id_.section('.', -1).upper(), id_);
  m_images.remove(id_);
  if(m_images.isEmpty()) {
    // give it some time
    QTimer::singleShot(3000, this, SLOT(deleteLater()));
  }
  return !newID.isEmpty();
}

// static
bool TellicoImporter::loadAllImages(const KURL& url_) {
  // only local files are allowed
  if(url_.isEmpty() || !url_.isValid() || !url_.isLocalFile()) {
//    myDebug() << "TellicoImporter::loadAllImages() - returning" << endl;
    return false;
  }

  // keep track of url for error reporting
  static KURL u;

  KZip zip(url_.path());
  if(!zip.open(IO_ReadOnly)) {
    if(u != url_) {
      Kernel::self()->sorry(i18n(errorImageLoad).arg(url_.fileName()));
    }
    u = url_;
    return false;
  }

  const KArchiveDirectory* dir = zip.directory();
  if(!dir) {
    if(u != url_) {
      Kernel::self()->sorry(i18n(errorImageLoad).arg(url_.fileName()));
    }
    u = url_;
    zip.close();
    return false;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QString::fromLatin1("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    zip.close();
    return false;
  }
  const QStringList images = static_cast<const KArchiveDirectory*>(imgDirEntry)->entries();
  for(QStringList::ConstIterator it = images.begin(); it != images.end(); ++it) {
    const KArchiveEntry* file = static_cast<const KArchiveDirectory*>(imgDirEntry)->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section('.', -1).upper(), (*it));
    }
  }
  zip.close();
  return true;
}

void TellicoImporter::addDefaultFilters() {
  switch(m_coll->type()) {
    case Data::Collection::Book:
      if(m_coll->hasField(QString::fromLatin1("read"))) {
        FilterPtr f = new Filter(Filter::MatchAny);
        f->setName(i18n("Unread Books"));
        f->append(new FilterRule(QString::fromLatin1("read"), QString::fromLatin1("true"), FilterRule::FuncNotContains));
        m_coll->addFilter(f);
        m_modified = true;
      }
      break;

    case Data::Collection::Video:
      if(m_coll->hasField(QString::fromLatin1("year"))) {
        FilterPtr f = new Filter(Filter::MatchAny);
        f->setName(i18n("Old Movies"));
        // old movies from before 1960
        f->append(new FilterRule(QString::fromLatin1("year"), QString::fromLatin1("19[012345]\\d"), FilterRule::FuncRegExp));
        m_coll->addFilter(f);
        m_modified = true;
      }
      if(m_coll->hasField(QString::fromLatin1("widescreen"))) {
        FilterPtr f = new Filter(Filter::MatchAny);
        f->setName(i18n("Widescreen"));
        f->append(new FilterRule(QString::fromLatin1("widescreen"), QString::fromLatin1("true"), FilterRule::FuncContains));
        m_coll->addFilter(f);
        m_modified = true;
      }
      break;

    case Data::Collection::Album:
      if(m_coll->hasField(QString::fromLatin1("year"))) {
        FilterPtr f = new Filter(Filter::MatchAny);
        f->setName(i18n("80's Music"));
        f->append(new FilterRule(QString::fromLatin1("year"), QString::fromLatin1("198\\d"),FilterRule::FuncRegExp));
        m_coll->addFilter(f);
        m_modified = true;
      }
      break;

    default:
      break;
  }
  if(m_coll->hasField(QString::fromLatin1("rating"))) {
    FilterPtr filter = new Filter(Filter::MatchAny);
    filter->setName(i18n("Favorites"));
    // check all the numbers, and use top 20% or so
    Data::FieldPtr field = m_coll->fieldByName(QString::fromLatin1("rating"));
    bool ok;
    uint min = Tellico::toUInt(field->property(QString::fromLatin1("minimum")), &ok);
    if(!ok) {
      min = 1;
    }
    uint max = Tellico::toUInt(field->property(QString::fromLatin1("maximum")), &ok);
    if(!ok) {
      min = 5;
    }
    for(uint i = QMAX(min, static_cast<uint>(0.8*(max-min+1))); i <= max; ++i) {
      filter->append(new FilterRule(QString::fromLatin1("rating"), QString::number(i), FilterRule::FuncContains));
    }
    if(!filter->isEmpty()) {
      m_coll->addFilter(filter);
      m_modified = true;
    }
  }
}

void TellicoImporter::slotCancel() {
  m_cancelled = true;
  m_format = Cancel;
}

#include "tellicoimporter.moc"
