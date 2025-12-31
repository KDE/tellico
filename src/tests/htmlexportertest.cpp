/***************************************************************************
    Copyright (C) 2015 Robby Stephenson <robby@periapsis.org>
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

#undef QT_NO_CAST_FROM_ASCII

#include "htmlexportertest.h"

#include "../translators/htmlexporter.h"
#include "../translators/tellicoimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../document.h"
#include "../images/imagefactory.h"
#include "../models/entrygroupmodel.h"
#include "../models/modelmanager.h"
#include "../utils/datafileregistry.h"
#include "../config/tellico_config.h"

#include <KLocalizedString>

#include <QTest>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QFile>
#include <QStandardPaths>
#include <QProcess>
#include <QLoggingCategory>

QTEST_GUILESS_MAIN( HtmlExporterTest )

void HtmlExporterTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = false"));
  Tellico::ImageFactory::init();
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerVideo(Tellico::Data::Collection::Video, "video");
  Tellico::RegisterCollection<Tellico::Data::VideoCollection> registerMusic(Tellico::Data::Collection::Album, "album");
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/tellico2html.xsl"));
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/entry-templates/Fancy.xsl"));
  Tellico::DataFileRegistry::self()->addDataLocation(QFINDTESTDATA("../../xslt/report-templates/Column_View.xsl"));
}

void HtmlExporterTest::cleanupTestCase() {
  Tellico::ImageFactory::clean(true);
}

void HtmlExporterTest::testHtml() {
  Tellico::Config::setImageLocation(Tellico::Config::ImagesInLocalDir);
  // the default collection will use a temporary directory as a local image dir
  QVERIFY(!Tellico::ImageFactory::localDir().isEmpty());

  QString tempDirName;
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());
  tempDir.setAutoRemove(true);
  tempDirName = tempDir.path();
  QString fileName = tempDirName + "/with-image.tc";
  QString imageDirName = tempDirName + "/with-image_files/";

  // copy a collection file that includes an image into the temporary directory
  QVERIFY(QFile::copy(QFINDTESTDATA("data/with-image.tc"), fileName));

  Tellico::Data::Document* doc = Tellico::Data::Document::self();
  QVERIFY(doc->openDocument(QUrl::fromLocalFile(fileName)));
  QCOMPARE(Tellico::ImageFactory::localDir(), QUrl::fromLocalFile(imageDirName));
  // save the document, so the images get copied out of the .tc file into the local image directory
  QVERIFY(doc->saveDocument(QUrl::fromLocalFile(fileName)));

  auto groupModel = new Tellico::EntryGroupModel(this);
  Tellico::ModelManager::self()->setGroupModel(groupModel);

  Tellico::Data::CollPtr coll = doc->collection();
  QVERIFY(coll);

  Tellico::Export::HTMLExporter exp(coll, Tellico::Data::Document::self()->URL());
  exp.setEntries(coll->entries());
  exp.setExportEntryFiles(true);
  exp.setEntryXSLTFile(QStringLiteral("Fancy"));
  exp.setColumns(QStringList() << QStringLiteral("Title") << QStringLiteral("Gift")
                               << QStringLiteral("Rating") << QStringLiteral("Front Cover"));
  exp.setSortTitles({QStringLiteral("Title"), QStringLiteral("Gift"), QStringLiteral("Rating")});
  exp.setPrintGrouped(true);
  exp.setGroupBy({QStringLiteral("binding"), QStringLiteral("author")});
  exp.setMaxImageSize(250, 250);
  exp.setURL(QUrl::fromLocalFile(tempDirName + "/testHtml.html"));

  QCOMPARE(exp.formatString(), QLatin1String("HTML"));
  QVERIFY(exp.fileFilter().contains(QLatin1String(";;All Files (*)")));

  QString output = exp.text();
  QVERIFY(!output.isEmpty());

  // verify the relative location of the tellico2html.js file
  QVERIFY(output.contains(QStringLiteral("src=\"testHtml_files/tellico2html.js")));
  // verify relative location of image pics
  QVERIFY(output.contains(QStringLiteral("src=\"testHtml_files/pics/checkmark.png")));
  // verify relative location of entry link
  QVERIFY(output.contains(QStringLiteral("href=\"testHtml_files/Catching_Fire__The_Second_Book_of_the_Hunger_Games_-1.html")));
  // verify relative location of image file
  QVERIFY(output.contains(QStringLiteral("src=\"testHtml_files/17b54b2a742c6d342a75f122d615a793.jpeg")));
  // verify group heading
  QVERIFY(output.contains(QStringLiteral("<td class=\"groupName\" colspan=\"4\">Collins, Suzanne</td>")));
  // verify max image size
  QVERIFY(output.contains(QStringLiteral("height=\"250\"")));

  QVERIFY(exp.exec());
  QFile f(tempDirName + "/testHtml.html");
  QVERIFY(f.exists());
  QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));

  QTextStream in(&f);
  QString fileText = in.readAll();
  QVERIFY(fileText.contains(QStringLiteral("src=\"testHtml_files/tellico2html.js")));
  QVERIFY(fileText.contains(QStringLiteral("src=\"testHtml_files/pics/checkmark.png")));
  QVERIFY(fileText.contains(QStringLiteral("href=\"testHtml_files/Catching_Fire__The_Second_Book_of_the_Hunger_Games_-1.html")));
  QVERIFY(fileText.contains(QStringLiteral("src=\"testHtml_files/17b54b2a742c6d342a75f122d615a793.jpeg")));

  QVERIFY(QFile::exists(tempDirName + "/testHtml_files/tellico2html.js"));
  QVERIFY(QFile::exists(tempDirName + "/testHtml_files/pics/checkmark.png"));
  QVERIFY(QFile::exists(tempDirName + "/testHtml_files/17b54b2a742c6d342a75f122d615a793.jpeg"));

  // check entry html output
  QFile f2(tempDirName + "/testHtml_files/Catching_Fire__The_Second_Book_of_the_Hunger_Games_-1.html");
  QVERIFY(f2.exists());
  QVERIFY(f2.open(QIODevice::ReadOnly | QIODevice::Text));

  QTextStream in2(&f2);
  QString entryText = in2.readAll();
  // verify relative location of image file
  QVERIFY(entryText.contains(QStringLiteral("src=\"./17b54b2a742c6d342a75f122d615a793.jpeg")));
  // verify relative location of image pics
  QVERIFY(entryText.contains(QStringLiteral("src=\"pics/checkmark.png")));
  // verify link to parent html file
  QVERIFY(entryText.contains(QStringLiteral("href=\"../testHtml.html")));

  // sanity check, the directory should not exists after QTemporaryDir destruction
  tempDir.remove();
  QVERIFY(!QDir(tempDirName).exists());
}

void HtmlExporterTest::testHtmlTitle() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  coll->setTitle(QStringLiteral("Robby's Books"));

  Tellico::Data::EntryPtr e(new Tellico::Data::Entry(coll));
  coll->addEntries(e);

  Tellico::Export::HTMLExporter exporter(coll, QUrl());
  exporter.setEntries(coll->entries());

  QString output = exporter.text();
//  qDebug() << output;
  QVERIFY(!output.isEmpty());

  // check https://bugs.kde.org/show_bug.cgi?id=348381
  static const QRegularExpression rx(QStringLiteral("<title>.*</title>"));
  QRegularExpressionMatch match = rx.match(output);
  QVERIFY(match.hasMatch());
  QCOMPARE(match.captured(), QStringLiteral("<title>Robby's Books</title>"));
}

void HtmlExporterTest::testReportHtml() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  coll->setTitle(QStringLiteral("Robby's Books"));

  Tellico::Data::EntryPtr e(new Tellico::Data::Entry(coll));
  e->setField(QStringLiteral("title"), QStringLiteral("My Title"));
  e->setField(QStringLiteral("rating"), QStringLiteral("3"));
  coll->addEntries(e);

  Tellico::Export::HTMLExporter exporter(coll, QUrl());
  exporter.setXSLTFile(QFINDTESTDATA("../../xslt/report-templates/Column_View.xsl"));
  exporter.setEntries(coll->entries());

  QString output = exporter.text();
  QVERIFY(!output.isEmpty());

  // check that cdate is passed correctly
  static const QRegularExpression rx(QStringLiteral("div class=\"box header-right\"><span>(.*)</span"));
  QRegularExpressionMatch match = rx.match(output);
  QVERIFY(match.hasMatch());
  QCOMPARE(match.captured(1), QLocale().toString(QDate::currentDate()));

  // test image location in tmp directory
  Tellico::Export::HTMLExporter exporter2(coll, QUrl());
  exporter2.setXSLTFile(QFINDTESTDATA("../../xslt/report-templates/Image_List.xsl"));
  exporter2.setEntries(coll->entries());
  exporter2.setColumns(QStringList() << QStringLiteral("Title") << QStringLiteral("Rating"));

  QString output2 = exporter2.text();
  QVERIFY(!output2.isEmpty());
  QRegularExpression starsPathRx(QStringLiteral("src=\"(.+stars3.png)\""));
  auto starsMatch = starsPathRx.match(output2);
  QVERIFY(starsMatch.hasMatch());
  // the rating pic image should be an absolute local path
  // but that's only when Tellico is fully installed
  QString installPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("tellico/pics/stars3.png"));
  if(!installPath.isEmpty()) {
    QFileInfo starsInfo(starsMatch.captured(1));
    QVERIFY(starsInfo.isAbsolute());
  }
}

void HtmlExporterTest::testDirectoryNames() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));
  Tellico::Export::HTMLExporter exp(coll, QUrl());

  exp.setURL(QUrl::fromLocalFile(QDir::homePath() + QStringLiteral("/test.html")));
  QCOMPARE(exp.fileDir(), QUrl::fromLocalFile(QDir::homePath() + QStringLiteral("/test_files/")));
  QCOMPARE(exp.fileDirName(), QStringLiteral("test_files/"));

  // setCollectionUrl used when exporting entry files only
  exp.setCollectionURL(QUrl::fromLocalFile(QDir::homePath()));
  QCOMPARE(exp.fileDirName(), QStringLiteral("/"));
}

void HtmlExporterTest::testTemplatesTidy() {
  const QString tidy = QStandardPaths::findExecutable(QStringLiteral("tidy"));
  if(tidy.isEmpty()) {
    QSKIP("This test requires tidy", SkipAll);
  }

  QFETCH(QString, xsltFile);
  QFETCH(QString, tellicoFile);
  Tellico::ImageFactory::clean(true);

  QStringList tidyArgs = { QStringLiteral("-errors"),
                           QStringLiteral("-quiet"),
                           QStringLiteral("--show-warnings"),
                           QStringLiteral("no"),
                           QStringLiteral("--strict-tags-attributes"),
                           QStringLiteral("yes") };
  QProcessEnvironment tidyEnv;
  // suppress warning about no tidyrc file
  tidyEnv.insert(QStringLiteral("HTML_TIDY"), QStringLiteral("/dev/null"));

  QUrl url = QUrl::fromLocalFile(tellicoFile);
  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);

  Tellico::Export::HTMLExporter exporter(coll, url);
  exporter.setParseDOM(false); // shows error for <wbr> tags and is not necessary for check
  exporter.setEntries(coll->entries());
  exporter.setXSLTFile(xsltFile);
  exporter.setPrintGrouped(true);
  long opt = exporter.options();
  opt |= Tellico::Export::ExportComplete; // include loan info
  exporter.setOptions(opt);

  const QString output = exporter.text();
  QVERIFY(!output.contains(QStringLiteral("<p><p>")));
  QProcess tidyProc;
  tidyProc.setProcessEnvironment(tidyEnv);
  tidyProc.setProcessChannelMode(QProcess::SeparateChannels);
  tidyProc.setReadChannel(QProcess::StandardError);
  tidyProc.start(tidy, tidyArgs);
  QVERIFY(tidyProc.waitForStarted());

  tidyProc.write(output.toUtf8() + '\n');
  QVERIFY(tidyProc.waitForBytesWritten());
  tidyProc.closeWriteChannel();
  QVERIFY(tidyProc.waitForFinished());

  QTextStream ts(&tidyProc);
  QString errorOutput = ts.readLine();
  while(!errorOutput.isEmpty()) {
    qDebug() << errorOutput;
    errorOutput = ts.readLine();
  }

  tidyProc.close();
  QCOMPARE(tidyProc.exitStatus(), QProcess::NormalExit);
  // tidy exit codes are 0 for none, 1 for warnings only, 2 for errors
  QVERIFY(tidyProc.exitCode() < 2);
}

void HtmlExporterTest::testTemplatesTidy_data() {
  QTest::addColumn<QString>("xsltFile");
  QTest::addColumn<QString>("tellicoFile");

  QString ted = QFINDTESTDATA(QStringLiteral("data/ted_lasso.xml"));
  QString moody = QFINDTESTDATA(QStringLiteral("data/moody_blue.xml"));

  QDir entryDir(QFINDTESTDATA(QStringLiteral("../../xslt/entry-templates/Default.xsl")));
  entryDir.cdUp();
  foreach(const QString& file, entryDir.entryList({"*.xsl"}, QDir::Files)) {
    const QString test1 = file + QLatin1String(":ted");
    const QString test2 = file + QLatin1String(":moody");
    QTest::newRow(test1.toUtf8().constData()) << file << ted;
    QTest::newRow(test2.toUtf8().constData()) << file << moody;
  }
  QDir reportDir(QFINDTESTDATA(QStringLiteral("../../xslt/report-templates/Column_View.xsl")));
  reportDir.cdUp();
  foreach(const QString& file, reportDir.entryList({"*.xsl"}, QDir::Files)) {
    const QString test1 = file + QLatin1String(":ted");
    const QString test2 = file + QLatin1String(":moody");
    QTest::newRow(test1.toUtf8().constData()) << file << ted;
    QTest::newRow(test2.toUtf8().constData()) << file << moody;
  }
  QDir xsltDir(QFINDTESTDATA(QStringLiteral("../../xslt/tellico2html.xsl")));
  xsltDir.cdUp();
  foreach(const QString& file, xsltDir.entryList({"tellico*.xsl"}, QDir::Files)) {
    // only want selected of these templates
    if(file.contains(QLatin1String("tellico2gcstar")) ||
       file.contains(QLatin1String("tellico2onix")) ||
       file.contains(QLatin1String("tellico-common"))) continue;
    const QString test1 = file + QLatin1String(":ted");
    const QString test2 = file + QLatin1String(":moody");
    QTest::newRow(test1.toUtf8().constData()) << file << ted;
    QTest::newRow(test2.toUtf8().constData()) << file << moody;
  }
}

void HtmlExporterTest::testEntryTemplates() {
  QFETCH(QString, xsltFile);
  Tellico::ImageFactory::clean(true);

  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QStringLiteral("data/books-format11.bc")));
  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);

  Tellico::Export::HTMLExporter exporter(coll, url);
  exporter.setParseDOM(false); // shows error for <wbr> tags and is not necessary for check
  exporter.setEntries(coll->entries());
  exporter.setXSLTFile(xsltFile);
  exporter.setPrintGrouped(true);
  long opt = exporter.options();
  opt |= Tellico::Export::ExportComplete; // include loan info
  exporter.setOptions(opt);

  // check that the loan info appears in all generated HTML
  const QString output = exporter.text();
  // expect failure for the non-book templates since the file is a book collection
  if(xsltFile.contains(QStringLiteral("Album")) || xsltFile.contains(QStringLiteral("Video"))) {
    QEXPECT_FAIL("", "The test data is valid for a book collection only", Continue);
  }
  QVERIFY(output.contains(QStringLiteral("Robby")));
}

void HtmlExporterTest::testEntryTemplates_data() {
  QTest::addColumn<QString>("xsltFile");

  QDir entryDir(QFINDTESTDATA(QStringLiteral("../../xslt/entry-templates/Default.xsl")));
  entryDir.cdUp();
  foreach(const QString& file, entryDir.entryList({"*.xsl"}, QDir::Files)) {
    QTest::newRow(file.toUtf8().constData()) << file;
  }
}

void HtmlExporterTest::testPrinting() {
  auto groupModel = new Tellico::EntryGroupModel(this);
  Tellico::ModelManager::self()->setGroupModel(groupModel);

  Tellico::Config::setImageLocation(Tellico::Config::ImagesInFile);
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QStringLiteral("data/with-image.tc")));
  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);

  // mimic PrintHandler::generateHtml()
  Tellico::Export::HTMLExporter exporter(coll, url);
  exporter.setEntries(coll->entries());
  exporter.setXSLTFile(QFINDTESTDATA(QStringLiteral("../../xslt/tellico-printing.xsl")));
  exporter.setPrintHeaders(true);
  exporter.setPrintGrouped(true);
  exporter.setGroupBy({ QStringLiteral("author") });
  exporter.setColumns({ QStringLiteral("Title"),
                        QStringLiteral("Gift"),
                        QStringLiteral("Rating"),
                        QStringLiteral("Front Cover")});
  exporter.setMaxImageSize(100, 100);
  exporter.setOptions(Tellico::Export::ExportUTF8 | Tellico::Export::ExportFormatted);

  const QString output = exporter.text();
  QVERIFY(!output.isEmpty());

  // verify relative location of image pics
  QVERIFY(output.contains(QStringLiteral("pics/checkmark.png")));
  // verify relative location of image file
  QVERIFY(output.contains(QStringLiteral("/17b54b2a742c6d342a75f122d615a793.jpeg")));
  // verify max image size
  QVERIFY(output.contains(QStringLiteral("height=\"100\"")));
}
