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

#include "filteritem.h"

#include <kiconloader.h>

#include <qpixmap.h>

using Tellico::FilterItem;

FilterItem::FilterItem(GUI::ListView* parent_, Tellico::Filter* filter_)
    : GUI::CountedItem(parent_), m_filter(filter_) {
  setText(0, filter_->name());
  setPixmap(0, SmallIcon(QString::fromLatin1("filter")));
}

void FilterItem::updateFilter(Tellico::Filter* filter_) {
  m_filter = filter_;
  setText(0, m_filter->name());
}
