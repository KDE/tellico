// SPDX-FileCopyrightText: 2026 The Tellico contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "collectionfieldsdialogtest.h"

#include "../collection.h"
#include "../collectionfieldsdialog.h"
#include "../field.h"

#include <KLocalizedString>

#include <QListWidget>
#include <QStandardPaths>
#include <QTest>

QTEST_MAIN(CollectionFieldsDialogTest)

void CollectionFieldsDialogTest::initTestCase() {
  QStandardPaths::setTestModeEnabled(true);
  KLocalizedString::setApplicationDomain("tellico");
}

void CollectionFieldsDialogTest::testDeleteLastField() {
  Tellico::Data::CollPtr coll(new Tellico::Data::Collection(false));
  coll->addField(Tellico::Data::FieldPtr(new Tellico::Data::Field(QStringLiteral("custom"), QStringLiteral("Custom"))));

  Tellico::CollectionFieldsDialog dialog(coll, nullptr);
  dialog.setNotifyKernel(false);

  auto* fieldsWidget = dialog.findChild<QListWidget*>();
  QVERIFY(fieldsWidget);
  fieldsWidget->setCurrentRow(-1);
  fieldsWidget->setCurrentRow(0);
  QVERIFY(fieldsWidget->currentItem());

  QVERIFY(QMetaObject::invokeMethod(&dialog, "slotDelete"));
  QCOMPARE(fieldsWidget->count(), 0);
  QVERIFY(!fieldsWidget->currentItem());
}
