/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "updateentries.h"
#include "fieldcommand.h"
#include "modifyentries.h"
#include "../collection.h"
#include "../tellico_kernel.h"
#include "../document.h"

#include <klocale.h>

using Tellico::Command::UpdateEntries;

namespace Tellico {
  namespace Command {

class MergeEntries : public KCommand {
public:
  MergeEntries(Data::EntryPtr currEntry_, Data::EntryPtr newEntry_, bool overWrite_) : KCommand()
    , m_oldEntry(new Data::Entry(*currEntry_)) {
    // we merge the entries here instead of in execute() because this
    // command is never called without also calling ModifyEntries()
    // which takes care of copying the entry values
    Data::Collection::mergeEntry(currEntry_, newEntry_, overWrite_);
  }

  virtual void execute() {} // does nothing
  virtual void unexecute() {} // does nothing
  virtual QString name() const { return QString(); }
  Data::EntryPtr oldEntry() const { return m_oldEntry; }

private:
  Data::EntryPtr m_oldEntry;
};
  }
}

UpdateEntries::UpdateEntries(Data::CollPtr coll_, Data::EntryPtr oldEntry_, Data::EntryPtr newEntry_, bool overWrite_)
    : Group(i18n("Modify (Entry Title)", "Modify %1").arg(newEntry_->title()))
    , m_coll(coll_)
    , m_oldEntry(oldEntry_)
    , m_newEntry(newEntry_)
    , m_overWrite(overWrite_)
{
}

void UpdateEntries::execute() {
  if(isEmpty()) {
    // add commands
    // do this here instead of the constructor because several UpdateEntries may be in one command
    // and I don't want to add new fields multiple times

    QPair<Data::FieldVec, Data::FieldVec> p = Kernel::self()->mergeFields(m_coll,
                                                                          m_newEntry->collection()->fields(),
                                                                          m_newEntry);
    Data::FieldVec modifiedFields = p.first;
    Data::FieldVec addedFields = p.second;

    for(Data::FieldVec::Iterator field = modifiedFields.begin(); field != modifiedFields.end(); ++field) {
      if(m_coll->hasField(field->name())) {
        addCommand(new FieldCommand(FieldCommand::FieldModify, m_coll,
                                    field, m_coll->fieldByName(field->name())));
      }
    }

    for(Data::FieldVec::Iterator field = addedFields.begin(); field != addedFields.end(); ++field) {
      addCommand(new FieldCommand(FieldCommand::FieldAdd, m_coll, field));
    }

    // MergeEntries copies values from m_newEntry into m_oldEntry
    // m_oldEntry is in the current collection
    // m_newEntry isn't...
    MergeEntries* cmd = new MergeEntries(m_oldEntry, m_newEntry, m_overWrite);
    addCommand(cmd);
    // cmd->oldEntry() returns a copy of m_oldEntry before values were merged
    // m_oldEntry has new values
    // in the ModifyEntries command, the second entry should be owned by the current
    // collection and contain the updated values
    // the first one is not owned by current collection
    addCommand(new ModifyEntries(m_coll, cmd->oldEntry(), m_oldEntry));
  }
  Group::execute();
}
