/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "htmlexporter.h"
#include "xslthandler.h"
#include "tellicoxmlexporter.h"
#include "../document.h"
#include "../collection.h"
#include "../core/filehandler.h"
#include "../images/image.h"
#include "../images/imagefactory.h"
#include "../images/imageinfo.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../progressmanager.h"
#include "../core/tellico_config.h"
#include "../core/tellico_strings.h"
#include "../gui/cursorsaver.h"
#include "../newstuff/manager.h"
#include "../tellico_debug.h"

#include <kstandarddirs.h>
#include <KConfigGroup>
#include <kglobal.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kapplication.h>
#include <klocale.h>
#include <kuser.h>

#include <QDomDocument>
#include <QGroupBox>
#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>

extern "C" {
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
}

using Tellico::Export::HTMLExporter;

HTMLExporter::HTMLExporter(Tellico::Data::CollPtr coll_) : Tellico::Export::Exporter(coll_),
    m_handler(0),
    m_printHeaders(true),
    m_printGrouped(false),
    m_exportEntryFiles(false),
    m_cancelled(false),
    m_parseDOM(true),
    m_checkCreateDir(true),
    m_checkCommonFile(true),
    m_imageWidth(0),
    m_imageHeight(0),
    m_widget(0),
    m_xsltFile(QLatin1String("tellico2html.xsl")) {
}

HTMLExporter::~HTMLExporter() {
  delete m_handler;
  m_handler = 0;
}

QString HTMLExporter::formatString() const {
  return i18n("HTML");
}

QString HTMLExporter::fileFilter() const {
  return i18n("*.html|HTML Files (*.html)") + QLatin1Char('\n') + i18n("*|All Files");
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
    myWarning() << "trying to export to invalid URL";
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
    ProgressItem& item = ProgressManager::self()->newProgressItem(this, QString(), true);
    item.setTotalSteps(100);
    connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  }
  // ok if not ExportProgress, no worries
  ProgressItem::Done done(this);

  htmlDocPtr htmlDoc = htmlParseDoc(reinterpret_cast<xmlChar*>(text().toUtf8().data()), NULL);
  xmlNodePtr root = xmlDocGetRootElement(htmlDoc);
  if(root == 0) {
    myDebug() << "no root";
    return false;
  }
  parseDOM(root);

  if(m_cancelled) {
    return true; // intentionally cancelled
  }
  ProgressManager::self()->setProgress(this, 15);

  xmlChar* c;
  int bytes;
  htmlDocDumpMemory(htmlDoc, &c, &bytes);
  QString allText;
  if(bytes > 0) {
    allText = QString::fromUtf8(reinterpret_cast<const char*>(c), bytes);
    xmlFree(c);
  }

  if(m_cancelled) {
    return true; // intentionally cancelled
  }
  ProgressManager::self()->setProgress(this, 20);

  bool success = FileHandler::writeTextURL(url(), allText, options() & Export::ExportUTF8, force);
  success &= copyFiles() && (!m_exportEntryFiles || writeEntryFiles());
  return success;
}

bool HTMLExporter::loadXSLTFile() {
  QString xsltfile = KStandardDirs::locate("appdata", m_xsltFile);
  if(xsltfile.isEmpty()) {
    myDebug() << "no xslt file for " << m_xsltFile;
    return false;
  }

  KUrl u;
  u.setPath(xsltfile);
  // do NOT do namespace processing, it messes up the XSL declaration since
  // QDom thinks there are no elements in the Tellico namespace and as a result
  // removes the namespace declaration
  QDomDocument dom = FileHandler::readXMLDocument(u, false);
  if(dom.isNull()) {
    myDebug() << "error loading xslt file: " << xsltfile;
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
  if(m_checkCommonFile && !m_handler->isValid()) {
    NewStuff::Manager::checkCommonFile();
    m_checkCommonFile = false;
    delete m_handler;
    m_handler = new XSLTHandler(dom, QFile::encodeName(xsltfile), true /*translate*/);
  }
  if(!m_handler->isValid()) {
    delete m_handler;
    m_handler = 0;
    return false;
  }
  m_handler->addStringParam("date", QDate::currentDate().toString(Qt::ISODate).toLatin1());
  m_handler->addStringParam("time", QTime::currentTime().toString(Qt::ISODate).toLatin1());
  m_handler->addStringParam("user", KUser(KUser::UseRealUserID).loginName().toLatin1());

  if(m_exportEntryFiles) {
    // export entries to same place as all the other date files
    m_handler->addStringParam("entrydir", QFile::encodeName(fileDir().fileName())+ '/');
    // be sure to link all the entries
    m_handler->addParam("link-entries", "true()");
  }

  if(!m_collectionURL.isEmpty()) {
    QString s = QLatin1String("../") + m_collectionURL.fileName();
    m_handler->addStringParam("collection-file", s.toUtf8());
  }

  // look for a file that gets installed to know the installation directory
  // if parseDOM, that means we want the locations to be the actual location
  // otherwise, we assume it'll be relative
  if(m_parseDOM && m_dataDir.isEmpty()) {
    m_dataDir = KGlobal::dirs()->findResourceDir("appdata", QLatin1String("pics/tellico.png"));
  } else if(!m_parseDOM) {
    m_dataDir.clear();
  }
  if(!m_dataDir.isEmpty()) {
    m_handler->addStringParam("datadir", QFile::encodeName(m_dataDir));
  }

  setFormattingOptions(collection());

  return m_handler->isValid();
}

QString HTMLExporter::text() {
  if((!m_handler || !m_handler->isValid()) && !loadXSLTFile()) {
    myWarning() << "error loading xslt file: " << m_xsltFile;
    return QString();
  }

  Data::CollPtr coll = collection();
  if(!coll) {
    myDebug() << "no collection pointer!";
    return QString();
  }

  if(m_groupBy.isEmpty()) {
    m_printGrouped = false; // can't group if no groups exist
  }

  GUI::CursorSaver cs;
  writeImages(coll);

  // now grab the XML
  TellicoXMLExporter exporter(coll);
  exporter.setURL(url());
  exporter.setEntries(entries());
  exporter.setFields(fields());
  exporter.setIncludeGroups(m_printGrouped);
// yes, this should be in utf8, always
  exporter.setOptions(options() | Export::ExportUTF8 | Export::ExportImages);
  QDomDocument output = exporter.exportXML();
#if 0
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << output.toString();
  }
  f.close();
#endif

  const QString text = m_handler->applyStylesheet(output.toString());
#if 0
  QFile f2(QLatin1String("/tmp/test.html"));
  if(f2.open(QIODevice::WriteOnly)) {
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

void HTMLExporter::setFormattingOptions(Tellico::Data::CollPtr coll) {
  QString file = Kernel::self()->URL().fileName();
  if(file != i18n(Tellico::untitledFilename)) {
    m_handler->addStringParam("filename", QFile::encodeName(file));
  }
  m_handler->addStringParam("cdate", KGlobal::locale()->formatDate(QDate::currentDate()).toUtf8());
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
  if(!m_sort3.isEmpty() && sortTitles.indexOf(m_sort3) == -1) {
    sortTitles << m_sort3;
  }

  if(sortTitles.count() > 0) {
    m_handler->addStringParam("sort-name1", coll->fieldNameByTitle(sortTitles[0]).toUtf8());
    if(sortTitles.count() > 1) {
      m_handler->addStringParam("sort-name2", coll->fieldNameByTitle(sortTitles[1]).toUtf8());
      if(sortTitles.count() > 2) {
        m_handler->addStringParam("sort-name3", coll->fieldNameByTitle(sortTitles[2]).toUtf8());
      }
    }
  }

  // no longer showing "sorted by..." since the column headers are clickable
  // but still use "grouped by"
  QString sortString;
  if(m_printGrouped) {
    if(!m_groupBy.isEmpty()) {
      QString s;
      // if more than one, then it's the People pseudo-group
      if(m_groupBy.count() > 1) {
        s = i18n("People");
      } else {
        s = coll->fieldTitleByName(m_groupBy[0]);
      }
      sortString = i18n("(grouped by %1)", s);
    }

    QString groupFields;
    for(QStringList::ConstIterator it = m_groupBy.constBegin(); it != m_groupBy.constEnd(); ++it) {
      Data::FieldPtr f = coll->fieldByName(*it);
      if(!f) {
        continue;
      }
      if(f->hasFlag(Data::Field::AllowMultiple)) {
        groupFields += QLatin1String("tc:") + *it + QLatin1String("s/tc:") + *it;
      } else {
        groupFields += QLatin1String("tc:") + *it;
      }
      int ncols = 0;
      if(f->type() == Data::Field::Table) {
        bool ok;
        ncols = Tellico::toUInt(f->property(QLatin1String("columns")), &ok);
        if(!ok) {
          ncols = 1;
        }
      }
      if(ncols > 1) {
        groupFields += QLatin1String("/tc:column[1]");
      }
      if(*it != m_groupBy.last()) {
        groupFields += QLatin1Char('|');
      }
    }
//    myDebug() << groupFields;
    m_handler->addStringParam("group-fields", groupFields.toUtf8());
    m_handler->addStringParam("sort-title", sortString.toUtf8());
  }

  QString pageTitle = coll->title();
  pageTitle += QLatin1Char(' ') + sortString;
  m_handler->addStringParam("page-title", pageTitle.toUtf8());

  QStringList showFields;
  foreach(const QString& column, m_columns) {
    showFields << coll->fieldNameByTitle(column);
  }
  m_handler->addStringParam("column-names", showFields.join(QLatin1String(" ")).toUtf8());

  if(m_imageWidth > 0 && m_imageHeight > 0) {
    m_handler->addParam("image-width", QByteArray().setNum(m_imageWidth));
    m_handler->addParam("image-height", QByteArray().setNum(m_imageHeight));
  }

  // add system colors to stylesheet
  const int type = coll->type();
  m_handler->addStringParam("font",     Config::templateFont(type).family().toLatin1());
  m_handler->addStringParam("fontsize", QByteArray().setNum(Config::templateFont(type).pointSize()));
  m_handler->addStringParam("bgcolor",  Config::templateBaseColor(type).name().toLatin1());
  m_handler->addStringParam("fgcolor",  Config::templateTextColor(type).name().toLatin1());
  m_handler->addStringParam("color1",   Config::templateHighlightedTextColor(type).name().toLatin1());
  m_handler->addStringParam("color2",   Config::templateHighlightedBaseColor(type).name().toLatin1());

  // add locale code to stylesheet (for sorting)
  m_handler->addStringParam("lang", KGlobal::locale()->language().toLatin1());
}

void HTMLExporter::writeImages(Tellico::Data::CollPtr coll_) {
  // keep track of which image fields to write, this is for field names
  StringSet imageFields;
  foreach(const QString& column, m_columns) {
    if(coll_->fieldByTitle(column)->type() == Data::Field::Image) {
      imageFields.add(column);
    }
  }

  // all the images potentially used in the HTML export need to be written to disk
  // if we're exporting entry files, then we'll certainly want all the image fields written
  // if we're not exporting to a file, then we might be exporting an entry template file
  // and so we need to write all of them too.
  if(m_exportEntryFiles || url().isEmpty()) {
    // add all image fields to string list
    Data::FieldList iFields = coll_->imageFields();
    // take intersection with the fields to be exported
    iFields = QSet<Data::FieldPtr>::fromList(iFields).intersect(fields().toSet()).toList();
    foreach(Data::FieldPtr field, iFields) {
      imageFields.add(field->name());
    }
  }

  // all of them are going to get written to tmp file
  bool useTemp = url().isEmpty();
  KUrl imgDir;
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
    imgDirRelative = ImageFactory::imageDir();
    createDir();
  } else {
    imgDir = fileDir();
    imgDirRelative = KUrl::relativeUrl(url(), imgDir);
    createDir();
  }
  m_handler->addStringParam("imgdir", QFile::encodeName(imgDirRelative));

  int count = 0;
  const int processCount = 100; // process after every 100 events

  StringSet imageSet; // track which images are written
  foreach(const QString& imageField, imageFields) {
    foreach(Data::EntryPtr entryIt, entries()) {
      QString id = entryIt->field(imageField);
      // if no id or is already writen, continue
      if(id.isEmpty() || imageSet.has(id)) {
        continue;
      }
      imageSet.add(id);
      // try writing
      bool success = false;
      if(useTemp) {
        // for link-only images, no need to write it out
        success = ImageFactory::imageInfo(id).linkOnly || ImageFactory::writeCachedImage(id, ImageFactory::TempDir);
      } else {
        const Data::Image& img = ImageFactory::imageById(id);
        KUrl target = imgDir;
        target.addPath(id);
        success = !img.isNull() && FileHandler::writeDataURL(target, img.byteArray(), true);
      }
      if(!success) {
        myWarning() << "unable to write image file: "
                    << imgDir.path() << id;
      }

      if(++count == processCount) {
        kapp->processEvents();
        count = 0;
      }
    }
  }
}

QWidget* HTMLExporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("HTML Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_checkPrintHeaders = new QCheckBox(i18n("Print field headers"), gbox);
  m_checkPrintHeaders->setWhatsThis(i18n("If checked, the field names will be "
                                         "printed as table headers."));
  m_checkPrintHeaders->setChecked(m_printHeaders);

  m_checkPrintGrouped = new QCheckBox(i18n("Group the entries"), gbox);
  m_checkPrintGrouped->setWhatsThis(i18n("If checked, the entries will be grouped by "
                                         "the selected field."));
  m_checkPrintGrouped->setChecked(m_printGrouped);

  m_checkExportEntryFiles = new QCheckBox(i18n("Export individual entry files"), gbox);
  m_checkExportEntryFiles->setWhatsThis(i18n("If checked, individual files will be created for each entry."));
  m_checkExportEntryFiles->setChecked(m_exportEntryFiles);

  vlay->addWidget(m_checkPrintHeaders);
  vlay->addWidget(m_checkPrintGrouped);
  vlay->addWidget(m_checkExportEntryFiles);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void HTMLExporter::readOptions(KSharedConfigPtr config_) {
  KConfigGroup exportConfig(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_printHeaders = exportConfig.readEntry("Print Field Headers", m_printHeaders);
  m_printGrouped = exportConfig.readEntry("Print Grouped", m_printGrouped);
  m_exportEntryFiles = exportConfig.readEntry("Export Entry Files", m_exportEntryFiles);

  // read current entry export template
  m_entryXSLTFile = Config::templateName(collection()->type());
  m_entryXSLTFile = KStandardDirs::locate("appdata", QLatin1String("entry-templates/")
                                          + m_entryXSLTFile + QLatin1String(".xsl"));
}

void HTMLExporter::saveOptions(KSharedConfigPtr config_) {
  KConfigGroup cfg(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_printHeaders = m_checkPrintHeaders->isChecked();
  cfg.writeEntry("Print Field Headers", m_printHeaders);
  m_printGrouped = m_checkPrintGrouped->isChecked();
  cfg.writeEntry("Print Grouped", m_printGrouped);
  m_exportEntryFiles = m_checkExportEntryFiles->isChecked();
  cfg.writeEntry("Export Entry Files", m_exportEntryFiles);
}

void HTMLExporter::setXSLTFile(const QString& filename_) {
  if(m_xsltFile == filename_) {
    return;
  }

  m_xsltFile = filename_;
  m_xsltFilePath.clear();
  reset();
}

KUrl HTMLExporter::fileDir() const {
  if(url().isEmpty()) {
    return KUrl();
  }
  KUrl fileDir = url();
  // cd to directory of target URL
  fileDir.cd(QLatin1String(".."));
  fileDir.addPath(fileDirName());
  return fileDir;
}

QString HTMLExporter::fileDirName() const {
  if(!m_collectionURL.isEmpty()) {
    return QLatin1String("/");
  }
  return url().fileName().section(QLatin1Char('.'), 0, 0) + QLatin1String("_files/");
}

// how ugly is this?
const xmlChar* HTMLExporter::handleLink(const xmlChar* link_) {
  return reinterpret_cast<xmlChar*>(qstrdup(handleLink(QString::fromUtf8(reinterpret_cast<const char*>(link_))).toUtf8()));
}

QString HTMLExporter::handleLink(const QString& link_) {
  if(m_links.contains(link_)) {
    return m_links[link_];
  }
  // assume that if the link_ is not relative, then we don't need to copy it
  if(!KUrl::isRelativeUrl(link_)) {
    return link_;
  }

  if(m_xsltFilePath.isEmpty()) {
    m_xsltFilePath = KStandardDirs::locate("appdata", m_xsltFile);
    if(m_xsltFilePath.isEmpty()) {
      myWarning() << "no xslt file for " << m_xsltFile;
    }
  }

  KUrl u;
  u.setPath(m_xsltFilePath);
  u = KUrl(u, link_);

  // one of the "quirks" of the html export is that img src urls are set to point to
  // the tmpDir() when exporting entry files from a collection, but those images
  // don't actually exist, and they get copied in writeImages() instead.
  // so we only need to keep track of the url if it exists
  const bool exists = KIO::NetAccess::exists(u, KIO::NetAccess::DestinationSide, 0);
  if(exists) {
    m_files.append(u);
  }

  // if we're exporting entry files, we want pics/ to
  // go in pics/
  const bool isPic = link_.startsWith(m_dataDir + QLatin1String("pics/"));
  QString midDir;
  if(m_exportEntryFiles && isPic) {
    midDir = QLatin1String("pics/");
  }
  // pictures are special since they might not exist when the HTML is exported, since they might get copied later
  // on the other hand, don't change the file location if it doesn't exist
  if(isPic || exists) {
    m_links.insert(link_, fileDirName() + midDir + u.fileName());
  } else {
    m_links.insert(link_, link_);
  }
  return m_links[link_];
}

const xmlChar* HTMLExporter::analyzeInternalCSS(const xmlChar* str_) {
  return reinterpret_cast<xmlChar*>(qstrdup(analyzeInternalCSS(QString::fromUtf8(reinterpret_cast<const char*>(str_))).toUtf8()));
}

QString HTMLExporter::analyzeInternalCSS(const QString& str_) {
  QString str = str_;
  int start = 0;
  int end = 0;
  const QString url = QLatin1String("url(");
  for(int pos = str.indexOf(url); pos >= 0; pos = str.indexOf(url, pos+1)) {
    pos += 4; // url(
    if(str[pos] ==  QLatin1Char('"') || str[pos] == QLatin1Char('\'')) {
      ++pos;
    }

    start = pos;
    pos = str.indexOf(QLatin1Char(')'), start);
    end = pos;
    if(str[pos-1] == QLatin1Char('"') || str[pos-1] == QLatin1Char('\'')) {
      --end;
    }

    str.replace(start, end-start, handleLink(str.mid(start, end-start)));
  }
  return str;
}

void HTMLExporter::createDir() {
  if(!m_checkCreateDir) {
    return;
  }
  KUrl dir = fileDir();
  if(dir.isEmpty()) {
    myDebug() << "called on empty URL!";
    return;
  }
  if(KIO::NetAccess::exists(dir, KIO::NetAccess::DestinationSide, 0)) {
    m_checkCreateDir = false;
  } else {
    m_checkCreateDir = !KIO::NetAccess::mkdir(dir, m_widget);
  }
}

bool HTMLExporter::copyFiles() {
  if(m_files.isEmpty()) {
    return true;
  }
  const int start = 20;
  const int maxProgress = m_exportEntryFiles ? 40 : 80;
  const int stepSize = qMax(1, m_files.count()/maxProgress);
  int j = 0;

  createDir();
  KUrl target;
  for(KUrl::List::ConstIterator it = m_files.constBegin(); it != m_files.constEnd() && !m_cancelled; ++it, ++j) {
    if(m_copiedFiles.has((*it).url())) {
      continue;
    }

    if(target.isEmpty()) {
      target = fileDir();
    }
    target.setFileName((*it).fileName());
    KIO::FileCopyJob* job = KIO::file_copy(*it, target, -1, KIO::Overwrite);
    bool success = KIO::NetAccess::synchronousRun(job, m_widget);
    if(success) {
      m_copiedFiles.add((*it).url());
    } else {
      myWarning() << "can't copy " << target;
      myWarning() << KIO::NetAccess::lastErrorString();
    }
    if(j%stepSize == 0) {
      if(options() & ExportProgress) {
        ProgressManager::self()->setProgress(this, qMin(start+j/stepSize, 99));
      }
      kapp->processEvents();
    }
  }
  return true;
}

bool HTMLExporter::writeEntryFiles() {
  if(m_entryXSLTFile.isEmpty()) {
    myWarning() << "no entry XSLT file";
    return false;
  }

  const int start = 60;
  const int stepSize = qMax(1, entries().count()/40);
  int j = 0;

  // now worry about actually exporting entry files
  // I can't reliable encode a string as a URI, so I'm punting, and I'll just replace everything but
  // a-zA-Z0-9 with an underscore. This MUST match the filename template in tellico2html.xsl
  // the id is used so uniqueness is guaranteed
  const QRegExp badChars(QLatin1String("[^-a-zA-Z0-9]"));
  FieldFormat::Request formatted = (options() & Export::ExportFormatted ?
                                                   FieldFormat::ForceFormat :
                                                   FieldFormat::AsIsFormat);

  KUrl outputFile = fileDir();

  GUI::CursorSaver cs(Qt::WaitCursor);

  HTMLExporter exporter(collection());
  long opt = options() | Export::ExportForce;
  opt &= ~ExportProgress;
  exporter.setFields(fields());
  exporter.setOptions(opt);
  exporter.setXSLTFile(m_entryXSLTFile);
  exporter.setCollectionURL(url());
  bool parseDOM = true;

  const QString title = QLatin1String("title");
  const QString html = QLatin1String(".html");
  bool multipleTitles = collection()->fieldByName(title)->hasFlag(Data::Field::AllowMultiple);
  Data::EntryList entries = this->entries(); // not const since the pointer has to be copied
  foreach(Data::EntryPtr entryIt, entries) {
    QString file = entryIt->formattedField(title, formatted);

    // but only use the first title if it has multiple
    if(multipleTitles) {
      file = file.section(QLatin1Char(';'), 0, 0);
    }
    file.replace(badChars, QLatin1String("_"));
    file += QLatin1Char('-') + QString::number(entryIt->id()) + html;
    outputFile.setFileName(file);

    exporter.setEntries(Data::EntryList() << entryIt);
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
        ProgressManager::self()->setProgress(this, qMin(start+j/stepSize, 99));
      }
      kapp->processEvents();
    }
    ++j;
  }
  // the images in "pics/" are special data images, copy them always
  // since the entry files may refer to them, but we don't know that
  QStringList dataImages;
  dataImages << QLatin1String("checkmark.png");
  for(uint i = 1; i <= 10; ++i) {
    dataImages << QString::fromLatin1("stars%1.png").arg(i);
  }
  KUrl dataDir;
  dataDir.setPath(KGlobal::dirs()->findResourceDir("appdata", QLatin1String("pics/tellico.png")) + QLatin1String("pics/"));
  KUrl target = fileDir();
  target.addPath(QLatin1String("pics/"));
  KIO::NetAccess::mkdir(target, m_widget);
  foreach(const QString& dataImage, dataImages) {
    dataDir.setFileName(dataImage);
    target.setFileName(dataImage);
    KIO::NetAccess::file_copy(dataDir, target, m_widget);
  }

  return true;
}

void HTMLExporter::slotCancel() {
  m_cancelled = true;
}

void HTMLExporter::parseDOM(xmlNode* node_) {
  if(node_ == 0) {
    myDebug() << "no node";
    return;
  }

  bool parseChildren = true;

  if(node_->type == XML_ELEMENT_NODE) {
    const QByteArray nodeName = QByteArray(reinterpret_cast<const char*>(node_->name)).toUpper();
    xmlElement* elem = reinterpret_cast<xmlElement*>(node_);
    // to speed up things, check now for nodename
    if(nodeName == "IMG" || nodeName == "SCRIPT" || nodeName == "LINK") {
      for(xmlAttribute* attr = elem->attributes; attr; attr = reinterpret_cast<xmlAttribute*>(attr->next)) {
        QByteArray attrName = QByteArray(reinterpret_cast<const char*>(attr->name)).toUpper();

        if( (attrName == "SRC" && (nodeName == "IMG" || nodeName == "SCRIPT")) ||
            (attrName == "HREF" && nodeName == "LINK")) {
/*          (attrName == "BACKGROUND" && (nodeName == "BODY" ||
                                                       nodeName == "TABLE" ||
                                                       nodeName == "TH" ||
                                                       nodeName == "TD"))) */
          xmlChar* value = xmlGetProp(node_, attr->name);
          if(value) {
            xmlSetProp(node_, attr->name, handleLink(value));
            xmlFree(value);
          }
          // each node only has one significant attribute, so break now
          break;
        }
      }
    } else if(nodeName == "STYLE") {
      // if the first child is a CDATA, use it, otherwise replace complete node
      xmlNode* nodeToReplace = node_;
      xmlNode* child = node_->children;
      if(child && child->type == XML_CDATA_SECTION_NODE) {
        nodeToReplace = child;
      }
      xmlChar* value = xmlNodeGetContent(nodeToReplace);
      if(value) {
        xmlNodeSetContent(nodeToReplace, analyzeInternalCSS(value));
        xmlFree(value);
      }
      // no longer need to parse child text nodes
      parseChildren = false;
    }
  }

  if(parseChildren) {
    xmlNode* child = node_->children;
    while(child) {
      parseDOM(child);
      child = child->next;
    }
  }
}

#include "htmlexporter.moc"
