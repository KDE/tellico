/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_BORROWERDIALOG_H
#define TELLICO_BORROWERDIALOG_H

#include "borrower.h"

#include <kdialog.h>

#include <QHash>
#include <QTreeWidget>

class KLineEdit;
namespace KABC {
  class Addressee;
}

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class BorrowerDialog : public KDialog {
Q_OBJECT

public:
  static Data::BorrowerPtr getBorrower(QWidget* parent);

private slots:
  void selectItem(const QString& name);
  void updateEdit(QTreeWidgetItem* item);
  void slotLoadAddressBook();

private:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   */
  BorrowerDialog(QWidget* parent);
  Data::BorrowerPtr borrower();

  QString m_uid;
  QTreeWidget* m_treeWidget;
  KLineEdit* m_lineEdit;
  QHash<QString, QTreeWidgetItem*> m_itemHash;

class Item : public QTreeWidgetItem {
public:
  Item(QTreeWidget* parent, const KABC::Addressee& addressee);
  Item(QTreeWidget* parent, const Data::Borrower& borrower);
  const QString& uid() const { return m_uid; }

private:
  QString m_uid;
};

};

} // end namespace
#endif
