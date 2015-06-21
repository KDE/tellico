/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_STRINGMAPDIALOG_H
#define TELLICO_STRINGMAPDIALOG_H

#include <KDialog>

class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;
template <typename T1, typename T2>
class QMap;

namespace Tellico {

/**
 * @short A simple dialog for editing a map between two strings.
 *
 * A \ref QTreeWidget is used with the map keys in the first column and
 * the map values in the second. Two edit boxes are below the list view.
 * When an item is selected, the key-value is pair is placed in the edit
 * boxes. Add and Delete buttons are used to add a new pair, or to remove
 * an existing one.
 *
 * @author Robby Stephenson
 */
class StringMapDialog : public KDialog {
Q_OBJECT

public:
  StringMapDialog(const QMap<QString, QString>& stringMap, QWidget* parent, bool modal=false);

  /**
   * Sets the titles for the key and value columns.
   *
   * @param label1 The name of the key string
   * @param label2 The name of the value string
   */
  void setLabels(const QString& label1, const QString& label2);
  /**
   * Returns the modified string map.
   *
   * @return The modified string map
   */
  QMap<QString, QString> stringMap();

private slots:
  void slotAdd();
  void slotDelete();
  void slotUpdate(QTreeWidgetItem* item);

protected:
  QTreeWidget* m_treeWidget;
  QLineEdit* m_edit1;
  QLineEdit* m_edit2;
};

} // end namespace
#endif
