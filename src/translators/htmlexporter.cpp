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
#include "tellicoxmlexporter.h"
#include "../collection.h"
#include "../filehandler.h"
#include "../imagefactory.h"
#include "../latin1literal.h"
#include "tellico_xml.h"

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
#include <qapplication.h>

using Tellico::Export::HTMLExporter;

HTMLExporter::HTMLExporter(const Data::Collection* coll_) : Tellico::Export::TextExporter(coll_),
    m_printHeaders(true),
    m_printGrouped(false),
    m_exportEntryFiles(false),
    m_imageWidth(0),
    m_imageHeight(0),
    m_widget(0),
    m_xsltfile(QString::fromLatin1("tellico2html.xsl")),
    m_columns(QString::fromLatin1("title")) {
}

QString HTMLExporter::formatString() const {
  return i18n("HTML");
}

QString HTMLExporter::fileFilter() const {
  return i18n("*.html|HTML files (*.html)") + QChar('\n') + i18n("*|All files");
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

  // notes about utf-8 encoding:
  // all params should be passed to XSLTHandler in utf8
  // input string to XSLTHandler should be in utf-8, EVEN IF DOM STRING SAYS OTHERWISE

  KURL u;
  u.setPath(xsltfile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements with the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLFile(u, false);
  if(dom.isNull()) {
    return QString::null;
  }
  const Data::Collection* coll = collection();

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!encodeUTF8_) {
    const QDomNodeList childs = dom.documentElement().childNodes();
    for(unsigned j = 0; j < childs.count(); ++j) {
      if(childs.item(j).isElement() && childs.item(j).nodeName() == Latin1Literal("xsl:output")) {
        QDomElement e = childs.item(j).toElement();
        const QString encoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());
        e.setAttribute(QString::fromLatin1("encoding"), encoding);
        break;
      }
    }
  }

  if(m_groupBy.isEmpty()) {
    m_printGrouped = false; // can't group if no groups exist
  }

  XSLTHandler handler(dom, QFile::encodeName(xsltfile));
//  XSLTHandler handler(u);
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

  if(sortTitles.count() > 0) {
    handler.addStringParam("sort-name1", coll->fieldNameByTitle(sortTitles[0]).utf8());
    if(sortTitles.count() > 1) {
      handler.addStringParam("sort-name2", coll->fieldNameByTitle(sortTitles[1]).utf8());
      if(sortTitles.count() > 2) {
        handler.addStringParam("sort-name3", coll->fieldNameByTitle(sortTitles[2]).utf8());
      }
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
    if(sortTitles.isEmpty()) {
      sortString = i18n("(grouped by %1)").arg(s);
    } else {
      sortString = i18n("(grouped by %1; sorted by %2)").arg(s).arg(sortTitles.join(QString::fromLatin1(", ")));
    }

    QString groupFields;
    for(QStringList::ConstIterator it = m_groupBy.begin(); it != m_groupBy.end(); ++it) {
      Data::Field* f = coll->fieldByName(*it);
      if(!f) {
        continue;
      }
      if(f->flags() & Data::Field::AllowMultiple) {
        groupFields += QString::fromLatin1("tc:") + *it + QString::fromLatin1("s/tc:") + *it;
      } else {
        groupFields += QString::fromLatin1("tc:") + *it;
      }
      if(f->type() == Data::Field::Table2) {
        groupFields += QString::fromLatin1("/tc:column[1]");
      }
      if(*it != m_groupBy.last()) {
        groupFields += '|';
      }
    }
//    kdDebug() << groupFields << endl;
    handler.addStringParam("group-fields", groupFields.utf8());
  } else {
    sortString = i18n("(sorted by %1)").arg(sortTitles.join(QString::fromLatin1(", ")));
  }

  handler.addStringParam("sort-title", sortString.utf8());

  QString pageTitle = QString::fromLatin1("Tellico: ") + coll->title();
  pageTitle += QChar(' ') + sortString;
  handler.addStringParam("page-title", pageTitle.utf8());

  QStringList printFields;
  for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
    printFields << coll->fieldNameByTitle(*it);
  }
  handler.addStringParam("column-names", printFields.join(QChar(' ')).utf8());

  TellicoXMLExporter exporter(coll);
  exporter.setEntryList(entryList());
  exporter.setIncludeGroups(m_printGrouped);
// yes, this should be in utf8, always
  QDomDocument output = exporter.exportXML(formatFields_, true);

  // where the file will be saved
  const KURL& targetURL = url();

  // now handle exporting images.
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
    if(m_exportEntryFiles) {
      QString entryDir = targetURL.fileName().section('.', 0, 0) + QString::fromLatin1("_entries/");
      handler.addStringParam("entrydir", entryDir.utf8());
    }
  }
//  kdDebug() << "HTMLExporter::text() - image dir = " << imageDir.path() << endl;

  if(m_imageWidth > 0 && m_imageHeight > 0) {
    handler.addParam("image-width", QCString().setNum(m_imageWidth));
    handler.addParam("image-height", QCString().setNum(m_imageHeight));
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

  if(m_exportEntryFiles) {
    KURL u;
    u.setPath(m_entryXSLTFile);
    // no namespace processing
    QDomDocument dom = FileHandler::readXMLFile(u, false);
    handler.addParam("link-entries", "true()");
    // the stylesheet prints utf-8 by default, if using locale encoding, need
    // to change the encoding attribute on the xsl:output element
    if(!encodeUTF8_) {
      const QDomNodeList childs = dom.documentElement().childNodes();
      for(unsigned j = 0; j < childs.count(); ++j) {
        if(childs.item(j).isElement() && childs.item(j).nodeName() == Latin1Literal("xsl:output")) {
          QDomElement e = childs.item(j).toElement();
          const QString encoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());
          e.setAttribute(QString::fromLatin1("encoding"), encoding);
          break;
        }
      }
    }

    XSLTHandler entryHandler(dom, QFile::encodeName(m_entryXSLTFile));
    if(entryHandler.isValid()) {
      // add system colors to stylesheet
      const QColorGroup& cg = QApplication::palette().active();
      entryHandler.addStringParam("font", KGlobalSettings::generalFont().family().utf8());
      entryHandler.addStringParam("bgcolor", cg.base().name().utf8());
      entryHandler.addStringParam("fgcolor", cg.text().name().utf8());
      entryHandler.addStringParam("color1", cg.highlightedText().name().utf8());
      entryHandler.addStringParam("color2", cg.highlight().name().utf8());

      // ok to use relative in HTML output
      // need to add "../" since the entry file is in a depper level
      entryHandler.addStringParam("imgdir", QFile::encodeName(QString::fromLatin1("../")+imageDir.fileName()+'/'));
      // relative to entry files, will be "../"
      QString s = QString::fromLatin1("../") + targetURL.fileName();
      entryHandler.addStringParam("collection-file", s.utf8());

      KURL entryDir = targetURL;
      entryDir.cd(QString::fromLatin1(".."));
      entryDir.addPath(targetURL.fileName().section('.', 0, 0) + QString::fromLatin1("_entries/"));
#if KDE_IS_VERSION(3,1,90)
      if(!KIO::NetAccess::exists(entryDir, false, m_widget)) {
        KIO::NetAccess::mkdir(entryDir, m_widget);
      }
#else
      if(!KIO::NetAccess::exists(entryDir, false)) {
        KIO::NetAccess::mkdir(entryDir);
      }
#endif

      // now worry about actually exporting entry files
      // I can't reliable encode a string as a URI, so I'm punting, and I'll just replace everything but
      // a-zA-Z0-9 with an underscore. This MUST match the filename template in tellico2html.xsl
      // the id is used so uniqueness is guaranteed
      const QRegExp badChars(QString::fromLatin1("[^-a-zA-Z0-9]"));
      for(entryIt.toFirst(); entryIt.current(); ++entryIt) {
        QString file;
        if(formatFields_) {
          file = entryIt.current()->formattedField(QString::fromLatin1("title"));
        } else {
          file = entryIt.current()->field(QString::fromLatin1("title"));
        }
        // but only use the first title if it has multiple
        if(collection()->fieldByName(QString::fromLatin1("title"))->flags() && Data::Field::AllowMultiple) {
          file = file.section(';', 0, 0);
        }
        file.replace(badChars, QString::fromLatin1("_"));
        file += QString::fromLatin1("-") + QString::number(entryIt.current()->id());
        file += QString::fromLatin1(".html");

        KURL url = entryDir;
        url.addPath(file);

        Data::EntryList list;
        list.append(entryIt.current());
        TellicoXMLExporter exporter(collection());
        exporter.setEntryList(list);
        // exportXML(bool formatValues, bool encodeUTF8);
        QDomDocument dom = exporter.exportXML(formatFields_, encodeUTF8_);
        FileHandler::writeTextURL(url, entryHandler.applyStylesheet(dom.toString()), encodeUTF8_, true);

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
  return handler.applyStylesheet(output.toString());
}
