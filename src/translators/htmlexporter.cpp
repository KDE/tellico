/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
#include "tellico_xml.h"
#include "../document.h"
#include "../collection.h"
#include "../filehandler.h"
#include "../imagefactory.h"
#include "../latin1literal.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"

#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kio/netaccess.h>
#include <kdeversion.h>
#include <kapplication.h>
#include <khtml_part.h>
#include <dom/dom_doc.h>
#include <dom/html_element.h>

#include <qdom.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qfile.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qapplication.h>

using Tellico::Export::HTMLExporter;

HTMLExporter::HTMLExporter() : Tellico::Export::Exporter(),
    m_part(0),
    m_handler(0),
    m_printHeaders(true),
    m_printGrouped(false),
    m_exportEntryFiles(false),
    m_imageWidth(0),
    m_imageHeight(0),
    m_widget(0),
    m_xsltFile(QString::fromLatin1("tellico2html.xsl")) {
}

HTMLExporter::HTMLExporter(const Data::Collection* coll_) : Tellico::Export::Exporter(coll_),
    m_part(0),
    m_handler(0),
    m_printHeaders(true),
    m_printGrouped(false),
    m_exportEntryFiles(false),
    m_imageWidth(0),
    m_imageHeight(0),
    m_widget(0),
    m_xsltFile(QString::fromLatin1("tellico2html.xsl")) {
}

HTMLExporter::~HTMLExporter() {
  delete m_part;
  m_part = 0;
  delete m_handler;
  m_handler = 0;
}

QString HTMLExporter::formatString() const {
  return i18n("HTML");
}

QString HTMLExporter::fileFilter() const {
  return i18n("*.html|HTML Files (*.html)") + QChar('\n') + i18n("*|All Files");
}

void HTMLExporter::reset() {
  // since the ExportUTF8 option may have changed, need to delete handler
  delete m_handler;
  m_handler = 0;
  m_files.clear();
  m_links.clear();
  m_copiedFiles.clear();
}

bool HTMLExporter::exec() {
  if(url().isEmpty() || !url().isValid()) {
    return false;
  }

  // this is expensive, but necessary to easily crawl the DOM and fetch external images
  if(!m_part) {
    m_part = new KHTMLPart((QWidget*)0);
    m_part->setJScriptEnabled(false);
    m_part->setJavaEnabled(false);
    m_part->setMetaRefreshEnabled(false);
    m_part->setPluginsEnabled(false);
  }

  m_part->begin();
  m_part->write(text());
  m_part->end();

  DOM::HTMLElement e = m_part->document().documentElement();
  // a little paranoia, the XSL transform might fail
  if(e.isNull() || !e.hasChildNodes()) {
    return false;
  }
  parseNode(e);

  bool success = FileHandler::writeTextURL(url(),
                                           QString::fromLatin1("<html>")
                                             + e.innerHTML().string()
                                             + QString::fromLatin1("</html>"),
                                           options() & Export::ExportUTF8,
                                           options() & Export::ExportForce);
  return success && copyFiles() && (!m_exportEntryFiles || writeEntryFiles());
}

QString HTMLExporter::text() {
  QString xsltfile = locate("appdata", m_xsltFile);
  if(xsltfile.isNull()) {
    kdDebug() << "HTMLExporter::text() - no xslt file for " << m_xsltFile << endl;
    return QString::null;
  }

  KURL u;
  u.setPath(xsltfile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements in the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLFile(u, false);
  if(dom.isNull()) {
    kdDebug() << "HTMLExporter::text() - error loading xslt file: " << xsltfile << endl;
    return QString::null;
  }

  // notes about utf-8 encoding:
  // all params should be passed to XSLTHandler in utf8
  // input string to XSLTHandler should be in utf-8, EVEN IF DOM STRING SAYS OTHERWISE

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!(options() & Export::ExportUTF8)) {
    XSLTHandler::setLocaleEncoding(dom);
  }

  const Data::Collection* coll = collection();
  if(!coll) {
    myDebug() << "HTMLExporter::text() - no collection pointer!" << endl;
    return QString::null;
  }

  if(m_groupBy.isEmpty()) {
    m_printGrouped = false; // can't group if no groups exist
  }

  if(!m_handler) {
    m_handler = new XSLTHandler(dom, QFile::encodeName(xsltfile));
    if(!m_handler->isValid()) {
      delete m_handler;
      m_handler = 0;
      kdWarning() << "HTMLExporter::text() - error loading xslt file: " << xsltfile << endl;
      return QString::null;
    }
    setFormattingOptions(coll);

    if(m_exportEntryFiles) {
      // export entries to same place as all the other date files
      m_handler->addStringParam("entrydir", QFile::encodeName(fileDir().fileName())+ '/');
      // be sure to link all the entries
      m_handler->addParam("link-entries", "true()");
    }

    if(!m_collectionURL.isEmpty()) {
      QString s = QString::fromLatin1("../") + m_collectionURL.fileName();
      m_handler->addStringParam("collection-file", s.utf8());
    }

    // look for a file that gets installed to know the installation directory
    QString dataDir = KGlobal::dirs()->findResourceDir("appdata", QString::fromLatin1("pics/tellico.png"));
    m_handler->addStringParam("datadir", QFile::encodeName(dataDir));
  }

  GUI::CursorSaver cs(Qt::waitCursor);

  writeImages(coll);

  // now grab the XML
  TellicoXMLExporter exporter(coll);
  exporter.setURL(url());
  exporter.setEntries(entries());
  exporter.setIncludeGroups(m_printGrouped);
// yes, this should be in utf8, always
  exporter.setOptions(options() | Export::ExportUTF8);
  QDomDocument output = exporter.exportXML();
#if 0
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(IO_WriteOnly)) {
    QTextStream t(&f);
    t << output.toString();
  }
  f.close();
#endif

  QString text = m_handler->applyStylesheet(output.toString());
#if 0
  QFile f2(QString::fromLatin1("/tmp/test.html"));
  if(f2.open(IO_WriteOnly)) {
    QTextStream t(&f2);
    t << text;
    t << "\n\n-------------------------------------------------------\n\n";
    t << Tellico::i18nReplace(text);
  }
  f2.close();
#endif
  return Tellico::i18nReplace(text);
}

void HTMLExporter::setFormattingOptions(const Data::Collection* coll) {
  m_handler->addParam("show-headers", m_printHeaders ? "true()" : "false()");
  m_handler->addParam("group-entries", m_printGrouped ? "true()" : "false()");

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
    m_handler->addStringParam("sort-name1", coll->fieldNameByTitle(sortTitles[0]).utf8());
    if(sortTitles.count() > 1) {
      m_handler->addStringParam("sort-name2", coll->fieldNameByTitle(sortTitles[1]).utf8());
      if(sortTitles.count() > 2) {
        m_handler->addStringParam("sort-name3", coll->fieldNameByTitle(sortTitles[2]).utf8());
      }
    }
  }

  // no longer showing "sorted by..." since the column headers are clickable
  // but still use "grouped by"
  QString sortString;
  if(m_printGrouped) {
    QString s;
    // if more than one, then it's the People pseudo-group
    if(m_groupBy.count() > 1) {
      s = i18n("People");
    } else {
      s = coll->fieldTitleByName(m_groupBy[0]);
    }
    sortString = i18n("(grouped by %1)").arg(s);

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
      int ncols = 0;
      if(f->type() == Data::Field::Table) {
        bool ok;
        ncols = Tellico::toUInt(f->property(QString::fromLatin1("columns")), &ok);
        if(!ok) {
          ncols = 1;
        }
      }
      if(ncols > 1) {
        groupFields += QString::fromLatin1("/tc:column[1]");
      }
      if(*it != m_groupBy.last()) {
        groupFields += '|';
      }
    }
//    kdDebug() << groupFields << endl;
    m_handler->addStringParam("group-fields", groupFields.utf8());
    m_handler->addStringParam("sort-title", sortString.utf8());
  }

  QString pageTitle = QString::fromLatin1("Tellico: ") + coll->title();
  pageTitle += QChar(' ') + sortString;
  m_handler->addStringParam("page-title", pageTitle.utf8());

  QStringList showFields;
  for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
    showFields << coll->fieldNameByTitle(*it);
  }
  m_handler->addStringParam("column-names", showFields.join(QChar(' ')).utf8());

  if(m_imageWidth > 0 && m_imageHeight > 0) {
    m_handler->addParam("image-width", QCString().setNum(m_imageWidth));
    m_handler->addParam("image-height", QCString().setNum(m_imageHeight));
  }

  // add system colors to stylesheet
  const QColorGroup& cg = QApplication::palette().active();
  m_handler->addStringParam("font",    KGlobalSettings::generalFont().family().utf8());
  m_handler->addStringParam("bgcolor", cg.base().name().utf8());
  m_handler->addStringParam("fgcolor", cg.text().name().utf8());
  m_handler->addStringParam("color1",  cg.highlightedText().name().utf8());
  m_handler->addStringParam("color2",  cg.highlight().name().utf8());
}

void HTMLExporter::writeImages(const Data::Collection* coll_) {
  // keep track of which image fields to write, this is for field names
  QStringList imageFields;
  for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
    if(coll_->fieldByTitle(*it)->type() == Data::Field::Image) {
      imageFields << *it;
    }
  }

  // all the images potentially used in the HTML export need to be written to disk
  // if we're exporting entry files, then we'll certainly want all the image fields written
  // if we're not exporting to a file, then we might be exporting an entry template file
  // and so we need to write all of them too.
  if(m_exportEntryFiles || url().isEmpty()) {
    // add all image fields to string list
    Data::FieldVec fields = coll_->imageFields();
    for(Data::FieldVec::Iterator fieldIt = fields.begin(); fieldIt != fields.end(); ++fieldIt) {
      const QString& fieldName = fieldIt->name();
      if(imageFields.findIndex(fieldName) == -1) { // be sure not to have duplicate values
        imageFields << fieldName;
      }
    }
  }

  // all of them are going to get written to tmp file
  m_handler->addStringParam("imgdir", QFile::encodeName(ImageFactory::tempDir()));

  // call kapp->processEvents(), too
  int count = 0;
  const int processCount = 100; // process after every 100 events

  StringSet imageSet; // track which images are written
  for(QStringList::ConstIterator fieldName = imageFields.begin(); fieldName != imageFields.end(); ++fieldName) {
    for(Data::EntryVec::ConstIterator entryIt = entries().begin(); entryIt != entries().end(); ++entryIt) {
      const QString& id = entryIt->field(*fieldName);
      // if no id or is already writen, continue
      if(id.isEmpty() || imageSet.has(id)) {
        continue;
      }
      imageSet.add(id);
      // try writing
      if(!ImageFactory::writeImage(id, ImageFactory::tempDir(), true)) {
        kdWarning() << "HTMLExporter::writeImages() - unable to write image file: "
                    << ImageFactory::tempDir() << id << endl;
      }

      if(++count == processCount) {
        kapp->processEvents();
        count = 0;
      }
    }
  }
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

  // read current entry export template
  config_->setGroup(QString::fromLatin1("Options - %1").arg(collection()->entryName()));
  m_entryXSLTFile = config_->readEntry("Entry Template", QString::fromLatin1("Default"));
  m_entryXSLTFile = locate("appdata", QString::fromLatin1("entry-templates/")
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

void HTMLExporter::setXSLTFile(const QString& filename_) {
  if(m_xsltFile == filename_) {
    return;
  }

  m_xsltFile = filename_;
  m_xsltFilePath = QString::null;
  reset();
}

KURL HTMLExporter::fileDir() const {
  if(url().isEmpty()) {
    return KURL();
  }

  KURL fileDir = url();
  // cd to directory of target URL
  fileDir.cd(QString::fromLatin1(".."));
  fileDir.addPath(fileDirName());
  return fileDir;
}

QString HTMLExporter::fileDirName() const {
  if(!m_collectionURL.isEmpty()) {
    return m_collectionURL.fileName().section('.', 0, 0) + QString::fromLatin1("_files/");
  }
  return url().fileName().section('.', 0, 0) + QString::fromLatin1("_files/");
}

void HTMLExporter::parseNode(DOM::Node node_) {
  DOM::Element elem = node_;
  if(!elem.isNull()) {
    const DOM::DOMString nodeName = node_.nodeName().upper();
    // to speed up things, check now for nodename
    if(nodeName == "IMG" || nodeName == "SCRIPT" || nodeName == "LINK") {
      DOM::DOMString attrName;
      DOM::Attr attr;
      DOM::NamedNodeMap attrs = elem.attributes();
      const uint lmap = attrs.length();
      for(uint j = 0; j < lmap; ++j) {
        attr = static_cast<DOM::Attr>(attrs.item(j));
        attrName = attr.name().upper();

        if( (attrName == "SRC" && (nodeName == "IMG" || nodeName == "SCRIPT")) ||
            (attrName == "HREF" && nodeName == "LINK")) {
/*          (attrName == "BACKGROUND" && (nodeName == "BODY" ||
                                                       nodeName == "TABLE" ||
                                                       nodeName == "TH" ||
                                                       nodeName == "TD"))) */
          elem.setAttribute(attr.name(), handleLink(attr.value().string()));
        }
      }
    }
  // now it's probably a text node
  } else if(node_.parentNode().nodeName().upper() == "STYLE") {
    node_.setNodeValue(analyzeInternalCSS(node_.nodeValue().string()));
  }

  DOM::Node child = node_.firstChild();
  while(!child.isNull()) {
    parseNode(child);
    child = child.nextSibling();
  }
}

QString HTMLExporter::handleLink(const QString& link_) {
  if(m_links.contains(link_)) {
    return m_links[link_];
  }
  // assume that if the link_ is not relative, then we don't need to copy it
  if(!KURL::isRelativeURL(link_)) {
    m_links.insert(link_, link_);
    return link_;
  }

  if(m_xsltFilePath.isEmpty()) {
    m_xsltFilePath = locate("appdata", m_xsltFile);
    if(m_xsltFilePath.isNull()) {
      myDebug() << "HTMLExporter::handleLink() - no xslt file for " << m_xsltFile << endl;
    }
  }

  KURL u;
  u.setPath(m_xsltFilePath);
  u = KURL(u, link_);
  m_files.append(u);
  m_links.insert(link_, fileDirName() + u.fileName());
  return fileDirName() + u.fileName();
}

QString HTMLExporter::analyzeInternalCSS(const QString& str_) {
  QString str = str_;
  int start = 0;
  int end = 0;
  const int length = str.length();
  const QString url = QString::fromLatin1("url(");
  for(int pos = str.find(url); pos < length && pos >= 0; pos = str.find(url, pos+1)) {
    pos += 4; // url(
    if(str[pos] ==  '"' || str[pos] == '\'') {
      ++pos;
    }

    start = pos;
    pos = str.find(')', start);
    end = pos;
    if(str[pos-1] == '"' || str[pos-1] == '\'') {
      --end;
    }

    str.replace(start, end-start, handleLink(str.mid(start, end-start)));
  }
  return str;
}

bool HTMLExporter::copyFiles() {
  bool checkTarget = true;
  for(KURL::List::ConstIterator it = m_files.begin(); it != m_files.end(); ++it) {
    if(m_copiedFiles.has((*it).url())) {
      continue;
    }
    m_copiedFiles.add((*it).url());

    KURL target = fileDir();
    if(checkTarget && !KIO::NetAccess::exists(target, false, 0)) {
#if KDE_IS_VERSION(3,1,90)
      KIO::NetAccess::mkdir(target, m_widget);
#else
      KIO::NetAccess::mkdir(target);
#endif
      checkTarget = false;
    }

    target.addPath((*it).fileName().section('/', -1));
    // KIO::NetAccess::copy() doesn't force overwrite
    // TODO: should the file be deleted?
#if KDE_IS_VERSION(3,1,90)
    KIO::NetAccess::del(target, m_widget);
    bool success = KIO::NetAccess::copy(*it, target, m_widget);
#else
    KIO::NetAccess::del(target);
    bool success = KIO::NetAccess::copy(*it, target);
#endif
    if(!success) {
      kdWarning() << "HTMLExporter::copyFiles() - can't copy " << target << endl;
      kdWarning() << KIO::NetAccess::lastErrorString() << endl;
    }
  }
  return true;
}

bool HTMLExporter::writeEntryFiles() {
  if(m_entryXSLTFile.isEmpty()) {
    kdWarning() << "HTMLExporter::writeEntryFiles() - no entry XSLT file" << endl;
    return false;
  }

  // now worry about actually exporting entry files
  // I can't reliable encode a string as a URI, so I'm punting, and I'll just replace everything but
  // a-zA-Z0-9 with an underscore. This MUST match the filename template in tellico2html.xsl
  // the id is used so uniqueness is guaranteed
  const QRegExp badChars(QString::fromLatin1("[^-a-zA-Z0-9]"));
  bool formatted = options() & Export::ExportFormatted;

  KURL outputFile = fileDir();

  GUI::CursorSaver cs(Qt::waitCursor);

  int count = 0;
  const int processCount = 100; // process after every 100 events

  HTMLExporter exporter(collection());
  exporter.setOptions(options() | Export::ExportForce);
  exporter.setXSLTFile(m_entryXSLTFile);
  exporter.setCollectionURL(url());

  bool multipleTitles = collection()->fieldByName(QString::fromLatin1("title"))->flags() & Data::Field::AllowMultiple;
  Data::EntryVec entries = this->entries(); // not const since the pointer has to be copied
  for(Data::EntryVecIt entryIt = entries.begin(); entryIt != entries.end(); ++entryIt) {
    QString file = entryIt->field(QString::fromLatin1("title"), formatted);

    // but only use the first title if it has multiple
    if(multipleTitles) {
      file = file.section(';', 0, 0);
    }
    file.replace(badChars, QString::fromLatin1("_"));
    file += QChar('-') + QString::number(entryIt->id()) + QString::fromLatin1(".html");
    outputFile.setFileName(file);

    Data::EntryVec vec;
    vec.append(entryIt);
    exporter.setEntries(vec);
    exporter.setURL(outputFile);
    exporter.exec();

    if(++count == processCount) {
      kapp->processEvents();
      count = 0;
    }
  }

  return true;
}
