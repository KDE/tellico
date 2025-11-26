/***************************************************************************
    Copyright (C) 2025 Robby Stephenson <robby@periapsis.org>
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

#include "xsltexportertest.h"

#include "../translators/xsltexporter.h"
#include "../collections/bookcollection.h"

#include <KLocalizedString>

#include <QTest>
#include <QTemporaryFile>
#include <QLoggingCategory>

QTEST_GUILESS_MAIN( XSLTExporterTest )

void XSLTExporterTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
  QLoggingCategory::setFilterRules(QStringLiteral("tellico.debug = true\ntellico.info = false"));
}

// general XSLT export test
void XSLTExporterTest::testXSLTExport() {
  Tellico::Data::CollPtr coll(new Tellico::Data::BookCollection(true));

  QTemporaryFile tmpFile;
  QVERIFY(tmpFile.open());

  Tellico::Export::XSLTExporter exporter(coll, QUrl());
  exporter.setURL(QUrl::fromLocalFile(tmpFile.fileName()));
  long opt = exporter.options();
  opt |= Tellico::Export::ExportForce;
  opt &= ~Tellico::Export::ExportUTF8; // Bug 512581
  exporter.setOptions(opt);
  exporter.m_xsltFile = QUrl::fromLocalFile(QFINDTESTDATA("../../xslt/entry-templates/Default.xsl"));
  QVERIFY(exporter.exec());
}
