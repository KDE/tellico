/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_ENTRYMERGER_H
#define TELLICO_ENTRYMERGER_H

#include "datavectors.h"
#include "utils/mergeconflictresolver.h"

#include <QObject>

namespace Tellico {
  namespace Merge {

class AskUserResolver : public ConflictResolver {
public:
  AskUserResolver()  {};
  ~AskUserResolver() {};
  virtual ConflictResolver::Result resolve(Data::EntryPtr entry1, Data::EntryPtr entry2, Data::FieldPtr field,
                                           const QString& value1 = QString(), const QString& value2 = QString()) override;

};

} // end namespace Merge

/**
 * @author Robby Stephenson
 */
class EntryMerger : public QObject {
Q_OBJECT
public:
  EntryMerger(Data::EntryList entries, QObject* parent);
  ~EntryMerger();

public Q_SLOTS:
  void slotCancel();

private Q_SLOTS:
  void slotStartNext();
  void slotCleanup();

private:
  // if a clean merge is possible
  bool cleanMerge(Data::EntryPtr entry1, Data::EntryPtr entry2) const;

  Data::EntryList m_entriesToCheck;
  Data::EntryList m_entriesToRemove;
  Data::EntryList m_entriesLeft;
  int m_origCount;
  bool m_cancelled;
  Merge::ConflictResolver* m_resolver;
};

} // end namespace

#endif
