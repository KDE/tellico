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
    , m_currentEntry(currEntry_)
    , m_newEntry(newEntry_)
    , m_oldEntry(m_currentEntry)
    , m_overWrite(overWrite_) {
  }

  virtual void execute() {
    *m_oldEntry = *m_currentEntry;
    Data::Collection::mergeEntry(m_currentEntry, m_newEntry, m_overWrite);
  }

  virtual void unexecute() {
    *m_currentEntry = *m_oldEntry;
  }

  virtual QString name() const { return QString::null; }

  Data::EntryPtr oldEntry() const { return m_oldEntry; }

private:
  Data::EntryPtr m_currentEntry;
  Data::EntryPtr m_newEntry;
  Data::EntryPtr m_oldEntry;
  bool m_overWrite;
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

    MergeEntries* cmd = new MergeEntries(m_oldEntry, m_newEntry, m_overWrite);
    addCommand(cmd);
    // now the oldEntry has the new entry values
    addCommand(new ModifyEntries(m_coll, cmd->oldEntry(), m_oldEntry));
  }
  Group::execute();
}
