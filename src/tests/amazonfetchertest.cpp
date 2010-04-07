/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#include "qtest_kde.h"
#include "amazonfetchertest.h"
#include "amazonfetchertest.moc"

#include "../fetch/fetcherjob.h"
#include "../fetch/amazonfetcher.h"
#include "../collections/bookcollection.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include  "../images/imagefactory.h"

#include <KStandardDirs>
#include <KConfigGroup>

QTEST_KDEMAIN( AmazonFetcherTest, GUI )

AmazonFetcherTest::AmazonFetcherTest() : m_loop(this), m_hasConfigFile(false)
    , m_config(QString::fromLatin1(KDESRCDIR)  + "/tellicotestconfig.rc", KConfig::SimpleConfig) {
}

void AmazonFetcherTest::initTestCase() {
  Tellico::RegisterCollection<Tellico::Data::BookCollection> registerBook(Tellico::Data::Collection::Book, "book");
  // since we use an XSL file
  KGlobal::dirs()->addResourceDir("appdata", QString::fromLatin1(KDESRCDIR) + "/../../xslt/");
  Tellico::ImageFactory::init();

  m_hasConfigFile = QFile::exists(QString::fromLatin1(KDESRCDIR)  + "/tellicotestconfig.rc");

  m_fieldValues.insert(QLatin1String("title"), QLatin1String("Practical RDF"));
  m_fieldValues.insert(QLatin1String("isbn"), QLatin1String("0-596-00263-7"));
  m_fieldValues.insert(QLatin1String("author"), QLatin1String("Shelley Powers"));
  m_fieldValues.insert(QLatin1String("binding"), QLatin1String("Paperback"));
  m_fieldValues.insert(QLatin1String("publisher"), QLatin1String("O'reilly Media"));
  m_fieldValues.insert(QLatin1String("pages"), QLatin1String("331"));
}

void AmazonFetcherTest::testTitle() {
  QFETCH(QString, locale);
  QFETCH(int, collType);
  QFETCH(QString, searchValue);

  QString groupName = QLatin1String("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  Tellico::Fetch::FetchRequest request(collType, Tellico::Fetch::Title, searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QVERIFY(!m_results.isEmpty());

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void AmazonFetcherTest::testTitle_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<int>("collType");
  QTest::addColumn<QString>("searchValue");

  QTest::newRow("US book title") << QString::fromLatin1("US")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QString::fromLatin1("Practical RDF");
  QTest::newRow("UK book title") << QString::fromLatin1("UK")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QString::fromLatin1("Practical RDF");
//  QTest::newRow("DE") << QString::fromLatin1("DE");
//  QTest::newRow("JP") << QString::fromLatin1("JP");
//  QTest::newRow("FR") << QString::fromLatin1("FR");
  QTest::newRow("CA book title") << QString::fromLatin1("CA")
                                 << static_cast<int>(Tellico::Data::Collection::Book)
                                 << QString::fromLatin1("Practical RDF");
}

void AmazonFetcherTest::testIsbn() {
  QFETCH(QString, locale);
  QFETCH(QString, searchValue);

  QString groupName = QLatin1String("Amazon ") + locale;
  if(!m_hasConfigFile || !m_config.hasGroup(groupName)) {
    QSKIP("This test requires a config file with Amazon settings.", SkipAll);
  }
  KConfigGroup cg(&m_config, groupName);

  // also testing multiple values
  Tellico::Fetch::FetchRequest request(Tellico::Data::Collection::Book, Tellico::Fetch::ISBN,
                                       searchValue);
  Tellico::Fetch::Fetcher::Ptr fetcher(new Tellico::Fetch::AmazonFetcher(this));
  fetcher->readConfig(cg, cg.name());

  // don't use 'this' as job parent, it crashes
  Tellico::Fetch::FetcherJob* job = new Tellico::Fetch::FetcherJob(0, fetcher, request);
  connect(job, SIGNAL(result(KJob*)), this, SLOT(slotResult(KJob*)));

  job->start();
  m_loop.exec();

  QCOMPARE(m_results.size(), 2);

  Tellico::Data::EntryPtr entry = m_results.at(0);
  QHashIterator<QString, QString> i(m_fieldValues);
  while(i.hasNext()) {
    i.next();
    QString result = entry->field(i.key()).toLower();
    QCOMPARE(result, i.value().toLower());
  }
  QVERIFY(!entry->field(QLatin1String("cover")).isEmpty());
}

void AmazonFetcherTest::testIsbn_data() {
  QTest::addColumn<QString>("locale");
  QTest::addColumn<QString>("searchValue");

  QTest::newRow("US isbn") << QString::fromLatin1("US") << QString::fromLatin1("0-596-00263-7; 978-1-59059-831-3");
  QTest::newRow("UK isbn") << QString::fromLatin1("UK") << QString::fromLatin1("0-596-00263-7; 978-1-59059-831-3");
//  QTest::newRow("DE") << QString::fromLatin1("DE");
//  QTest::newRow("JP") << QString::fromLatin1("JP");
//  QTest::newRow("FR") << QString::fromLatin1("FR");
  QTest::newRow("CA isbn") << QString::fromLatin1("CA") << QString::fromLatin1("0-596-00263-7; 978-1-59059-831-3");
}

void AmazonFetcherTest::slotResult(KJob* job_) {
  m_results = static_cast<Tellico::Fetch::FetcherJob*>(job_)->entries();
  m_loop.quit();
}
