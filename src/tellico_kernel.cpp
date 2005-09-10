/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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

#include <kdebug.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdeversion.h>
#if KDE_IS_VERSION(3,1,90)
#include <kinputdialog.h>
#else
#include <klineeditdlg.h>
#endif

using Tellico::Kernel;
Kernel* Kernel::s_self = 0;

Kernel::Kernel(MainWindow* parent) : m_widget(parent), m_commandHistory(parent->actionCollection()), m_commandGroup(0) {
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

const QString& Kernel::entryName() const {
  return Data::Document::self()->collection()->entryName();
}

int Kernel::collectionType() const {
  return Data::Document::self()->collection()->type();
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

bool Kernel::addField(Data::Field* field_) {
  if(!field_) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldAdd,
                                      Data::Document::self()->collection(),
                                      field_));
  return true;
}

bool Kernel::modifyField(Data::Field* field_) {
  if(!field_) {
    return false;
  }
  Data::Field* oldField = Data::Document::self()->collection()->fieldByName(field_->name());
  if(!oldField) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldModify,
                                      Data::Document::self()->collection(),
                                      field_,
                                      oldField));
  return true;
}

bool Kernel::removeField(Data::Field* field_) {
  if(!field_) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldRemove,
                                      Data::Document::self()->collection(),
                                      field_));
  return true;
}

void Kernel::saveEntries(Data::EntryVec oldEntries_, Data::EntryVec newEntries_) {
  if(newEntries_.isEmpty()) {
    return;
  }

  KCommand* cmd;
  if(newEntries_[0]->isOwned()) {
    cmd = new Command::ModifyEntries(Data::Document::self()->collection(), oldEntries_, newEntries_);
  } else {
    cmd = new Command::AddEntries(Data::Document::self()->collection(), newEntries_);
  }
  doCommand(cmd);
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

bool Kernel::modifyLoan(Data::Loan* loan_) {
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

void Kernel::addFilter(Filter* filter_) {
  if(!filter_) {
    return;
  }

  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterAdd, filter_));
}

bool Kernel::modifyFilter(Filter* filter_) {
  FilterDialog filterDlg(FilterDialog::ModifyFilter, m_widget);
  // need to create a new filter object
  Filter* newFilter = new Filter(*filter_);
  filterDlg.setFilter(newFilter);
  if(filterDlg.exec() != QDialog::Accepted) {
    delete newFilter;
    return false;
  }

  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterModify, newFilter, filter_));
  return true;
}

bool Kernel::removeFilter(Filter* filter_) {
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

void Kernel::appendCollection(Data::Collection* coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Append,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::mergeCollection(Data::Collection* coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Merge,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::replaceCollection(Data::Collection* coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Replace,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::renameCollection() {
  bool ok;
#if KDE_IS_VERSION(3,1,90)
  QString newTitle = KInputDialog::getText(i18n("Rename Collection"), i18n("New collection name:"),
                                           Data::Document::self()->collection()->title(), &ok, m_widget);
#else
  QString newTitle = KLineEditDlg::getText(i18n("Rename Collection"), i18n("New collection name:"),
                                           Data::Document::self()->collection()->title(), &ok, m_widget);
#endif
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
