/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BORROWERDIALOG_H
#define BORROWERDIALOG_H

class KLineEdit;

#include "borrower.h"

#include <kdialogbase.h>
#include <kabc/addressee.h>

#include <klistview.h>
#include <qdict.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class BorrowerDialog : public KDialogBase {
Q_OBJECT

public:
  static Data::Borrower* getBorrower(QWidget* parent);

private slots:
  void selectItem(const QString& name);
  void updateEdit(QListViewItem* item);
  void slotLoadAddressBook();

private:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BorrowerDialog(QWidget* parent, const char* name=0);
  Data::Borrower* borrower();

  QString m_uid;
  KListView* m_listView;
  KLineEdit* m_lineEdit;
  QDict<KListViewItem> m_itemDict;

class Item : public KListViewItem {
public:
  Item(KListView* parent, const KABC::Addressee& addressee);
  Item(KListView* parent, const Data::Borrower& borrower);
  const QString& uid() const { return m_uid; }

private:
  QString m_uid;
};

};

} // end namespace
#endif
