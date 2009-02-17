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

#ifndef TELLICO_GUI_LISTWIDGETITEM_H
#define TELLICO_GUI_LISTWIDGETITEM_H

#include <QListWidgetItem>

namespace Tellico {
  namespace GUI {

class ListWidgetItem : public QListWidgetItem {
public:
  ListWidgetItem(const QString& text, QListWidget* parent);

  bool isColored() const { return m_colored; }
  void setColored(bool colored);

private:
  bool m_colored;
};

  } // end namespace
} // end namespace

#endif
