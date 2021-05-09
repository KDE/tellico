/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "tellicoreadtest.h"

#include "../translators/tellicoimporter.h"
#include "../collections/bookcollection.h"
#include "../collections/bibtexcollection.h"
#include "../collections/coincollection.h"
#include "../collections/musiccollection.h"
#include "../collectionfactory.h"
#include "../translators/tellicoxmlexporter.h"
#include "../translators/tellico_xml.h"
#include "../translators/xslthandler.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../fieldformat.h"
#include "../entry.h"
#include "../document.h"
#include "../utils/xmlhandler.h"

#include <QTest>
#include <QNetworkInterface>
#include <QDate>
#include <QTextCodec>
#include <QStandardPaths>

QTEST_GUILESS_MAIN( TellicoReadTest )

#define QSL(x) QStringLiteral(x)
#define TELLICOREAD_NUMBER_OF_CASES 11

static bool hasNetwork() {
#ifdef ENABLE_NETWORK_TESTS
  foreach(const QNetworkInterface& net, QNetworkInterface::allInterfaces()) {
    if(net.flags().testFlag(QNetworkInterface::IsUp) && !net.flags().testFlag(QNetworkInterface::IsLoopBack)) {
      return true;
    }
  }
#endif
  return false;
}

void TellicoReadTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  // need to register this first
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  Tellico::RegisterCollection<Tellico::Data::BibtexCollection> registerBibtex(Tellico::Data::Collection::Bibtex, "bibtex");
  Tellico::RegisterCollection<Tellico::Data::CoinCollection> registerCoin(Tellico::Data::Collection::Coin, "coin");
  Tellico::RegisterCollection<Tellico::Data::Collection> registerBase(Tellico::Data::Collection::Base, "entry");
  Tellico::RegisterCollection<Tellico::Data::MusicCollection> registerAlbum(Tellico::Data::Collection::Album, "album");

  for(int i = 1; i <= TELLICOREAD_NUMBER_OF_CASES; ++i) {
    QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QSL("data/books-format%1.bc").arg(i)));

    Tellico::Import::TellicoImporter importer(url);
    Tellico::Data::CollPtr coll = importer.collection();
    m_collections.append(coll);
  }
  Tellico::ImageFactory::init();
}

void TellicoReadTest::init() {
  Tellico::ImageFactory::clean(true);
}

void TellicoReadTest::testBookCollection() {
  Tellico::Data::CollPtr coll1 = m_collections[0];
  // skip the first one
  for(int i = 1; i < m_collections.count(); ++i) {
    Tellico::Data::CollPtr coll2 = m_collections[i];
    QVERIFY(coll2);
    QCOMPARE(coll1->type(), coll2->type());
    QCOMPARE(coll1->title(), coll2->title());
    QCOMPARE(coll1->entryCount(), coll2->entryCount());
  }
}

void TellicoReadTest::testEntries() {
  QFETCH(QString, fieldName);

  Tellico::Data::FieldPtr field1 = m_collections[0]->fieldByName(fieldName);

  // skip the first one
  for(int i = 1; i < m_collections.count(); ++i) {
    Tellico::Data::FieldPtr field2 = m_collections[i]->fieldByName(fieldName);
    if(field1 && field2) {
      QCOMPARE(field1->name(), field2->name());
      QCOMPARE(field1->title(), field2->title());
      QCOMPARE(field1->category(), field2->category());
      QCOMPARE(field1->type(), field2->type());
      QCOMPARE(field1->flags(), field2->flags());
      QCOMPARE(field1->propertyList(), field2->propertyList());
    }

    for(int j = 0; j < m_collections[0]->entryCount(); ++j) {
      // don't test id values since the initial value has changed from 0 to 1
      Tellico::Data::EntryPtr entry1 = m_collections[0]->entries().at(j);
      Tellico::Data::EntryPtr entry2 = m_collections[i]->entries().at(j);
      QVERIFY(entry1);
      QVERIFY(entry2);
      QCOMPARE(entry1->field(fieldName), entry2->field(fieldName));
    }
  }
}

void TellicoReadTest::testEntries_data() {
  QTest::addColumn<QString>("fieldName");

  QTest::newRow("title") << QSL("title");
  QTest::newRow("author") << QSL("author");
  QTest::newRow("publisher") << QSL("publisher");
  QTest::newRow("keywords") << QSL("keywords");
  QTest::newRow("keyword") << QSL("keyword");
  QTest::newRow("genre") << QSL("genre");
  QTest::newRow("isbn") << QSL("isbn");
  QTest::newRow("pub_year") << QSL("pub_year");
  QTest::newRow("rating") << QSL("rating");
  QTest::newRow("comments") << QSL("comments");
}

void TellicoReadTest::testCoinCollection() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/coins-format9.tc"));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Coin);

  Tellico::Data::FieldPtr field = coll->fieldByName(QStringLiteral("title"));
  // old field has Dependent value, now is Line
  QVERIFY(field);
  QCOMPARE(field->type(), Tellico::Data::Field::Line);
  QCOMPARE(field->title(), QSL("Title"));
  QVERIFY(field->hasFlag(Tellico::Data::Field::Derived));

  Tellico::Data::EntryPtr entry = coll->entries().at(0);
  // test creating the derived title
  QCOMPARE(entry->title(), QSL("1974D Jefferson Nickel 0.05"));
}

void TellicoReadTest::testBibtexCollection() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("data/bibtex-format11.tc"));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();
  Tellico::Data::BibtexCollection* bColl = dynamic_cast<Tellico::Data::BibtexCollection*>(coll.data());

  QVERIFY(coll);
  QCOMPARE(coll->type(), Tellico::Data::Collection::Bibtex);
  QVERIFY(bColl);
  QVERIFY(!bColl->preamble().isEmpty());

  auto macroList = bColl->macroList();
  QCOMPARE(macroList.count(), 13); // includes 12 months plus the one in the file
  QVERIFY(!macroList.value(QLatin1String("SPE")).isEmpty());

  auto borrowerList = coll->borrowers();
  QCOMPARE(borrowerList.count(), 1);
  auto borr1 = borrowerList.front();
  QCOMPARE(borr1->count(), 1);
  QCOMPARE(borr1->name(), QStringLiteral("кириллица"));

  auto filterList = coll->filters();
  QCOMPARE(filterList.count(), 1);
  auto filter1 = filterList.front();
  QCOMPARE(filter1->name(), QStringLiteral("1990"));

  Tellico::Export::TellicoXMLExporter exporter(coll);
  exporter.setEntries(coll->entries());
  exporter.setOptions(exporter.options() | Tellico::Export::ExportComplete);
  Tellico::Import::TellicoImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();
  Tellico::Data::BibtexCollection* bColl2 = dynamic_cast<Tellico::Data::BibtexCollection*>(coll2.data());

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());
  QVERIFY(bColl2);
  QCOMPARE(bColl2->preamble(), bColl->preamble());
  QCOMPARE(bColl2->macroList(), bColl->macroList());

  QCOMPARE(coll2->filters().count(), coll->filters().count());
  auto filter2 = coll->filters().front();
  QCOMPARE(filter1->name(), filter2->name());
  QCOMPARE(filter1->count(), filter2->count());
  QCOMPARE(filter1->op(), filter2->op());

  QCOMPARE(coll2->borrowers().count(), coll->borrowers().count());
  auto borr2 = coll2->borrowers().front();
  QCOMPARE(borr1->name(), borr2->name());
  QCOMPARE(borr1->uid(), borr2->uid());
  QCOMPARE(borr1->count(), borr2->count());
  auto loan1 = borr1->loans().front();
  auto loan2 = borr2->loans().front();
  QCOMPARE(loan1->loanDate(), loan2->loanDate());
  QCOMPARE(loan1->borrower()->name(), borr1->name());
  QCOMPARE(loan1->dueDate(), loan2->dueDate());
  QCOMPARE(loan1->note(), loan2->note());
  QCOMPARE(loan1->uid(), loan2->uid());
  QCOMPARE(loan1->entry()->title(), loan2->entry()->title());
}

void TellicoReadTest::testTableData() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("/data/tabletest.tc"));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QCOMPARE(coll->entryCount(), 3);

  Tellico::Export::TellicoXMLExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::TellicoImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();

  QVERIFY(coll2);
  QCOMPARE(coll2->type(), coll->type());
  QCOMPARE(coll2->entryCount(), coll->entryCount());

  foreach(Tellico::Data::EntryPtr e1, coll->entries()) {
    Tellico::Data::EntryPtr e2 = coll2->entryById(e1->id());
    QVERIFY(e2);
    foreach(Tellico::Data::FieldPtr f, coll->fields()) {
      QCOMPARE(f->name() + e1->field(f), f->name() + e2->field(f));
    }
  }

  // test table value concatenation
  Tellico::Data::EntryPtr e3(new Tellico::Data::Entry(coll));
  coll->addEntries(e3);
  QString value = QSL("11a") + Tellico::FieldFormat::delimiterString() + QSL("11b")
                + Tellico::FieldFormat::columnDelimiterString() + QSL("12")
                + Tellico::FieldFormat::columnDelimiterString() + QSL("13")
                + Tellico::FieldFormat::rowDelimiterString() + QSL("21")
                + Tellico::FieldFormat::columnDelimiterString() + QSL("22")
                + Tellico::FieldFormat::columnDelimiterString() + QSL("23");
  e3->setField(QSL("table"), value);
  QStringList groups = e3->groupNamesByFieldName(QStringLiteral("table"));
  QCOMPARE(groups.count(), 3);
  // the order of the group names is not stable (it uses QSet::toList)
  QCOMPARE(groups.size(), 3);
  QVERIFY(groups.contains(QSL("11a")));
  QVERIFY(groups.contains(QSL("11b")));
  QVERIFY(groups.contains(QSL("21")));

  // test having empty value in table
  Tellico::Data::EntryPtr e = coll2->entryById(2);
  QVERIFY(e);
  const QStringList rows = Tellico::FieldFormat::splitTable(e->field(QSL("table")));
  QCOMPARE(rows.count(), 1);
  const QStringList cols = Tellico::FieldFormat::splitRow(rows.at(0));
  QCOMPARE(cols.count(), 3);
}

void TellicoReadTest::testDuplicateLoans() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("/data/duplicate_loan.xml"));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);

  QCOMPARE(coll->borrowers().count(), 1);

  Tellico::Data::BorrowerPtr bor = coll->borrowers().first();
  QVERIFY(bor);

  QCOMPARE(bor->loans().count(), 1);
}

void TellicoReadTest::testDuplicateBorrowers() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("/data/duplicate_borrower.xml"));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);

  QCOMPARE(coll->borrowers().count(), 1);

  Tellico::Data::BorrowerPtr bor = coll->borrowers().first();
  QVERIFY(bor);

  QCOMPARE(bor->loans().count(), 2);
}

void TellicoReadTest::testLocalImage() {
  // this is the md5 hash of the tellico.png icon, used as an image id
  const QString imageId(QSL("dde5bf2cbd90fad8635a26dfb362e0ff.png"));
  // not yet loaded
  QVERIFY(!Tellico::ImageFactory::self()->hasImageInMemory(imageId));
  QVERIFY(!Tellico::ImageFactory::self()->hasImageInfo(imageId));

  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("/data/local_image.xml"));
  QFile f(url.toLocalFile());
  QVERIFY(f.exists());
  QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));

  QTextStream in(&f);
  QString fileText = in.readAll();
  // replace %COVER% with image file location
  fileText.replace(QSL("%COVER%"),
                   QFINDTESTDATA("../../icons/tellico.png"));

  Tellico::Import::TellicoImporter importer(fileText);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->entries().count(), 1);

  Tellico::Data::EntryPtr entry = coll->entries().at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("cover")), imageId);

  // the image should be in local memory now
  QVERIFY(Tellico::ImageFactory::self()->hasImageInMemory(imageId));
  QVERIFY(Tellico::ImageFactory::self()->hasImageInfo(imageId));

  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(imageId);
  QVERIFY(!img.isNull());
}

void TellicoReadTest::testRemoteImage() {
  if(!hasNetwork()) QSKIP("This test requires network access", SkipSingle);

  // this is the md5 hash of the logo.png icon, used as an image id
  const QString imageId(QSL("ecaf5185c4016881aaabb4933211d5d6.png"));
  // not yet loaded
  QVERIFY(!Tellico::ImageFactory::self()->hasImageInMemory(imageId));
  QVERIFY(!Tellico::ImageFactory::self()->hasImageInfo(imageId));

  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA("/data/local_image.xml"));
  QFile f(url.toLocalFile());
  QVERIFY(f.exists());
  QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));

  QTextStream in(&f);
  QString fileText = in.readAll();
  // replace %COVER% with image file location
  fileText.replace(QSL("%COVER%"),
                   QSL("https://tellico-project.org/wp-content/uploads/96-tellico.png"));

  Tellico::Import::TellicoImporter importer(fileText);
  Tellico::Data::CollPtr coll = importer.collection();
  QVERIFY(coll);
  QCOMPARE(coll->entries().count(), 1);

  Tellico::Data::EntryPtr entry = coll->entries().at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("cover")), imageId);

  // the image should be in local memory now
  QVERIFY(Tellico::ImageFactory::self()->hasImageInMemory(imageId));
  QVERIFY(Tellico::ImageFactory::self()->hasImageInfo(imageId));

  const Tellico::Data::Image& img = Tellico::ImageFactory::imageById(imageId);
  QVERIFY(!img.isNull());
}

void TellicoReadTest::testXMLHandler() {
  QFETCH(QByteArray, data);
  QFETCH(QString, expectedString);
  QFETCH(bool, changeEncoding);

  QString origString = QString::fromUtf8(data);
  QCOMPARE(Tellico::XMLHandler::readXMLData(data), expectedString);
  QCOMPARE(Tellico::XMLHandler::setUtf8XmlEncoding(origString), changeEncoding);
}

void TellicoReadTest::testXMLHandler_data() {
  QTest::addColumn<QByteArray>("data");
  QTest::addColumn<QString>("expectedString");
  QTest::addColumn<bool>("changeEncoding");

  QTest::newRow("basic") << QByteArray("<x>value</x>") << QStringLiteral("<x>value</x>") << false;
  QTest::newRow("utf8") << QByteArray("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<x>value</x>")
                        << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<x>value</x>") << false;
  QTest::newRow("UTF8") << QByteArray("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<x>value</x>")
                        << QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<x>value</x>") << false;
  QTest::newRow("latin1") << QByteArray("<?xml version=\"1.0\" encoding=\"latin1\"?>\n<x>value</x>")
                          << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<x>value</x>") << true;
  QTest::newRow("LATIN1") << QByteArray("<?xml version=\"1.0\" encoding=\"LATIN1\"?>\n<x>value</x>")
                          << QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<x>value</x>") << true;

  QString usa = QString::fromUtf8("США");
  QTextCodec* cp1251 = QTextCodec::codecForName("cp1251");
  QByteArray usaBytes = QByteArray("<?xml version=\"1.0\" encoding=\"cp1251\"?>\n<x>")
                      + cp1251->fromUnicode(usa)
                      + QByteArray("</x>");
  QString usaString = QStringLiteral("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<x>")
                    + usa
                    + QStringLiteral("</x>");
  QTest::newRow("cp1251") << usaBytes << usaString << true;
}

void TellicoReadTest::testXmlName() {
  QFETCH(bool, valid);
  QFETCH(QString, input);
  QFETCH(QString, modified);

  QCOMPARE(Tellico::XML::validXMLElementName(input), valid);
  QCOMPARE(Tellico::XML::elementName(input), modified);
}

void TellicoReadTest::testXmlName_data() {
  QTest::addColumn<bool>("valid");
  QTest::addColumn<QString>("input");
  QTest::addColumn<QString>("modified");

  QTest::newRow("start")  << true  << QSL("start")  << QSL("start");
  QTest::newRow("_start") << true  << QSL("_start") << QSL("_start");
  QTest::newRow("n42")    << true  << QSL("n42")    << QSL("n42");
  // an empty string is handled in CollectionFieldsDialog when creating the field name
  QTest::newRow("42")     << false << QSL("42")     << QString();
  QTest::newRow("she is") << false << QSL("she is") << QSL("she-is");
  QTest::newRow("colon:") << false << QSL("colon:") << QSL("colon");
  QTest::newRow("Svět")   << true  << QSL("Svět")   << QSL("Svět");
  QTest::newRow("<test>") << false << QSL("<test>") << QSL("test");
  QTest::newRow("is-€:")  << false << QSL("is-€:")  << QSL("is-");
}

void TellicoReadTest::testRecoverXmlName() {
  QFETCH(QByteArray, input);
  QFETCH(QByteArray, modified);

  QCOMPARE(Tellico::XML::recoverFromBadXMLName(input), modified);
}

void TellicoReadTest::testRecoverXmlName_data() {
  QTest::addColumn<QByteArray>("input");
  QTest::addColumn<QByteArray>("modified");

  QTest::newRow("<nr:>")   << QByteArray("<fields><field name=\"nr:\"/></fields><nr:>x</nr:>")
                           << QByteArray("<fields><field name=\"nr\"/></fields><nr>x</nr>");
  QTest::newRow("<nr:>2")  << QByteArray("<fields><field name=\"nr:\" d=\"d\"/></fields><nr:>x</nr:>")
                           << QByteArray("<fields><field name=\"nr\" d=\"d\"/></fields><nr>x</nr>");
  QTest::newRow("<nr:>3")  << QByteArray("<fields><field name=\"nr:\"/></fields><nr:s><nr:>x</nr:></nr:s>")
                           << QByteArray("<fields><field name=\"nr\"/></fields><nrs><nr>x</nr></nrs>");
  QTest::newRow("<nr:>4")  << QByteArray("<fields><field d=\"nr:\" name=\"nr:\" d=\"nr:\"/></fields><nr:>x</nr:>")
                           << QByteArray("<fields><field d=\"nr:\" name=\"nr\" d=\"nr:\"/></fields><nr>x</nr>");
  QTest::newRow("<is-€:>") << QByteArray("<fields><field name=\"is-€:\"/></fields><is-€:>x</is-€:>")
                           << QByteArray("<fields><field name=\"is-\"/></fields><is->x</is->");
}

void TellicoReadTest::testBug418067() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QSL("data/bug418067.xml")));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QVERIFY(coll->hasField(QSL("lc-no.")));
  QVERIFY(coll->hasField(QSL("mein-wunschpreis-")));
}

void TellicoReadTest::testNoCreationDate() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QSL("data/no_cdate.xml")));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QVERIFY(coll->hasField(QStringLiteral("cdate")));
  QVERIFY(coll->hasField(QStringLiteral("mdate")));
  QCOMPARE(coll->entries().count(), 1);

  Tellico::Data::EntryPtr entry = coll->entries().at(0);
  QVERIFY(entry);
  // entry data has an mdate but no cdate
  // cdate should be set to same as mdate
  QString mdate(QStringLiteral("2020-05-30"));
  QCOMPARE(entry->field(QStringLiteral("cdate")), mdate);
  QCOMPARE(entry->field(QStringLiteral("mdate")), mdate);
}

void TellicoReadTest::testFutureVersion() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QSL("data/future_version.xml")));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(!coll);
  QVERIFY(!importer.statusMessage().isEmpty());
}

void TellicoReadTest::testRelativeLink() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QSL("data/relative-link.xml")));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QVERIFY(coll->hasField(QStringLiteral("url")));
  Tellico::Data::EntryPtr entry = coll->entries().at(0);
  QVERIFY(entry);
  QCOMPARE(entry->field(QStringLiteral("url")), QLatin1String("collectorz/image.png"));

  Tellico::XSLTHandler handler(QFile::encodeName(QFINDTESTDATA("data/output-url.xsl")));
  QVERIFY(handler.isValid());

  Tellico::Data::Document::self()->setURL(url); // set the base url
  QUrl expected = url.resolved(QUrl(QLatin1String("collectorz/image.png")));

  Tellico::Export::TellicoXMLExporter exp(coll);
  exp.setEntries(coll->entries());
  QString output = handler.applyStylesheet(exp.text());
  // first, the link should remain completely relative
  QVERIFY(output.contains(QLatin1String("href=\"collectorz/image.png")));

  exp.setOptions(exp.options() | Tellico::Export::ExportAbsoluteLinks);
  output = handler.applyStylesheet(exp.text());
  // now, the link should be absolute
  QVERIFY(output.contains(expected.url()));
}

void TellicoReadTest::testEmptyFirstTableRow() {
  QUrl url = QUrl::fromLocalFile(QFINDTESTDATA(QSL("data/table-empty-first-row.xml")));

  Tellico::Import::TellicoImporter importer(url);
  Tellico::Data::CollPtr coll = importer.collection();

  QVERIFY(coll);
  QVERIFY(coll->hasField(QStringLiteral("table")));
  Tellico::Data::EntryPtr entry = coll->entries().at(0);
  QVERIFY(entry);
  const QStringList rows = Tellico::FieldFormat::splitTable(entry->field(QSL("table")));
  QCOMPARE(rows.count(), 2);

  Tellico::Export::TellicoXMLExporter exporter(coll);
  exporter.setEntries(coll->entries());
  Tellico::Import::TellicoImporter importer2(exporter.text());
  Tellico::Data::CollPtr coll2 = importer2.collection();
  QVERIFY(coll2);
  Tellico::Data::EntryPtr entry2 = coll2->entries().at(0);
  QVERIFY(entry2);
  const QStringList rows2 = Tellico::FieldFormat::splitTable(entry2->field(QSL("table")));
  QCOMPARE(rows2.count(), 2);
}
