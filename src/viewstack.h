/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCASEVIEWSTACK_H
#define BOOKCASEVIEWSTACK_H

#include <qwidgetstack.h>

namespace Tellico {
  class EntryView;
  class EntryIconView;
  namespace Data {
    class Collection;
    class Entry;
    class EntryGroup;
  }

/**
 * @author Robby Stephenson
 * @version $Id: viewstack.h 862 2004-09-15 01:49:51Z robby $
 */
class ViewStack : public QWidgetStack {
Q_OBJECT

public:
  ViewStack(QWidget* parent, const char* name = 0);

  EntryView* entryView() const { return m_entryView; }
  EntryIconView* iconView() const { return m_iconView; }

  void clear();
  void refresh();
  void showCollection(const Data::Collection* collection);
  void showEntry(const Data::Entry* entry);
  void showEntryGroup(const Data::EntryGroup* group);

private:
  EntryView* m_entryView;
  EntryIconView* m_iconView;
};

} // end namespace
#endif
