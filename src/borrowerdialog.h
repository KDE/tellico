/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_BORROWERDIALOG_H
#define TELLICO_BORROWERDIALOG_H

#include <config.h>
#include "borrower.h"

#include <QDialog>
#include <QHash>
#include <QTreeWidget>

class KLineEdit;
class KJob;
#ifdef HAVE_KABC
namespace KContacts {
  class Addressee;
}
#endif

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class BorrowerDialog : public QDialog {
Q_OBJECT

public:
  static Data::BorrowerPtr getBorrower(QWidget* parent);

private slots:
  void selectItem(const QString& name);
  void updateEdit(QTreeWidgetItem* item);
  void akonadiSearchResult(KJob*);

private:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   */
  BorrowerDialog(QWidget* parent);
  Data::BorrowerPtr borrower();
  void populateBorrowerList();

  QString m_uid;
  QTreeWidget* m_treeWidget;
  KLineEdit* m_lineEdit;
  QHash<QString, QTreeWidgetItem*> m_itemHash;

class Item : public QTreeWidgetItem {
public:
#ifdef HAVE_KABC
  Item(QTreeWidget* parent, const KContacts::Addressee& addressee);
#endif
  Item(QTreeWidget* parent, const Data::Borrower& borrower);
  const QString& uid() const { return m_uid; }

private:
  QString m_uid;
};

};

} // end namespace
#endif
