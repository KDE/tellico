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

#ifndef FILTERITEM_H
#define FILTERITEM_H

#include "gui/counteditem.h"
#include "filter.h"

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FilterItem : public GUI::CountedItem {
public:
  FilterItem(GUI::ListView* parent, Tellico::Filter* filter);

  virtual bool isFilterItem() const { return true; }
  Tellico::Filter* filter() { return m_filter; }
  void updateFilter(Tellico::Filter* filter);

private:
  KSharedPtr<Filter> m_filter;
};

}

#endif
