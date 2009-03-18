/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
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
#include "../imageinfo.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_strings.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"
#include "../progressmanager.h"

#include <klocale.h>
#include <kcodecs.h>
#include <kzip.h>
#include <kapplication.h>

#include <QDomDocument>
#include <QBuffer>
#include <QFile>
#include <QTimer>

using Tellico::Import::TellicoImporter;

TellicoImporter::TellicoImporter(const KUrl& url_, bool loadAllImages_) : DataImporter(url_),
    m_loadAllImages(loadAllImages_), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(0), m_zip(0), m_imgDir(0) {
}

TellicoImporter::TellicoImporter(const QString& text_) : DataImporter(text_),
    m_loadAllImages(true), m_format(Unknown), m_modified(false),
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

  QByteArray s; // read first 5 characters
  if(source() == URL) {
    if(!fileRef().open()) {
      return Data::CollPtr();
    }
    QIODevice* f = fileRef().file();
    for(int i = 0; i < 5; ++i) {
      char c;
      s += f->getChar(&c);
    }
    f->reset();
  } else {
    if(data().size() < 5) {
      m_format = Error;
      return Data::CollPtr();
    }
    s = QByteArray(data(), 6);
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
    QString str = i18n(errorLoad, url().fileName()) + QLatin1Char('\n');
    str += i18n("There is an XML parsing error in line %1, column %2.", errorLine, errorColumn);
    str += QLatin1String("\n");
    str += i18n("The error message from Qt is:");
    str += QLatin1String("\n\t") + errorMsg;
    myDebug() << str << endl;
    setStatusMessage(str);
    m_format = Error;
    return;
  }

  QDomElement root = dom.documentElement();

  // the syntax version field name changed from "version" to "syntaxVersion" in version 3
  uint syntaxVersion;
  if(root.hasAttribute(QLatin1String("syntaxVersion"))) {
    syntaxVersion = root.attribute(QLatin1String("syntaxVersion")).toInt();
  } else if (root.hasAttribute(QLatin1String("version"))) {
    syntaxVersion = root.attribute(QLatin1String("version")).toInt();
  } else {
    if(!url().isEmpty()) {
      setStatusMessage(i18n(errorLoad, url().fileName()));
    }
    m_format = Error;
    return;
  }
//  myDebug() << "TellicoImporter::loadXMLData() - syntaxVersion = " << syntaxVersion << endl;

  if((syntaxVersion > 6 && root.tagName() != QLatin1String("tellico"))
     || (syntaxVersion < 7 && root.tagName() != QLatin1String("bookcase"))) {
    if(!url().isEmpty()) {
      setStatusMessage(i18n(errorLoad, url().fileName()));
    }
    m_format = Error;
    return;
  }

  if(syntaxVersion > XML::syntaxVersion) {
    if(!url().isEmpty()) {
      QString str = i18n(errorLoad, url().fileName()) + QLatin1Char('\n');
      str += i18n("It is from a future version of Tellico.");
      myDebug() << str << endl;
      setStatusMessage(str);
    } else {
      myDebug() << "Unable to load collection, from a future version (" << syntaxVersion << ")" << endl;
    }
    m_format = Error;
    return;
  } else if(XML::versionConversion(syntaxVersion, XML::syntaxVersion)) {
    // going from version 9 to 10, there's no conversion needed
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
    if(n.isElement() && n.localName() == QLatin1String("collection")) {
      collelem = n.toElement();
      break;
    }
  }
  if(collelem.isNull()) {
    kWarning() << "TellicoImporter::loadDomDocument() - No collection item found.";
    return;
  }

  QString title = collelem.attribute(QLatin1String("title"));

  // be careful not to have element name collision
  // for fields, each true field element is a child of a fields element
  QDomNodeList fieldelems;
  for(QDomNode n = collelem.firstChild(); !n.isNull(); n = n.nextSibling()) {
    if(n.namespaceURI() != m_namespace) {
      continue;
    }
    // QLatin1String is a macro, so can't say QLatin1String(syntaxVersion > 3 ? "fields" : "attributes")
    if((syntaxVersion > 3 && n.localName() == QLatin1String("fields"))
       || (syntaxVersion < 4 && n.localName() == QLatin1String("attributes"))) {
      QDomElement e = n.toElement();
      fieldelems = e.elementsByTagNameNS(m_namespace, (syntaxVersion > 3) ? QLatin1String("field")
                                                                          : QLatin1String("attribute"));
      break;
    }
  }
//  myDebug() << "TellicoImporter::loadXMLData() - " << fieldelems.count() << " field(s)" << endl;

  // the dilemma is when to force the new collection to have all the default attributes
  // if there are no attributes or if the first one has the special name of _default
  bool addFields = (fieldelems.count() == 0);
  if(!addFields) {
    QString name = fieldelems.item(0).toElement().attribute(QLatin1String("name"));
    addFields = (name == QLatin1String("_default"));
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
    entryName = QLatin1String("entry");
    QString typeStr = collelem.attribute(QLatin1String("type"));
    Data::Collection::Type type = static_cast<Data::Collection::Type>(typeStr.toInt());
    m_coll = CollectionFactory::collection(type, addFields);
  } else {
    entryName = collelem.attribute(QLatin1String("unit"));
    m_coll = CollectionFactory::collection(entryName, addFields);
  }

  if(!title.isEmpty()) {
    m_coll->setTitle(title);
  }

  for(int j = 0; j < fieldelems.count(); ++j) {
    readField(syntaxVersion, fieldelems.item(j).toElement());
  }

  if(m_coll->type() == Data::Collection::Bibtex) {
    Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(m_coll.data());
    QDomNodeList macroelems;
    for(QDomNode n = collelem.firstChild(); !n.isNull(); n = n.nextSibling()) {
      if(n.namespaceURI() != m_namespace) {
        continue;
      }
      if(n.localName() == QLatin1String("macros")) {
        macroelems = n.toElement().elementsByTagNameNS(m_namespace, QLatin1String("macro"));
        break;
      }
    }
//    myDebug() << "TellicoImporter::loadXMLData() - found " << macroelems.count() << " macros" << endl;
    for(int j = 0; c && j < macroelems.count(); ++j) {
      QDomElement elem = macroelems.item(j).toElement();
      c->addMacro(elem.attribute(QLatin1String("name")), elem.text());
    }

    for(QDomNode n = collelem.firstChild(); !n.isNull(); n = n.nextSibling()) {
      if(n.namespaceURI() != m_namespace) {
        continue;
      }
      if(n.localName() == QLatin1String("bibtex-preamble")) {
        c->setPreamble(n.toElement().text());
        break;
      }
    }
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
    return;
  }

//  as a special case, for old book collections with a bibtex-id field, convert to Bibtex
  if(syntaxVersion < 4 && m_coll->type() == Data::Collection::Book
     && m_coll->hasField(QLatin1String("bibtex-id"))) {
    m_coll = Data::BibtexCollection::convertBookCollection(m_coll);
  }

  const uint count = collelem.childNodes().count();
  const uint stepSize = qMax(s_stepSize, count/100);
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
    if(n.localName() == QLatin1String("images")) {
      imgelems = n.toElement().elementsByTagNameNS(m_namespace, QLatin1String("image"));
      break;
    }
  }
  for(int j = 0; j < imgelems.count(); ++j) {
    readImage(imgelems.item(j).toElement(), loadImages_);
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
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
    m_coll = Data::CollPtr();
    return;
  }

  // filters and borrowers are at document root level, not collection
  for(QDomNode n = root.firstChild(); !n.isNull() && !m_cancelled; n = n.nextSibling()) {
    if(n.namespaceURI() != m_namespace) {
      continue;
    }
    if(n.localName() == QLatin1String("borrowers")) {
      QDomNodeList borrowerElems = n.toElement().elementsByTagNameNS(m_namespace, QLatin1String("borrower"));
      for(int j = 0; j < borrowerElems.count(); ++j) {
        readBorrower(borrowerElems.item(j).toElement());
      }
    } else if(n.localName() == QLatin1String("filters")) {
      QDomNodeList filterElems = n.toElement().elementsByTagNameNS(m_namespace, QLatin1String("filter"));
      for(int j = 0; j < filterElems.count(); ++j) {
        readFilter(filterElems.item(j).toElement());
      }
    }
  }

  // special for user, if using an older document format, add some nice new filters
  if(syntaxVersion < 8) {
    addDefaultFilters();
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
  }
}

void TellicoImporter::readField(uint syntaxVersion_, const QDomElement& elem_) {
  // special case: if the i18n attribute equals true, then translate the title, description, and category
  bool isI18n = elem_.attribute(QLatin1String("i18n")) == QLatin1String("true");

  QString name  = elem_.attribute(QLatin1String("name"), QLatin1String("unknown"));
  QString title = elem_.attribute(QLatin1String("title"), i18n("Unknown"));
  if(isI18n) {
    title = i18n(title.toUtf8());
  }

  QString typeStr = elem_.attribute(QLatin1String("type"), QString::number(Data::Field::Line));
  Data::Field::Type type = static_cast<Data::Field::Type>(typeStr.toInt());

  Data::FieldPtr field;
  if(type == Data::Field::Choice) {
    QStringList allowed = elem_.attribute(QLatin1String("allowed")).split(QRegExp(QLatin1String("\\s*;\\s*")));
    if(isI18n) {
      for(QStringList::Iterator it = allowed.begin(); it != allowed.end(); ++it) {
        (*it) = i18n((*it).toUtf8());
      }
    }
    field = new Data::Field(name, title, allowed);
  } else {
    field = new Data::Field(name, title, type);
  }

  if(elem_.hasAttribute(QLatin1String("category"))) {
    // at one point, the categories had keyboard accels
    QString cat = elem_.attribute(QLatin1String("category"));
    if(syntaxVersion_ < 9 && cat.indexOf(QLatin1Char('&')) > -1) {
      cat.remove(QLatin1Char('&'));
    }
    if(isI18n) {
      cat = i18n(cat.toUtf8());
    }
    field->setCategory(cat);
  }

  if(elem_.hasAttribute(QLatin1String("flags"))) {
    int flags = elem_.attribute(QLatin1String("flags")).toInt();
    // I also changed the enum values for syntax 3, but the only custom field
    // would have been bibtex-id
    if(syntaxVersion_ < 3 && field->name() == QLatin1String("bibtex-id")) {
      flags = 0;
    }

    // in syntax version 4, added a flag to disallow deleting attributes
    // if it's a version before that and is the title, then add the flag
    if(syntaxVersion_ < 4 && field->name() == QLatin1String("title")) {
      flags |= Data::Field::NoDelete;
    }
    field->setFlags(flags);
  }

  QString formatStr = elem_.attribute(QLatin1String("format"), QString::number(Data::Field::FormatNone));
  Data::Field::FormatFlag format = static_cast<Data::Field::FormatFlag>(formatStr.toInt());
  field->setFormatFlag(format);

  if(elem_.hasAttribute(QLatin1String("description"))) {
    QString desc = elem_.attribute(QLatin1String("description"));
    if(isI18n) {
      desc = i18n(desc.toUtf8());
    }
    field->setDescription(desc);
  }

  if(syntaxVersion_ >= 5) {
    QDomNodeList props = elem_.elementsByTagNameNS(m_namespace, QLatin1String("prop"));
    for(int i = 0; i < props.count(); ++i) {
      QDomElement e = props.item(i).toElement();
      field->setProperty(e.attribute(QLatin1String("name")), e.text());
    }
    // all track fields in music collections prior to version 9 get converted to three columns
    if(syntaxVersion_ < 9) {
      if(m_coll->type() == Data::Collection::Album && field->name() == QLatin1String("track")) {
        field->setProperty(QLatin1String("columns"), QLatin1String("3"));
        field->setProperty(QLatin1String("column1"), i18n("Title"));
        field->setProperty(QLatin1String("column2"), i18n("Artist"));
        field->setProperty(QLatin1String("column3"), i18n("Length"));
      } else if(m_coll->type() == Data::Collection::Video && field->name() == QLatin1String("cast")) {
        field->setProperty(QLatin1String("column1"), i18n("Actor/Actress"));
        field->setProperty(QLatin1String("column2"), i18n("Role"));
      }
    }
  } else if(elem_.hasAttribute(QLatin1String("bibtex-field"))) {
    field->setProperty(QLatin1String("bibtex"), elem_.attribute(QLatin1String("bibtex-field")));
  }

  // Table2 is deprecated
  if(field->type() == Data::Field::Table2) {
    field->setType(Data::Field::Table);
    field->setProperty(QLatin1String("columns"), QLatin1String("2"));
  }
  // for syntax 8, rating fields got their own type
  if(syntaxVersion_ < 8) {
    Data::Field::convertOldRating(field); // does all its own checking
  }
  m_coll->addField(field);
//  myDebug() << QString("  Added field: %1, %2", field->name(), field->title()) << endl;
}

void TellicoImporter::readEntry(uint syntaxVersion_, const QDomElement& entryElem_) {
  const int id = entryElem_.attribute(QLatin1String("id")).toInt();
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

    bool isI18n = elem.attribute(QLatin1String("i18n")) == QLatin1String("true");

    // Entry::setField checks to see if an field of 'name' is allowed
    // in version 3 and prior, checkbox attributes had no text(), set it to "true" now
    if(syntaxVersion_ < 4 && elem.text().isEmpty()) {
      // "true" means checked
      entry->setField(elem.localName(), QLatin1String("true"));
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
          QDomNode yNode = elem.elementsByTagNameNS(m_namespace, QLatin1String("year")).item(0);
          if(!yNode.isNull()) {
            value += yNode.toElement().text();
          }
          value += QLatin1Char('-');
          QDomNode mNode = elem.elementsByTagNameNS(m_namespace, QLatin1String("month")).item(0);
          if(!mNode.isNull()) {
            value += mNode.toElement().text();
          }
          value += QLatin1Char('-');
          QDomNode dNode = elem.elementsByTagNameNS(m_namespace, QLatin1String("day")).item(0);
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
        value = value.trimmed();
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
          KUrl u(value);
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
      } else if(syntaxVersion_ < 2 && name == QLatin1String("keywords")) {
        // in version 2, "keywords" changed to "keyword"
        name = QLatin1String("keyword");
      }
      // special case: if the i18n attribute equals true, then translate the title, description, and category
      if(isI18n) {
        entry->setField(name, i18n(value.toUtf8()));
      } else {
        // special case for isbn fields, go ahead and validate
        if(name == QLatin1String("isbn")) {
          const ISBNValidator val(0);
          if(elem.attribute(QLatin1String("validate")) != QLatin1String("no")) {
            val.fixup(value);
          }
        }
        entry->setField(name, value);
      }
    } else { // if no field by the tag name, then it has children, iterate through them
      // the field name has the final QLatin1Char('s'), so remove it
      name.truncate(name.length() - 1);
      f = m_coll->fieldByName(name);

      // if it's a derived value, no field value is added
      if(!f || f->type() == Data::Field::Dependent) {
        continue;
      }

      const bool oldTracks = (oldMusic && name == QLatin1String("track"));

      QStringList values;
      // concatenate values
      for(QDomNode childNode = node.firstChild(); !childNode.isNull(); childNode = childNode.nextSibling()) {
        QString value;
        // don't worry about i18n here, Tables are never translated
        QDomNodeList cols = childNode.toElement().elementsByTagNameNS(m_namespace, QLatin1String("column"));
        if(cols.count() > 0) {
          for(int i = 0; i < cols.count(); ++i) {
            // special case for old tracks
            if(oldTracks && i == 1) {
              // if the second column holds the track length, bump it to next column
              QRegExp rx(QLatin1String("\\d+:\\d\\d"));
              if(rx.exactMatch(cols.item(i).toElement().text())) {
                value += entry->field(QLatin1String("artist"));
                value += QLatin1String("::");
              }
            }
            value += cols.item(i).toElement().text().trimmed();
            if(i < cols.count()-1) {
              value += QLatin1String("::");
            } else if(oldTracks && cols.count() == 1) {
              value += QLatin1String("::");
              value += entry->field(QLatin1String("artist"));
            }
          }
          values += value;
        } else {
          // really loose here, we don't even check that the element name
          // is what we think it is
          QString s = childNode.toElement().text().trimmed();
          if(isI18n && !s.isEmpty()) {
            value += i18n(s.toUtf8());
          } else {
            value += s;
          }
          if(oldTracks) {
            value += QLatin1String("::");
            value += entry->field(QLatin1String("artist"));
          }
          if(values.indexOf(value) == -1) {
            values += value;
          }
        }
      }
      entry->setField(name, values.join(QLatin1String("; ")));
    }
  } // end field value loop

  m_coll->addEntries(entry);
}

void TellicoImporter::readImage(const QDomElement& elem_, bool loadImage_) {
  QString format = elem_.attribute(QLatin1String("format"));
  const bool link = elem_.attribute(QLatin1String("link")) == QLatin1String("true");
  // idClean() already calls shareString()
  QString id = link ? shareString(elem_.attribute(QLatin1String("id")))
                    : Data::Image::idClean(elem_.attribute(QLatin1String("id")));

  bool readInfo = true;
  if(loadImage_) {
    QByteArray ba;
    KCodecs::base64Decode(QByteArray(elem_.text().toLatin1()), ba);
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
    int width = elem_.attribute(QLatin1String("width")).toInt();
    int height = elem_.attribute(QLatin1String("height")).toInt();
    Data::ImageInfo info(id, format.toLatin1(), width, height, link);
    ImageFactory::cacheImageInfo(info);
  }
}

void TellicoImporter::readFilter(const QDomElement& elem_) {
  FilterPtr f(new Filter(Filter::MatchAny));
  f->setName(elem_.attribute(QLatin1String("name")));

  QString match = elem_.attribute(QLatin1String("match"));
  if(match == QLatin1String("all")) {
    f->setMatch(Filter::MatchAll);
  }

  QDomNodeList rules = elem_.elementsByTagNameNS(m_namespace, QLatin1String("rule"));
  for(int i = 0; i < rules.count(); ++i) {
    QDomElement e = rules.item(i).toElement();
    if(e.isNull()) {
      continue;
    }

    QString field = e.attribute(QLatin1String("field"));
    // empty field means match any of them
    QString pattern = e.attribute(QLatin1String("pattern"));
    // empty pattern is bad
    if(pattern.isEmpty()) {
      kWarning() << "TellicoImporter::readFilter() - empty rule!";
      continue;
    }
    QString function = e.attribute(QLatin1String("function")).toLower();
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
      kWarning() << "TellicoImporter::readFilter() - invalid rule function: " << function;
      continue;
    }
    f->append(new FilterRule(field, pattern, func));
  }

  if(!f->isEmpty()) {
    m_coll->addFilter(f);
  }
}

void TellicoImporter::readBorrower(const QDomElement& elem_) {
  QString name = elem_.attribute(QLatin1String("name"));
  QString uid = elem_.attribute(QLatin1String("uid"));
  Data::BorrowerPtr b(new Data::Borrower(name, uid));

  QDomNodeList loans = elem_.elementsByTagNameNS(m_namespace, QLatin1String("loan"));
  for(int i = 0; i < loans.count(); ++i) {
    QDomElement e = loans.item(i).toElement();
    if(e.isNull()) {
      continue;
    }
    long id = e.attribute(QLatin1String("entryRef")).toLong();
    Data::EntryPtr entry = m_coll->entryById(id);
    if(!entry) {
      myDebug() << "TellicoImporter::readBorrower() - no entry with id = " << id << endl;
      continue;
    }
    QString uid = e.attribute(QLatin1String("uid"));
    QDate loanDate, dueDate;
    QString s = e.attribute(QLatin1String("loanDate"));
    if(!s.isEmpty()) {
      loanDate = QDate::fromString(s, Qt::ISODate);
    }
    s = e.attribute(QLatin1String("dueDate"));
    if(!s.isEmpty()) {
      dueDate = QDate::fromString(s, Qt::ISODate);
    }
    Data::LoanPtr loan(new Data::Loan(entry, loanDate, dueDate, e.text()));
    loan->setUID(uid);
    b->addLoan(loan);
    s = e.attribute(QLatin1String("calendar"));
    loan->setInCalendar(s == QLatin1String("true"));
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
    QByteArray allData = data();
    m_buffer = new QBuffer(&allData);
    m_zip = new KZip(m_buffer);
  }
  if(!m_zip->open(QIODevice::ReadOnly)) {
    setStatusMessage(i18n(errorLoad, url().fileName()));
    m_format = Error;
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const KArchiveDirectory* dir = m_zip->directory();
  if(!dir) {
    QString str = i18n(errorLoad, url().fileName()) + QLatin1Char('\n');
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
  const KArchiveEntry* entry = dir->entry(QLatin1String("tellico.xml"));
  if(!entry) {
    entry = dir->entry(QLatin1String("bookcase.xml"));
  }
  if(!entry || !entry->isFile()) {
    QString str = i18n(errorLoad, url().fileName()) + QLatin1Char('\n');
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

  const KArchiveEntry* imgDirEntry = dir->entry(QLatin1String("images"));
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
  const uint stepSize = qMax(s_stepSize, static_cast<uint>(images.count())/100);

  uint j = 0;
  for(QStringList::ConstIterator it = images.begin(); !m_cancelled && it != images.end(); ++it, ++j) {
    const KArchiveEntry* file = m_imgDir->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section(QLatin1Char('.'), -1).toUpper(), (*it));
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
                                         id_.section(QLatin1Char('.'), -1).toUpper(), id_);
  m_images.remove(id_);
  if(m_images.isEmpty()) {
    // give it some time
    QTimer::singleShot(3000, this, SLOT(deleteLater()));
  }
  return !newID.isEmpty();
}

// static
bool TellicoImporter::loadAllImages(const KUrl& url_) {
  // only local files are allowed
  if(url_.isEmpty() || !url_.isValid() || !url_.isLocalFile()) {
//    myDebug() << "TellicoImporter::loadAllImages() - returning" << endl;
    return false;
  }

  // keep track of url for error reporting
  static KUrl u;

  KZip zip(url_.path());
  if(!zip.open(QIODevice::ReadOnly)) {
    if(u != url_) {
      Kernel::self()->sorry(i18n(errorImageLoad, url_.fileName()));
    }
    u = url_;
    return false;
  }

  const KArchiveDirectory* dir = zip.directory();
  if(!dir) {
    if(u != url_) {
      Kernel::self()->sorry(i18n(errorImageLoad, url_.fileName()));
    }
    u = url_;
    zip.close();
    return false;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QLatin1String("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    zip.close();
    return false;
  }
  const QStringList images = static_cast<const KArchiveDirectory*>(imgDirEntry)->entries();
  for(QStringList::ConstIterator it = images.begin(); it != images.end(); ++it) {
    const KArchiveEntry* file = static_cast<const KArchiveDirectory*>(imgDirEntry)->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section(QLatin1Char('.'), -1).toUpper(), (*it));
    }
  }
  zip.close();
  return true;
}

void TellicoImporter::addDefaultFilters() {
  switch(m_coll->type()) {
    case Data::Collection::Book:
      if(m_coll->hasField(QLatin1String("read"))) {
        FilterPtr f(new Filter(Filter::MatchAny));
        f->setName(i18n("Unread Books"));
        f->append(new FilterRule(QLatin1String("read"), QLatin1String("true"), FilterRule::FuncNotContains));
        m_coll->addFilter(f);
        m_modified = true;
      }
      break;

    case Data::Collection::Video:
      if(m_coll->hasField(QLatin1String("year"))) {
        FilterPtr f(new Filter(Filter::MatchAny));
        f->setName(i18n("Old Movies"));
        // old movies from before 1960
        f->append(new FilterRule(QLatin1String("year"), QLatin1String("19[012345]\\d"), FilterRule::FuncRegExp));
        m_coll->addFilter(f);
        m_modified = true;
      }
      if(m_coll->hasField(QLatin1String("widescreen"))) {
        FilterPtr f(new Filter(Filter::MatchAny));
        f->setName(i18n("Widescreen"));
        f->append(new FilterRule(QLatin1String("widescreen"), QLatin1String("true"), FilterRule::FuncContains));
        m_coll->addFilter(f);
        m_modified = true;
      }
      break;

    case Data::Collection::Album:
      if(m_coll->hasField(QLatin1String("year"))) {
        FilterPtr f(new Filter(Filter::MatchAny));
        f->setName(i18n("80's Music"));
        f->append(new FilterRule(QLatin1String("year"), QLatin1String("198\\d"),FilterRule::FuncRegExp));
        m_coll->addFilter(f);
        m_modified = true;
      }
      break;

    default:
      break;
  }
  if(m_coll->hasField(QLatin1String("rating"))) {
    FilterPtr filter(new Filter(Filter::MatchAny));
    filter->setName(i18n("Favorites"));
    // check all the numbers, and use top 20% or so
    Data::FieldPtr field = m_coll->fieldByName(QLatin1String("rating"));
    bool ok;
    uint min = Tellico::toUInt(field->property(QLatin1String("minimum")), &ok);
    if(!ok) {
      min = 1;
    }
    uint max = Tellico::toUInt(field->property(QLatin1String("maximum")), &ok);
    if(!ok) {
      min = 5;
    }
    for(uint i = qMax(min, static_cast<uint>(0.8*(max-min+1))); i <= max; ++i) {
      filter->append(new FilterRule(QLatin1String("rating"), QString::number(i), FilterRule::FuncContains));
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
