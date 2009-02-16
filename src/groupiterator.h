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

#ifndef TELLICO_GROUPITERATOR_H
#define TELLICO_GROUPITERATOR_H

class QAbstractItemModel;

namespace Tellico {
  namespace Data {
    class EntryGroup;
  }

/**
 * @author Robby Stephenson
 */
class GroupIterator{
public:
  GroupIterator(QAbstractItemModel* model);

  GroupIterator& operator++();
  Data::EntryGroup* group();

private:
  QAbstractItemModel* m_model;
  int m_row;
};

}

#endif
