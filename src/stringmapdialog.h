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

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: stringmapdialog.h 386 2004-01-24 05:12:28Z robby $
 */
class StringMapDialog : public KDialogBase {
Q_OBJECT

public:
  StringMapDialog(const Data::StringMap& stringMap, QWidget* parent, const char* name=0, bool modal=false);

  void setLabels(const QString& label1, const QString& label2);
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
