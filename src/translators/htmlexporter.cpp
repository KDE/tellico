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

#include "htmlexporter.h"
#include "xslthandler.h"
#include "bookcasexmlexporter.h"
#include "../collection.h"
#include "../filehandler.h"
#include "../imagefactory.h"

#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kio/netaccess.h>
#include <kdeversion.h>
#include <kapplication.h>

#include <qdom.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qfile.h>
#include <qtextcodec.h>
#include <qhbox.h>
#include <qlabel.h>
//#include <qdir.h>
#include <qapp.h>

using Bookcase::Export::HTMLExporter;

HTMLExporter::HTMLExporter(const Data::Collection* coll_, Data::EntryList list_)
  : Bookcase::Export::TextExporter(coll_, list_),
    m_printHeaders(true),
    m_printGrouped(false),
    m_exportEntryFiles(false),
    m_widget(0),
    m_xsltfile(QString::fromLatin1("bookcase2html.xsl")),
    m_columns(QString::fromLatin1("title")) {
}

QString HTMLExporter::formatString() const {
  return i18n("HTML");
}

QString HTMLExporter::fileFilter() const {
  return i18n("*.html|HTML files (*.html)") + QString::fromLatin1("\n") + i18n("*|All files");
}

QWidget* HTMLExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Horizontal, i18n("HTML Options"), m_widget);
  l->addWidget(box);

  m_checkPrintHeaders = new QCheckBox(i18n("Print field headers"), box);
  QWhatsThis::add(m_checkPrintHeaders, i18n("If checked, the field names will be "
                                            "printed as table headers."));
  m_checkPrintHeaders->setChecked(m_printHeaders);

  m_checkPrintGrouped = new QCheckBox(i18n("Group the entries"), box);
  QWhatsThis::add(m_checkPrintGrouped, i18n("If checked, the entries will be grouped by "
                                            "the selected field."));
  m_checkPrintGrouped->setChecked(m_printGrouped);

  m_checkExportEntryFiles = new QCheckBox(i18n("Export individual entry files"), box);
  QWhatsThis::add(m_checkExportEntryFiles, i18n("If checked, individual files will be created for each entry."));
  m_checkExportEntryFiles->setChecked(m_exportEntryFiles);

  l->addStretch(1);
  return m_widget;
}

void HTMLExporter::readOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_printHeaders = config_->readBoolEntry("Print Field Headers", m_printHeaders);
  m_printGrouped = config_->readBoolEntry("Print Grouped", m_printGrouped);
  m_exportEntryFiles = config_->readBoolEntry("Export Entry Files", m_exportEntryFiles);

  config_->setGroup(QString::fromLatin1("Options - %1").arg(collection()->entryName()));
  m_entryXSLTFile = config_->readEntry("Entry Template", QString::fromLatin1("Default"));
  m_entryXSLTFile = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("entry-templates/")
                                                             + m_entryXSLTFile + QString::fromLatin1(".xsl"));
}

void HTMLExporter::saveOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_printHeaders = m_checkPrintHeaders->isChecked();
  config_->writeEntry("Print Field Headers", m_printHeaders);
  m_printGrouped = m_checkPrintGrouped->isChecked();
  config_->writeEntry("Print Grouped", m_printGrouped);
  m_exportEntryFiles = m_checkExportEntryFiles->isChecked();
  config_->writeEntry("Export Entry Files", m_exportEntryFiles);
}

QString HTMLExporter::text(bool formatFields_, bool encodeUTF8_) {
  QString xsltfile = KGlobal::dirs()->findResource("appdata", m_xsltfile);
  if(xsltfile.isNull()) {
    return QString::null;
  }

  KURL u;
  u.setPath(xsltfile);
  QDomDocument dom = FileHandler::readXMLFile(u);
  if(dom.isNull()) {
    return QString::null;
  }
  const Data::Collection* coll = collection();

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!encodeUTF8_) {
    QDomNodeList outNodes = dom.elementsByTagName(QString::fromLatin1("xsl:output"));
    const QString encoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());
    outNodes.item(0).toElement().setAttribute(QString::fromLatin1("encoding"), encoding);
  }

  if(m_printGrouped && !m_groupBy.isEmpty()) {
    // since the XSL stylesheet can't use a parameter as a key name, need to replace keys
    QDomNodeList keyNodes = dom.elementsByTagName(QString::fromLatin1("xsl:key"));
    for(unsigned i = 0; i < keyNodes.count(); ++i) {
      QDomElement elem = keyNodes.item(i).toElement();
      // dont't forget bc namespace
      // the entries key needs to use the groupName as the property
      if(elem.attribute(QString::fromLatin1("name"), QString::null) == QString::fromLatin1("entries")) {
        QString s;
        for(QStringList::ConstIterator it = m_groupBy.begin(); it != m_groupBy.end(); ++it) {
          s += QString::fromLatin1(".//bc:") + *it;
          if(coll->fieldByName(*it)->type() == Data::Field::Table2) {
            s += QString::fromLatin1("/bc:column[1]");
          }
          if(*it != m_groupBy.last()) {
            s += QString::fromLatin1("|");
          }
        }
        elem.setAttribute(QString::fromLatin1("use"), s);
      // the groups key needs to use the groupName as the match
      } else if(elem.attribute(QString::fromLatin1("name"), QString::null) == QString::fromLatin1("groups")) {
        QString s;
        for(QStringList::ConstIterator it = m_groupBy.begin(); it != m_groupBy.end(); ++it) {
          s += QString::fromLatin1("bc:") + *it;
          if(coll->fieldByName(*it)->type() == Data::Field::Table2) {
            s += QString::fromLatin1("/bc:column[1]");
          }
          if(*it != m_groupBy.last()) {
            s += QString::fromLatin1("|");
          }
        }
        elem.setAttribute(QString::fromLatin1("match"), s);
      }
    }
    QDomNodeList varNodes = dom.elementsByTagName(QString::fromLatin1("xsl:variable"));
    for(unsigned i = 0; i < varNodes.count(); ++i) {
      QDomElement elem = varNodes.item(i).toElement();
      if(elem.attribute(QString::fromLatin1("name"), QString::null) == QString::fromLatin1("all-groups")) {
        QString s;
        for(QStringList::ConstIterator it = m_groupBy.begin(); it != m_groupBy.end(); ++it) {
          s += QString::fromLatin1("//bc:") + *it;
          if(coll->fieldByName(*it)->type() == Data::Field::Table2) {
            s += QString::fromLatin1("/bc:column[1]");
          }
          if(*it != m_groupBy.last()) {
            s += QString::fromLatin1("|");
          }
        }
        elem.setAttribute(QString::fromLatin1("select"), s);
        break;
      }
    }
  }

//  kdDebug() << dom.toString() << endl;
  XSLTHandler handler(dom, QFile::encodeName(xsltfile));
  handler.addParam("show-headers", m_printHeaders ? "true()" : "false()");
  handler.addParam("group-entries", m_printGrouped ? "true()" : "false()");

  QStringList sortTitles;
  if(!m_sort1.isEmpty()) {
    sortTitles << m_sort1;
  }
  if(!m_sort2.isEmpty()) {
    sortTitles << m_sort2;
  }

  // the third sort column may be same as first
  if(!m_sort3.isEmpty() && sortTitles.findIndex(m_sort3) == -1) {
    sortTitles << m_sort3;
  }

  handler.addStringParam("sort-name1", coll->fieldNameByTitle(sortTitles[0]).utf8());
  if(sortTitles.count() > 1) {
    handler.addStringParam("sort-name2", coll->fieldNameByTitle(sortTitles[1]).utf8());
    if(sortTitles.count() > 2) {
      handler.addStringParam("sort-name3", coll->fieldNameByTitle(sortTitles[2]).utf8());
    }
  }

  QString sortString;
  if(m_printGrouped) {
    QString s;
    // if more than one, then it's the People pseudo-group
    if(m_groupBy.count() > 1) {
      s = i18n("People");
    } else {
      s = coll->fieldTitleByName(m_groupBy[0]);
    }
    sortString = i18n("(grouped by %1; sorted by %2)").arg(s).arg(sortTitles.join(QString::fromLatin1(", ")));
  } else {
    sortString = i18n("(sorted by %1)").arg(sortTitles.join(QString::fromLatin1(", ")));
  }

  handler.addStringParam("sort-title", encodeUTF8_ ? sortString.utf8() : sortString.local8Bit());

  QString pageTitle = QString::fromLatin1("Bookcase: ") + coll->title();
  pageTitle += QString::fromLatin1(" ") + sortString;
  handler.addStringParam("page-title", encodeUTF8_ ? pageTitle.utf8() : pageTitle.local8Bit());

  QStringList printFields;
  for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
    printFields << coll->fieldNameByTitle(*it);
  }
  handler.addStringParam("column-names",
                         encodeUTF8_ ? printFields.join(QString::fromLatin1(" ")).utf8()
                                     : printFields.join(QString::fromLatin1(" ")).local8Bit());

  BookcaseXMLExporter exporter(coll, entryList());
  exporter.includeID(true);
  QDomDocument output = exporter.exportXML(formatFields_, encodeUTF8_);

  // now handle exporting images.
  const KURL& targetURL = url();
  // if the target URL is empty, then the images should be saved to the default ImageFactory::TempDir()
  // otherwise save to a directory named by the collection title
  KURL imageDir;
  if(targetURL.isEmpty()) {
    imageDir.setPath(ImageFactory::tempDir());
    handler.addStringParam("imgdir", QFile::encodeName(imageDir.path()));
  } else {
    // use absolute path here for writing
    imageDir = targetURL;
    imageDir.cd(QString::fromLatin1(".."));
    QString dirname = targetURL.fileName().section('.', 0, 0);
    dirname += QString::fromLatin1("_images/");
    imageDir.addPath(dirname);
#if KDE_IS_VERSION(3,1,90)
    if(ImageFactory::hasImages() && !KIO::NetAccess::exists(imageDir, false, m_widget)) {
      KIO::NetAccess::mkdir(imageDir, m_widget);
    }
#else
    if(ImageFactory::hasImages() && !KIO::NetAccess::exists(imageDir, false)) {
      KIO::NetAccess::mkdir(imageDir);
    }
#endif
    // ok to use relative in HTML output
//    kdDebug() << "HTMLExporter::text() - relative image dir = " << imageDir.fileName()+'/' << endl;
    handler.addStringParam("imgdir", QFile::encodeName(imageDir.fileName()+'/'));
  }
//  kdDebug() << "HTMLExporter::text() - image dir = " << imageDir.path() << endl;

  if(m_imageWidth > 0 && m_imageHeight > 0) {
    handler.addParam("image-width", QCString().setNum(m_imageWidth));
    handler.addParam("image-height", QCString().setNum(m_imageHeight));
    kdDebug() << "HTMLExporter::text() - size = " << m_imageWidth << " x " << m_imageHeight << endl;
  }

  // keep track of which image fields to write, this is for field names
  QStringList imageFields;
  for(QStringList::ConstIterator f = printFields.begin(); f != printFields.end(); ++f) {
    if(coll->fieldByName(*f)->type() != Data::Field::Image) {
      continue;
    }
    imageFields << *f;
  }

  if(m_exportEntryFiles) {
    // add all image fields to string list
    for(Data::FieldListIterator fieldIt(collection()->imageFields()); fieldIt.current(); ++fieldIt) {
      const QString& fieldName = fieldIt.current()->name();
      if(imageFields.findIndex(fieldName) == -1) { // be sure not to have duplicate values
        imageFields << fieldName;
      }
    }
  }

  // now write all the images, keep an iterator around
  Data::EntryListIterator entryIt(entryList());

  kapp->setOverrideCursor(Qt::waitCursor);
  // call kapp->processEvents(), too
  int count = 0;
  const int processCount = 100; // process after every 100 events
  // use a dict for fast random access to keep track of which images were written to the file
  QDict<int> imageDict;
  for(QStringList::ConstIterator fieldName = imageFields.begin(); fieldName != imageFields.end(); ++fieldName) {
    for(entryIt.toFirst(); entryIt.current(); ++entryIt) {
      const QString& id = entryIt.current()->field(*fieldName);
      if(!id.isEmpty() && !imageDict[id]) {
        if(ImageFactory::writeImage(id, imageDir, true)) {
          imageDict.insert(id, reinterpret_cast<const int *>(1));
        } else {
          kdWarning() << "HTMLExporter::text() - unable to write temporary image file: "
                      << imageDir.path() << id << endl;
        }

        if(++count == processCount) {
          kapp->processEvents();
          // processEvents() seems to pop the override cursor???
          kapp->setOverrideCursor(Qt::waitCursor);
          count = 0;
        }
      }
    }
  }

  // now worry about actually exporting entry files
  if(m_exportEntryFiles) {
    KURL u;
    u.setPath(m_entryXSLTFile);
    QDomDocument dom = FileHandler::readXMLFile(u);
    handler.addParam("link-entries", "true()");
    // the stylesheet prints utf-8 by default, if using locale encoding, need
    // to change the encoding attribute on the xsl:output element
    if(!encodeUTF8_) {
      QDomNodeList outNodes = dom.elementsByTagName(QString::fromLatin1("xsl:output"));
      const QString encoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());
      outNodes.item(0).toElement().setAttribute(QString::fromLatin1("encoding"), encoding);
    }

    XSLTHandler entryHandler(dom, QFile::encodeName(m_entryXSLTFile));
    if(entryHandler.isValid()) {
      // add system colors to stylesheet
      const QColorGroup& cg = QApplication::palette().active();
      entryHandler.addStringParam("font", KGlobalSettings::generalFont().family().latin1());
      entryHandler.addStringParam("bgcolor", cg.base().name().latin1());
      entryHandler.addStringParam("fgcolor", cg.text().name().latin1());
      entryHandler.addStringParam("color1", cg.highlightedText().name().latin1());
      entryHandler.addStringParam("color2", cg.highlight().name().latin1());

      // ok to use relative in HTML output
      entryHandler.addStringParam("imgdir", QFile::encodeName(imageDir.fileName()+'/'));

      // I can't reliable encode a string as a URI, so I'm punting, and I'll just replace everything but
      // a-zA-Z0-9 with an underscore. This MUST match the filename template in bookcase2html.xsl
      // the id is used so uniqueness is guaranteed
      const QRegExp badChars(QString::fromLatin1("[^-a-zA-Z0-9]"));
      for(entryIt.toFirst(); entryIt.current(); ++entryIt) {
        QString file;
        if(formatFields_) {
          file = entryIt.current()->formattedField(QString::fromLatin1("title"));
        } else {
          file = entryIt.current()->field(QString::fromLatin1("title"));
        }
        file.replace(badChars, QString::fromLatin1("_"));
        file += QString::fromLatin1("-") + QString::number(entryIt.current()->id());
        file += QString::fromLatin1(".html");

        KURL url = targetURL;
        url.cd(QString::fromLatin1(".."));
        url.addPath(file);

        Data::EntryList list;
        list.append(entryIt.current());
        BookcaseXMLExporter exporter(collection(), list);
        // exportXML(bool formatValues, bool encodeUTF8);
        QDomDocument dom = exporter.exportXML(formatFields_, encodeUTF8_);
        FileHandler::writeTextURL(url, entryHandler.applyStylesheet(dom.toString(), encodeUTF8_), encodeUTF8_, true);

        if(++count == processCount) {
          kapp->processEvents();
          // processEvents() seems to pop the override cursor???
          kapp->setOverrideCursor(Qt::waitCursor);
          count = 0;
        }
      }
    }
  }

  kapp->restoreOverrideCursor();
  return handler.applyStylesheet(output.toString(), encodeUTF8_);
}
