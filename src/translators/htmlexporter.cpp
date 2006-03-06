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
#include "../progressmanager.h"

#include <kstandarddirs.h>
#include <kconfig.h>
#include <kglobalsettings.h>
#include <kglobal.h>
#include <kio/netaccess.h>
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
    m_cancelled(false),
    m_parseDOM(true),
    m_imageWidth(0),
    m_imageHeight(0),
    m_widget(0),
    m_xsltFile(QString::fromLatin1("tellico2html.xsl")) {
}

HTMLExporter::HTMLExporter(Data::CollPtr coll_) : Tellico::Export::Exporter(coll_),
    m_part(0),
    m_handler(0),
    m_printHeaders(true),
    m_printGrouped(false),
    m_exportEntryFiles(false),
    m_cancelled(false),
    m_parseDOM(true),
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
  LOG_FUNC;
  if(url().isEmpty() || !url().isValid()) {
    kdWarning() << "HTMLExporter::exec() - trying to export to invalid URL" << endl;
    return false;
  }

  // check file exists first
  // if we're not forcing, ask use
  bool force = (options() & Export::ExportForce) || FileHandler::queryExists(url());
  if(!force) {
    return false;
  }

  if(!m_parseDOM) {
    return FileHandler::writeTextURL(url(), text(), options() & Export::ExportUTF8, force);
  }

  m_cancelled = false;
  // TODO: maybe need label?
  if(options() & ExportProgress) {
    ProgressItem& item = ProgressManager::self()->newProgressItem(this, QString::null, true);
    item.setTotalSteps(100);
    connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  }

  // this is expensive, but necessary to easily crawl the DOM and fetch external images
  if(!m_part) {
    ProgressManager::self()->setProgress(this, 5);
    m_part = new KHTMLPart((QWidget*)0);
    m_part->setJScriptEnabled(false);
    m_part->setJavaEnabled(false);
    m_part->setMetaRefreshEnabled(false);
    m_part->setPluginsEnabled(false);
  }

  ProgressManager::self()->setProgress(this, 10);

  m_part->begin();
  m_part->write(text());
  m_part->end();

  if(m_cancelled) {
    return true; // intentionally cancelled
  }
  ProgressManager::self()->setProgress(this, 15);

  DOM::HTMLElement e = m_part->document().documentElement();
  // a little paranoia, the XSL transform might fail
  if(e.isNull() || !e.hasChildNodes()) {
    return false;
  }

  parseNode(e);
  ProgressManager::self()->setProgress(this, 20);

  if(m_cancelled) {
    return true; // intentionally cancelled
  }

  bool success = FileHandler::writeTextURL(url(),
                                           QString::fromLatin1("<html>")
                                             + e.innerHTML().string()
                                             + QString::fromLatin1("</html>"),
                                           options() & Export::ExportUTF8,
                                           force);
  success &= copyFiles() && (!m_exportEntryFiles || writeEntryFiles());
  ProgressManager::self()->setDone(this);
  return success;
}

bool HTMLExporter::loadXSLTFile() {
  QString xsltfile = locate("appdata", m_xsltFile);
  if(xsltfile.isNull()) {
    myDebug() << "HTMLExporter::loadXSLTFile() - no xslt file for " << m_xsltFile << endl;
    return false;
  }

  KURL u;
  u.setPath(xsltfile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements in the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLFile(u, false);
  if(dom.isNull()) {
    myDebug() << "HTMLExporter::loadXSLTFile() - error loading xslt file: " << xsltfile << endl;
    return false;
  }

  // notes about utf-8 encoding:
  // all params should be passed to XSLTHandler in utf8
  // input string to XSLTHandler should be in utf-8, EVEN IF DOM STRING SAYS OTHERWISE

  // the stylesheet prints utf-8 by default, if using locale encoding, need
  // to change the encoding attribute on the xsl:output element
  if(!(options() & Export::ExportUTF8)) {
    XSLTHandler::setLocaleEncoding(dom);
  }

  delete m_handler;
  m_handler = new XSLTHandler(dom, QFile::encodeName(xsltfile), true /*translate*/);
  if(!m_handler->isValid()) {
    delete m_handler;
    m_handler = 0;
    return false;
  }

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
  // if parseDOM, that means we want the locations to be the actual location
  // otherwise, we assume it'll be relative
  if(m_parseDOM && m_dataDir.isEmpty()) {
    m_dataDir = KGlobal::dirs()->findResourceDir("appdata", QString::fromLatin1("pics/tellico.png"));
  } else if(!m_parseDOM) {
    m_dataDir.truncate(0);
  }
  if(!m_dataDir.isEmpty()) {
    m_handler->addStringParam("datadir", QFile::encodeName(m_dataDir));
  }

  setFormattingOptions(collection());

  return m_handler->isValid();
}

QString HTMLExporter::text() {
  LOG_FUNC;
  if((!m_handler || !m_handler->isValid()) && !loadXSLTFile()) {
    kdWarning() << "HTMLExporter::text() - error loading xslt file: " << m_xsltFile << endl;
    return QString::null;
  }

  Data::CollPtr coll = collection();
  if(!coll) {
    myDebug() << "HTMLExporter::text() - no collection pointer!" << endl;
    return QString::null;
  }

  if(m_groupBy.isEmpty()) {
    m_printGrouped = false; // can't group if no groups exist
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
//    t << "\n\n-------------------------------------------------------\n\n";
//    t << Tellico::i18nReplace(text);
  }
  f2.close();
#endif
  // the XSLT file gets translated instead
//  return Tellico::i18nReplace(text);
  return text;
}

void HTMLExporter::setFormattingOptions(Data::CollPtr coll) {
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
      Data::FieldPtr f = coll->fieldByName(*it);
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
//    myDebug() << groupFields << endl;
    m_handler->addStringParam("group-fields", groupFields.utf8());
    m_handler->addStringParam("sort-title", sortString.utf8());
  }

  QString pageTitle = coll->title();
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

  // add locale code to stylesheet (for sorting)
  m_handler->addStringParam("lang",  KGlobal::locale()->languagesTwoAlpha().first().utf8());
  ;
}

void HTMLExporter::writeImages(Data::CollPtr coll_) {
  // keep track of which image fields to write, this is for field names
  StringSet imageFields;
  for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
    if(coll_->fieldByTitle(*it)->type() == Data::Field::Image) {
      imageFields.add(*it);
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
      imageFields.add(fieldIt->name());
    }
  }

  // all of them are going to get written to tmp file
  bool useTemp = url().isEmpty();
  KURL imgDir;
  QString imgDirRelative;
  // really some convoluted logic here
  // basically, four cases. 1) we're writing to a tmp file, for printing probably
  // so then write all the images to the tmp directory, 2) we're exporting to HTML, and
  // this is the main collection file, in which case m_parseDOM is always true;
  // 3) we're exporting HTML, and this is the first entry file, for which parseDOM is true
  // and exportEntryFiles is false. Then the image file will get copied in copyFiles() and is
  // probably an image in the entry template. 4) we're exporting HTML, and this is not the
  // first entry file, in which case, we want to refer directly to the target dir
  if(useTemp) { // everything goes in the tmp dir
    imgDir.setPath(ImageFactory::tempDir());
    imgDirRelative = imgDir.path();
  } else if(m_parseDOM) {
    imgDir = fileDir(); // copy to fileDir
    imgDirRelative = ImageFactory::tempDir(); // use tempDir to read from since the DOM is parsed
                                              // and the link will get changed later
    createDir();
  } else {
    imgDir = fileDir();
    imgDirRelative = KURL::relativeURL(url(), imgDir);
    createDir();
  }
  m_handler->addStringParam("imgdir", QFile::encodeName(imgDirRelative));

  int count = 0;
  const int processCount = 100; // process after every 100 events

  QStringList fieldsList = imageFields.toList();
  StringSet imageSet; // track which images are written
  for(QStringList::ConstIterator fieldName = fieldsList.begin(); fieldName != fieldsList.end(); ++fieldName) {
    for(Data::EntryVec::ConstIterator entryIt = entries().begin(); entryIt != entries().end(); ++entryIt) {
      QString id = entryIt->field(*fieldName);
      // if no id or is already writen, continue
      if(id.isEmpty() || imageSet.has(id)) {
        continue;
      }
      imageSet.add(id);
      // try writing
      bool success = useTemp ? ImageFactory::writeImage(id, ImageFactory::TempDir)
                             : ImageFactory::copyImage(id, imgDir, true);
      if(!success) {
        kdWarning() << "HTMLExporter::writeImages() - unable to write image file: "
                    << imgDir.path() << id << endl;
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
  config_->setGroup(QString::fromLatin1("Options - %1").arg(collection()->typeName()));
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
//    return QString::fromLatin1("../") + m_collectionURL.fileName().section('.', 0, 0) + QString::fromLatin1("_files/");
    return QString::fromLatin1("/");
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
      kdWarning() << "HTMLExporter::handleLink() - no xslt file for " << m_xsltFile << endl;
    }
  }

  KURL u;
  u.setPath(m_xsltFilePath);
  u = KURL(u, link_);
  m_files.append(u);
  // if we're exporting entry files, we want pics/ to
  // go in pics/
  QString midDir;
  if(link_.startsWith(m_dataDir + QString::fromLatin1("pics/"))) {
    midDir += QString::fromLatin1("pics/");
  }
  m_links.insert(link_, fileDirName() + midDir + u.fileName());
  return m_links[link_];
}

QString HTMLExporter::analyzeInternalCSS(const QString& str_) {
  QString str = str_;
  int start = 0;
  int end = 0;
  const QString url = QString::fromLatin1("url(");
  for(int pos = str.find(url); pos >= 0; pos = str.find(url, pos+1)) {
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

bool HTMLExporter::createDir() {
  LOG_FUNC;
  KURL dir = fileDir();
  if(dir.isEmpty()) {
    myDebug() << "HTMLExporter::createDir() - called on empty URL!" << endl;
    return true;
  }
  if(KIO::NetAccess::exists(dir, false, 0)) {
    return false;
  }
  return KIO::NetAccess::mkdir(dir, m_widget);
}

bool HTMLExporter::copyFiles() {
  const uint start = 20;
  const uint maxProgress = m_exportEntryFiles ? 40 : 80;
  const uint stepSize = QMAX(1, m_files.count()/maxProgress);
  uint j = 0;

  bool checkTarget = true;
  for(KURL::List::ConstIterator it = m_files.begin(); it != m_files.end() && !m_cancelled; ++it, ++j) {
    if(m_copiedFiles.has((*it).url())) {
      continue;
    }

    KURL target = fileDir();
    if(checkTarget) {
      checkTarget = false;
      createDir();
    }

    target.addPath((*it).fileName());
    bool success = KIO::NetAccess::file_copy(*it, target, -1, true /* overwrite */, false /* resume */, m_widget);
    if(success) {
      m_copiedFiles.add((*it).url());
    } else {
      kdWarning() << "HTMLExporter::copyFiles() - can't copy " << target << endl;
      kdWarning() << KIO::NetAccess::lastErrorString() << endl;
    }
    if(j%stepSize == 0) {
      if(options() & ExportProgress) {
        ProgressManager::self()->setProgress(this, QMIN(start+j/stepSize, 99));
      }
      kapp->processEvents();
    }
  }
  return true;
}

bool HTMLExporter::writeEntryFiles() {
  LOG_FUNC;
  if(m_entryXSLTFile.isEmpty()) {
    kdWarning() << "HTMLExporter::writeEntryFiles() - no entry XSLT file" << endl;
    return false;
  }

  const uint start = 60;
  const uint stepSize = QMAX(1, entries().count()/40);
  uint j = 0;

  // now worry about actually exporting entry files
  // I can't reliable encode a string as a URI, so I'm punting, and I'll just replace everything but
  // a-zA-Z0-9 with an underscore. This MUST match the filename template in tellico2html.xsl
  // the id is used so uniqueness is guaranteed
  const QRegExp badChars(QString::fromLatin1("[^-a-zA-Z0-9]"));
  bool formatted = options() & Export::ExportFormatted;

  KURL outputFile = fileDir();

  GUI::CursorSaver cs(Qt::waitCursor);

  HTMLExporter exporter(collection());
  long opt = options() | Export::ExportForce;
  opt &= ~ExportProgress;
  exporter.setOptions(opt);
  exporter.setXSLTFile(m_entryXSLTFile);
  exporter.setCollectionURL(url());
  bool parseDOM = true;

  const QString title = QString::fromLatin1("title");
  const QString html = QString::fromLatin1(".html");
  bool multipleTitles = collection()->fieldByName(title)->flags() & Data::Field::AllowMultiple;
  Data::EntryVec entries = this->entries(); // not const since the pointer has to be copied
  for(Data::EntryVecIt entryIt = entries.begin(); entryIt != entries.end() && !m_cancelled; ++entryIt, ++j) {
    QString file = entryIt->field(title, formatted);

    // but only use the first title if it has multiple
    if(multipleTitles) {
      file = file.section(';', 0, 0);
    }
    file.replace(badChars, QChar('_'));
    file += QChar('-') + QString::number(entryIt->id()) + html;
    outputFile.setFileName(file);

    exporter.setEntries(Data::EntryVec(entryIt));
    exporter.setURL(outputFile);
    exporter.exec();

    // no longer need to parse DOM
    if(parseDOM) {
      parseDOM = false;
      exporter.setParseDOM(false);
      // this is rather stupid, but I'm too lazy to figure out the better way
      // since we parsed the DOM for the first entry file to grab any
      // images used in the template, need to resave it so the image links
      // get written correctly
      exporter.exec();
    }

    if(j%stepSize == 0) {
      if(options() & ExportProgress) {
        ProgressManager::self()->setProgress(this, QMIN(start+j/stepSize, 99));
      }
      kapp->processEvents();
    }
  }
  // the images in "pics/" are special data images, copy them always
  QStringList dataImages;
  dataImages << QString::fromLatin1("checkmark.png");
  for(uint i = 1; i <= 10; ++i) {
    dataImages << QString::fromLatin1("stars%1.png").arg(i);
  }
  KURL dataDir;
  dataDir.setPath(KGlobal::dirs()->findResourceDir("appdata", QString::fromLatin1("pics/tellico.png")) + "pics/");
  KURL target = fileDir();
  target.addPath(QString::fromLatin1("pics/"));
  KIO::NetAccess::mkdir(target, m_widget);
  for(QStringList::ConstIterator it = dataImages.begin(); it != dataImages.end(); ++it) {
    dataDir.setFileName(*it);
    target.setFileName(*it);
    KIO::NetAccess::copy(dataDir, target, m_widget);
  }

  return true;
}

void HTMLExporter::slotCancel() {
  m_cancelled = true;
}

#include "htmlexporter.moc"
