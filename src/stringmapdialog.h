/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef STRINGMAPDIALOG_H
#define STRINGMAPDIALOG_H

class KLineEdit;
class QListView;
class QListViewItem;

#include "field.h" // needed for StringMap typedef

#include <kdialogbase.h>

namespace Tellico {

/**
 * @short A simple dialog for editing a map between two strings.
 *
 * A \ref KListView is used with the map keys in the first column and
 * the map values in the second. Two edit boxes are below the list view.
 * When an item is selected, the key-value is pair is placed in the edit
 * boxes. Add and Delete buttons are used to add a new pair, or to remove
 * an existing one.
 *
 * @author Robby Stephenson
 * @version $Id: stringmapdialog.h 934 2004-11-01 03:33:54Z robby $
 */
class StringMapDialog : public KDialogBase {
Q_OBJECT

public:
  StringMapDialog(const Data::StringMap& stringMap, QWidget* parent, const char* name=0, bool modal=false);

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
  Data::StringMap stringMap();

private slots:
  void slotAdd();
  void slotDelete();
  void slotClicked(QListViewItem* item);

protected:
  QListView* m_listView;
  KLineEdit* m_edit1;
  KLineEdit* m_edit2;
};

} // end namespace
#endif
