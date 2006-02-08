/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellico_kernel.h"
#include "mainwindow.h"
#include "document.h"
#include "collection.h"
#include "entryitem.h"
#include "controller.h"
#include "filter.h"
#include "filterdialog.h"
#include "loandialog.h"
#include "calendarhandler.h"
#include "tellico_utils.h"
#include "tellico_debug.h"
#include "commands/group.h"
#include "commands/collectioncommand.h"
#include "commands/fieldcommand.h"
#include "commands/filtercommand.h"
#include "commands/addentries.h"
#include "commands/modifyentries.h"
#include "commands/removeentries.h"
#include "commands/removeloans.h"
#include "commands/reorderfields.h"
#include "commands/renamecollection.h"

#include <kmessagebox.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kinputdialog.h>

using Tellico::Kernel;
Kernel* Kernel::s_self = 0;

Kernel::Kernel(MainWindow* parent) : m_widget(parent)
    , m_commandHistory(parent->actionCollection())
    , m_commandGroup(0)
    , m_writeImagesInFile(true) {
}

const KURL& Kernel::URL() const {
  return Data::Document::self()->URL();
}

const QStringList& Kernel::fieldTitles() const {
  return Data::Document::self()->collection()->fieldTitles();
}

QString Kernel::fieldNameByTitle(const QString& title_) const {
  return Data::Document::self()->collection()->fieldNameByTitle(title_);
}

QString Kernel::fieldTitleByName(const QString& name_) const {
  return Data::Document::self()->collection()->fieldTitleByName(name_);
}

QStringList Kernel::valuesByFieldName(const QString& name_) const {
  return Data::Document::self()->collection()->valuesByFieldName(name_);
}

int Kernel::collectionType() const {
  return Data::Document::self()->collection()->type();
}

QString Kernel::collectionTypeName() const {
  return Data::Document::self()->collection()->typeName();
}

void Kernel::sorry(const QString& text_, QWidget* widget_/* =0 */) {
  if(text_.isEmpty()) {
    return;
  }
  GUI::CursorSaver cs(Qt::arrowCursor);
  KMessageBox::sorry(widget_ ? widget_ : m_widget, text_);
}

void Kernel::beginCommandGroup(const QString& name_) {
  if(m_commandGroup) {
    myDebug() << "Kernel::beginCommandGroup() - there's an uncommitted group!" << endl;
    delete m_commandGroup;
  }
  m_commandGroup = new Command::Group(name_);
}

void Kernel::endCommandGroup() {
  if(!m_commandGroup) {
    myDebug() << "Kernel::endCommandGroup() - beginCommandGroup() must be called first!" << endl;
    return;
  }
  if(m_commandGroup->isEmpty()) {
    delete m_commandGroup;
  } else {
    m_commandHistory.addCommand(m_commandGroup);
    Data::Document::self()->slotSetModified(true);
  }
  m_commandGroup = 0;
}

void Kernel::resetHistory() {
  m_commandHistory.clear();
  m_commandHistory.documentSaved();
}

bool Kernel::addField(Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldAdd,
                                      Data::Document::self()->collection(),
                                      field_));
  return true;
}

bool Kernel::modifyField(Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
  Data::FieldPtr oldField = Data::Document::self()->collection()->fieldByName(field_->name());
  if(!oldField) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldModify,
                                      Data::Document::self()->collection(),
                                      field_,
                                      oldField));
  return true;
}

bool Kernel::removeField(Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldRemove,
                                      Data::Document::self()->collection(),
                                      field_));
  return true;
}

void Kernel::addEntries(Data::EntryVec entries_, bool checkFields_) {
  if(entries_.isEmpty()) {
    return;
  }

  KCommand* cmd = new Command::AddEntries(Data::Document::self()->collection(), entries_);
  if(checkFields_) {
    beginCommandGroup(cmd->name());
    Data::FieldVec fields = entries_[0]->collection()->fields();
    for(Data::FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
      if(Data::Document::self()->collection()->hasField(field->name())) {
        continue;
      }
      // add field if any values are not empty
      for(Data::EntryVec::Iterator entry = entries_.begin(); entry != entries_.end(); ++entry) {
        if(!entry->field(field).isEmpty()) {
          addField(field);
          break;
        }
      }
    }
  }
  doCommand(cmd);
  if(checkFields_) {
    endCommandGroup();
  }
}

void Kernel::modifyEntries(Data::EntryVec oldEntries_, Data::EntryVec newEntries_) {
  if(newEntries_.isEmpty()) {
    return;
  }

  doCommand(new Command::ModifyEntries(Data::Document::self()->collection(), oldEntries_, newEntries_));
}

void Kernel::removeEntries(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  doCommand(new Command::RemoveEntries(Data::Document::self()->collection(), entries_));
}

bool Kernel::addLoans(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return false;
  }

  LoanDialog dlg(entries_, m_widget);
  if(dlg.exec() != QDialog::Accepted) {
    return false;
  }

  KCommand* cmd = dlg.createCommand();
  if(!cmd) {
    return false;
  }
  doCommand(cmd);
  return true;
}

bool Kernel::modifyLoan(Data::LoanPtr loan_) {
  if(!loan_) {
    return false;
  }

  LoanDialog dlg(loan_, m_widget);
  if(dlg.exec() != QDialog::Accepted) {
    return false;
  }

  KCommand* cmd = dlg.createCommand();
  if(!cmd) {
    return false;
  }
  doCommand(cmd);
  return true;
}

bool Kernel::removeLoans(Data::LoanVec loans_) {
  if(loans_.isEmpty()) {
    return true;
  }

  doCommand(new Command::RemoveLoans(loans_));
  return true;
}

void Kernel::addFilter(FilterPtr filter_) {
  if(!filter_) {
    return;
  }

  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterAdd, filter_));
}

bool Kernel::modifyFilter(FilterPtr filter_) {
  if(!filter_) {
    return false;
  }

  FilterDialog filterDlg(FilterDialog::ModifyFilter, m_widget);
  // need to create a new filter object
  FilterPtr newFilter = new Filter(*filter_);
  filterDlg.setFilter(newFilter);
  if(filterDlg.exec() != QDialog::Accepted) {
    return false;
  }

  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterModify, newFilter, filter_));
  return true;
}

bool Kernel::removeFilter(FilterPtr filter_) {
  if(!filter_) {
    return false;
  }

  QString str = i18n("Do you really want to delete this filter?");
  QString dontAsk = QString::fromLatin1("DeleteFilter");
  int ret = KMessageBox::questionYesNo(m_widget, str, i18n("Delete Filter?"),
                                       KStdGuiItem::yes(), KStdGuiItem::no(), dontAsk);
  if(ret != KMessageBox::Yes) {
    return false;
  }

  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterRemove, filter_));
  return true;
}

void Kernel::reorderFields(const Data::FieldVec& fields_) {
  doCommand(new Command::ReorderFields(Data::Document::self()->collection(),
                                       Data::Document::self()->collection()->fields(),
                                       fields_));
}

void Kernel::appendCollection(Data::CollPtr coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Append,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::mergeCollection(Data::CollPtr coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Merge,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::replaceCollection(Data::CollPtr coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Replace,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::renameCollection() {
  bool ok;
  QString newTitle = KInputDialog::getText(i18n("Rename Collection"), i18n("New collection name:"),
                                           Data::Document::self()->collection()->title(), &ok, m_widget);
  if(ok) {
    doCommand(new Command::RenameCollection(Data::Document::self()->collection(), newTitle));
  }
}

void Kernel::doCommand(KCommand* command_) {
  if(m_commandGroup) {
    m_commandGroup->addCommand(command_);
  } else {
    m_commandHistory.addCommand(command_);
    Data::Document::self()->slotSetModified(true);
  }
}
