/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_ENTRYMERGER_H
#define TELLICO_ENTRYMERGER_H

#include "datavectors.h"

#include <QObject>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryMerger : public QObject {
Q_OBJECT
public:
  EntryMerger(Data::EntryList entries, QObject* parent);

public slots:
  void slotCancel();

private slots:
  void slotStartNext();
  void slotCleanup();

private:
  // if a clean merge is possible
  bool cleanMerge(Data::EntryPtr entry1, Data::EntryPtr entry2) const;
  bool askUser(Data::EntryPtr entry1, Data::EntryPtr entry2);

  Data::EntryList m_entriesToCheck;
  Data::EntryList m_entriesToRemove;
  Data::EntryList m_entriesLeft;
  int m_origCount;
  bool m_cancelled;
};

} // end namespace

#endif
