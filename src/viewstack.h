/***************************************************************************
    copyright            : (C) 2002-2008 by Robby Stephenson
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

#include <QStackedWidget>

namespace Tellico {
  class EntryView;
  class EntryIconView;

/**
 * @author Robby Stephenson
 */
class ViewStack : public QStackedWidget {
Q_OBJECT

public:
  ViewStack(QWidget* parent);

  EntryView* entryView() { return m_entryView; }
  EntryIconView* iconView() { return m_iconView; }

  void clear();
  void refresh();
  void showEntry(Data::EntryPtr entry);
  void showEntries(const Data::EntryList& entries);

private:
  EntryView* m_entryView;
  EntryIconView* m_iconView;
};

} // end namespace
#endif
