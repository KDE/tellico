/***************************************************************************
    copyright            : (C) 2002-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_VIEWSTACK_H
#define TELLICO_VIEWSTACK_H

#include "datavectors.h"

#include <qwidgetstack.h>

namespace Tellico {
  class EntryView;
  class EntryIconView;

/**
 * @author Robby Stephenson
 */
class ViewStack : public QWidgetStack {
Q_OBJECT

public:
  ViewStack(QWidget* parent, const char* name = 0);

  EntryView* entryView() const { return m_entryView; }
  EntryIconView* iconView() const { return m_iconView; }

  void clear();
  void refresh();
  void showEntry(Data::EntryPtr entry);
  void showEntries(const Data::EntryVec& entries);

private:
  EntryView* m_entryView;
  EntryIconView* m_iconView;
};

} // end namespace
#endif
