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

#include "citetest.h"
#include "../cite/lyxpipe.h"
#include "../cite/clipboard.h"
#include "../config/tellico_config.h"
#include "../collections/bibtexcollection.h"

#include <KLocalizedString>

#include <QTest>
#include <QTemporaryFile>
#include <QApplication>
#include <QClipboard>

QTEST_MAIN( CiteTest )

void CiteTest::initTestCase() {
  KLocalizedString::setApplicationDomain("tellico");
}

void CiteTest::testLyxpipe() {
  QTemporaryFile tempFile(QStringLiteral("citetest.XXXXXX.in"));
  QVERIFY(tempFile.open());
  // remove ".in" that gets added by Lyxpipe
  Tellico::Config::setLyxpipe(tempFile.fileName().remove(QStringLiteral(".in")));

  Tellico::Cite::Lyxpipe pipe;
  QVERIFY(!pipe.hasError());
  QVERIFY(pipe.errorString().isEmpty());
  QCOMPARE(pipe.type(), Tellico::Cite::CiteLyxpipe);

  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("Title 1"));
  entry1->setField(QStringLiteral("bibtex-key"), QStringLiteral("title1"));
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  entry2->setField(QStringLiteral("title"), QStringLiteral("Title 2"));
  entry2->setField(QStringLiteral("bibtex-key"), QStringLiteral("title2"));

  coll->addEntries(Tellico::Data::EntryList() << entry1 << entry2);

  QVERIFY(pipe.cite(coll->entries()));
  QVERIFY(!pipe.hasError());
  QVERIFY(pipe.errorString().isEmpty());

  // read and verify file contents
  QTextStream ts(&tempFile);
  QString text = ts.readAll();
  QCOMPARE(text, QStringLiteral("LYXCMD:tellico:citation-insert:title1, title2\n"));
}

void CiteTest::testLyxpipeNotExists() {
  Tellico::Cite::Lyxpipe pipe;

  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("Title 1"));
  entry1->setField(QStringLiteral("bibtex-key"), QStringLiteral("title1"));
  coll->addEntries(Tellico::Data::EntryList() << entry1);

  // cite should fail with an error
  QVERIFY(!pipe.cite(coll->entries()));
  QVERIFY(pipe.hasError());
  QVERIFY(!pipe.errorString().isEmpty());
}

void CiteTest::testClipboard() {
  Tellico::Cite::Clipboard clip;
  QVERIFY(!clip.hasError());
  QVERIFY(clip.errorString().isEmpty());
  QCOMPARE(clip.type(), Tellico::Cite::CiteClipboard);

  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QStringLiteral("title"), QStringLiteral("Title 1"));
  entry1->setField(QStringLiteral("bibtex-key"), QStringLiteral("title1"));
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  entry2->setField(QStringLiteral("title"), QStringLiteral("Title 2"));
  entry2->setField(QStringLiteral("bibtex-key"), QStringLiteral("title2"));

  coll->addEntries(Tellico::Data::EntryList() << entry1 << entry2);

  QVERIFY(clip.cite(coll->entries()));
  QVERIFY(!clip.hasError());
  QVERIFY(clip.errorString().isEmpty());

  // read and verify clipboard contents
  QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("\\cite{title1, title2}"));
}

void CiteTest::testManager() {
  auto mgr = Tellico::Cite::ActionManager::self();
  QVERIFY(mgr);
  // empty list has flse result
  QCOMPARE(mgr->cite(Tellico::Cite::CiteClipboard, {}), false);
  QVERIFY(!mgr->hasError());

  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  coll->addEntries(Tellico::Data::EntryList() << entry1);
  QCOMPARE(mgr->cite(Tellico::Cite::CiteClipboard, coll->entries()), true);
  QVERIFY(!mgr->hasError());
  QVERIFY(mgr->errorString().isEmpty());
}
