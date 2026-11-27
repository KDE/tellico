// SPDX-FileCopyrightText: 2026 The Tellico contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TELLICO_COLLECTIONFIELDSDIALOGTEST_H
#define TELLICO_COLLECTIONFIELDSDIALOGTEST_H

#include <QObject>

class CollectionFieldsDialogTest : public QObject {
Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void testDeleteLastField();
};

#endif
