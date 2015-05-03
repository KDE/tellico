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

#undef QT_NO_CAST_FROM_ASCII

#include "lyxpipetest.h"
#include "../cite/lyxpipe.h"
#include "../core/tellico_config.h"
#include "../collections/bibtexcollection.h"

#include <QTest>
#include <QTemporaryFile>

QTEST_GUILESS_MAIN( LyxpipeTest )

void LyxpipeTest::testLyxpipe() {
  QTemporaryFile tempFile(QLatin1String("lyxpipetest.XXXXXX.in"));
  QVERIFY(tempFile.open());
  // remove ".in" that gets added by Lyxpipe
  Tellico::Config::setLyxpipe(tempFile.fileName().remove(QLatin1String(".in")));

  Tellico::Cite::Lyxpipe pipe;
  QVERIFY(!pipe.hasError());
  QVERIFY(pipe.errorString().isEmpty());

  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));

  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QLatin1String("title"), QLatin1String("Title 1"));
  entry1->setField(QLatin1String("bibtex-key"), QLatin1String("title1"));
  Tellico::Data::EntryPtr entry2(new Tellico::Data::Entry(coll));
  entry2->setField(QLatin1String("title"), QLatin1String("Title 2"));
  entry2->setField(QLatin1String("bibtex-key"), QLatin1String("title2"));

  coll->addEntries(Tellico::Data::EntryList() << entry1 << entry2);

  QVERIFY(pipe.cite(coll->entries()));
  QVERIFY(!pipe.hasError());
  QVERIFY(pipe.errorString().isEmpty());

  // read and verify file contents
  QTextStream ts(&tempFile);
  QString text = ts.readAll();
  QCOMPARE(text, QLatin1String("LYXCMD:tellico:citation-insert:title1, title2\n"));
}

void LyxpipeTest::testLyxpipeNotExists() {
  QTemporaryFile tempFile(QLatin1String("lyxpipetest.XXXXXX.in"));
  // do not open/create the tempfile
  //QVERIFY(tempFile.open());

  Tellico::Cite::Lyxpipe pipe;

  Tellico::Data::CollPtr coll(new Tellico::Data::BibtexCollection(true));
  Tellico::Data::EntryPtr entry1(new Tellico::Data::Entry(coll));
  entry1->setField(QLatin1String("title"), QLatin1String("Title 1"));
  entry1->setField(QLatin1String("bibtex-key"), QLatin1String("title1"));
  coll->addEntries(Tellico::Data::EntryList() << entry1);

  // cite should fail with an error
  QVERIFY(!pipe.cite(coll->entries()));
  QVERIFY(pipe.hasError());
  QVERIFY(!pipe.errorString().isEmpty());
}

