/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#include "updateentries.h"
#include "fieldcommand.h"
#include "modifyentries.h"
#include "../collection.h"
#include "../utils/mergeconflictresolver.h"

#include <KLocalizedString>

namespace {

class OverWriteResolver : public Tellico::Merge::ConflictResolver {
public:
  OverWriteResolver(bool overwrite_) : m_overWrite(overwrite_) {};
  Tellico::Merge::ConflictResolver::Result resolve(Tellico::Data::EntryPtr,
                                                   Tellico::Data::EntryPtr,
                                                   Tellico::Data::FieldPtr,
                                                   const QString& value1 = QString(),
                                                   const QString& value2 = QString()) override {
    Q_UNUSED(value1);
    Q_UNUSED(value2);
    return m_overWrite ? Tellico::Merge::ConflictResolver::KeepSecond : Tellico::Merge::ConflictResolver::KeepFirst;
  }

private:
  Q_DISABLE_COPY(OverWriteResolver)
  bool m_overWrite;
};

}

using Tellico::Command::UpdateEntries;

namespace Tellico {
  namespace Command {

class MergeEntries : public QUndoCommand {
public:
  MergeEntries(QUndoCommand* updater, Data::EntryPtr currEntry_, Data::EntryPtr newEntry_, bool overWrite_)
    : QUndoCommand(updater)
    , m_currEntry(currEntry_)
    , m_newEntry(newEntry_)
    , m_orphanEntry(new Data::Entry(*currEntry_))
    , m_overWrite(overWrite_) {
    // we merge the entries here instead of in redo() because this
    // command is never called without also calling ModifyEntries()
    // which takes care of copying the entry values
//    Data::Document::mergeEntry(currEntry_, newEntry_, overWrite_);
  }

  virtual void redo() override {
    OverWriteResolver res(m_overWrite);
    Tellico::Merge::mergeEntry(m_currEntry, m_newEntry, &res);
  }
  virtual void undo() override {} // does nothing
  Data::EntryPtr orphanEntry() const { return m_orphanEntry; }

private:
  Data::EntryPtr m_currEntry;
  Data::EntryPtr m_newEntry;
  Data::EntryPtr m_orphanEntry;
  bool m_overWrite;
};
  }
}

UpdateEntries::UpdateEntries(Tellico::Data::CollPtr coll_, Tellico::Data::EntryPtr oldEntry_, Tellico::Data::EntryPtr newEntry_, bool overWrite_)
    : QUndoCommand(i18nc("Modify (Entry Title)", "Modify %1", newEntry_->title()))
    , m_coll(coll_)
    , m_oldEntry(oldEntry_)
    , m_newEntry(newEntry_)
    , m_overWrite(overWrite_)
{
}

void UpdateEntries::redo() {
  if(childCount() == 0) {
    // add commands
    // do this here instead of the constructor because several UpdateEntries may be in one command
    // and I don't want to add new fields multiple times

    auto p = Tellico::Merge::mergeFields(m_coll,
                                         m_newEntry->collection()->fields(),
                                         Data::EntryList() << m_newEntry);
    Data::FieldList modifiedFields = p.first;
    Data::FieldList addedFields = p.second;

    QStringList updatedFields;

    foreach(Data::FieldPtr field, modifiedFields) {
      if(m_coll->hasField(field->name())) {
        new FieldCommand(this, FieldCommand::FieldModify, m_coll,
                         field, m_coll->fieldByName(field->name()));
      }
      updatedFields << field->name();
    }

    foreach(Data::FieldPtr field, addedFields) {
      new FieldCommand(this, FieldCommand::FieldAdd, m_coll, field);
      updatedFields << field->name();
    }

    // MergeEntries copies values from m_newEntry into m_oldEntry
    // m_oldEntry is in the current collection
    // m_newEntry isn't...
    MergeEntries* cmd = new MergeEntries(this, m_oldEntry, m_newEntry, m_overWrite);
    // cmd->orphanEntry() returns a copy of m_oldEntry before values were merged
    // m_oldEntry has new values
    // in the ModifyEntries command, the second entry should be owned by the current
    // collection and contain the updated values
    // the first one is not owned by current collection
    new ModifyEntries(this, m_coll, Data::EntryList() << cmd->orphanEntry(), Data::EntryList() << m_oldEntry, updatedFields);
  }
  // calls redo() on all child commands
  QUndoCommand::redo();
}
