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
#include "commands/updateentries.h"
#include "commands/removeentries.h"
#include "commands/removeloans.h"
#include "commands/reorderfields.h"
#include "commands/renamecollection.h"
#include "collectionfactory.h"
#include "stringset.h"

#include <kmessagebox.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>

using Tellico::Kernel;
Kernel* Kernel::s_self = 0;

Kernel::Kernel(MainWindow* parent) : m_widget(parent)
    , m_commandHistory(parent->actionCollection())
    , m_commandGroup(0) {
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

    // this is the same as in Command::UpdateEntries::execute()
    Data::CollPtr c = Data::Document::self()->collection();
    Data::FieldVec fields = entries_[0]->collection()->fields();

    QPair<Data::FieldVec, Data::FieldVec> p = mergeFields(c, fields, entries_);
    Data::FieldVec modifiedFields = p.first;
    Data::FieldVec addedFields = p.second;

    for(Data::FieldVec::Iterator field = modifiedFields.begin(); field != modifiedFields.end(); ++field) {
      if(c->hasField(field->name())) {
        doCommand(new Command::FieldCommand(Command::FieldCommand::FieldModify, c,
                                            field, c->fieldByName(field->name())));
      }
    }

    for(Data::FieldVec::Iterator field = addedFields.begin(); field != addedFields.end(); ++field) {
      doCommand(new Command::FieldCommand(Command::FieldCommand::FieldAdd, c, field));
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

void Kernel::updateEntry(Data::EntryPtr oldEntry_, Data::EntryPtr newEntry_, bool overWrite_) {
  if(!newEntry_) {
    return;
  }

  doCommand(new Command::UpdateEntries(Data::Document::self()->collection(), oldEntry_, newEntry_, overWrite_));
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

QPair<Tellico::Data::FieldVec, Tellico::Data::FieldVec> Kernel::mergeFields(Data::CollPtr coll_,
                                                                            Data::FieldVec fields_,
                                                                            Data::EntryVec entries_) {
  Data::FieldVec modified, created;
  for(Data::FieldVec::Iterator field = fields_.begin(); field != fields_.end(); ++field) {
    // don't add a field if it's a default field and not in the current collection
    if(coll_->hasField(field->name()) || CollectionFactory::isDefaultField(coll_->type(), field->name())) {
      // special case for choice fields, since we might want to add a value
      if(field->type() == Data::Field::Choice && coll_->hasField(field->name())) {
        QStringList a1 = field->allowed();
        QStringList a2 = coll_->fieldByName(field->name())->allowed();
        if(a1 != a2) {
          StringSet a;
          a.add(a1);
          a.add(a2);
          Data::FieldPtr f = new Data::Field(*coll_->fieldByName(field->name()));
          f->setAllowed(a.toList());
          modified.append(f);
        }
      }
      continue;
    }
    // add field if any values are not empty
    for(Data::EntryVec::Iterator entry = entries_.begin(); entry != entries_.end(); ++entry) {
      if(!entry->field(field).isEmpty()) {
        created.append(new Data::Field(*field));
        break;
      }
    }
  }
  return qMakePair(modified, created);
}

int Kernel::askAndMerge(Data::EntryPtr entry1_, Data::EntryPtr entry2_, Data::FieldPtr field_,
                        QString value1_, QString value2_) {
  QString title1 = entry1_->field(QString::fromLatin1("title"));
  QString title2 = entry2_->field(QString::fromLatin1("title"));
  if(title1 == title2) {
    title1 = i18n("Entry 1");
    title2 = i18n("Entry 2");
  }
  if(value1_.isEmpty()) {
    value1_ = entry1_->field(field_);
  }
  if(value2_.isEmpty()) {
    value2_ = entry2_->field(field_);
  }
  QString text = QString::fromLatin1("<qt>")
                + i18n("Conflicting values for %1 were found while merging entries.").arg(field_->title())
                + QString::fromLatin1("<br/><center><table><tr>"
                                      "<th>%1</th>"
                                      "<th>%2</th></tr>").arg(title1, title2)
                + QString::fromLatin1("<tr><td><em>%1</em></td>").arg(value1_)
                + QString::fromLatin1("<td><em>%1</em></td></tr></table></center>").arg(value2_)
                + i18n("Please choose which value to keep.")
                + QString::fromLatin1("</qt>");

  int ret = KMessageBox::warningYesNoCancel(Kernel::self()->widget(),
                                            text,
                                            i18n("Merge Entries"),
                                            i18n("Select value from %1").arg(title1),
                                            i18n("Select value from %1").arg(title2));
  switch(ret) {
    case KMessageBox::Cancel: return 0;
    case KMessageBox::Yes: return -1; // keep original value
    case KMessageBox::No: return 1; // use newer value
  }
  return 0;
}
