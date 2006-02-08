/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GROUPITERATOR_H
#define TELLICO_GROUPITERATOR_H

class QListView;

namespace Tellico {
  namespace Data {
    class EntryGroup;
  }
  namespace GUI {
    class ListViewItem;
  }

/**
 * @author Robby Stephenson
 */
class GroupIterator{
public:
  GroupIterator(const QListView* view);

  GroupIterator& operator++();
  Data::EntryGroup* group();

private:
  GUI::ListViewItem* m_item;
};

}

#endif
