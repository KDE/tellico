/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellico_kernel.h"
#include "document.h"
#include "collection.h"
#include "filter.h"
#include "filterdialog.h"
#include "loandialog.h"
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
#include "utils/cursorsaver.h"
#include "utils/mergeconflictresolver.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <kwidgetsaddons_version.h>

#include <QInputDialog>
#include <QUndoStack>

using Tellico::Kernel;
Kernel* Kernel::s_self = nullptr;

Kernel::Kernel(QWidget* parent) : QObject()
    , m_widget(parent)
    , m_commandHistory(new QUndoStack(parent)) {
}

Kernel::~Kernel() {
}

QUrl Kernel::URL() const {
  return Data::Document::self()->URL();
}

int Kernel::collectionType() const {
  return Data::Document::self()->collection() ?
         Data::Document::self()->collection()->type() :
         Data::Collection::Base;
}

QString Kernel::collectionTypeName() const {
  return CollectionFactory::typeName(collectionType());
}

void Kernel::sorry(const QString& text_, QWidget* widget_/* =nullptr */) {
  if(text_.isEmpty()) {
    return;
  }
  GUI::CursorSaver cs(Qt::ArrowCursor);
  KMessageBox::error(widget_ ? widget_ : m_widget, text_);
}

void Kernel::beginCommandGroup(const QString& name_) {
  m_commandHistory->beginMacro(name_);
}

void Kernel::endCommandGroup() {
  m_commandHistory->endMacro();
}

void Kernel::resetHistory() {
  m_commandHistory->clear();
  m_commandHistory->setClean();
}

bool Kernel::addField(Tellico::Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldAdd,
                                      Data::Document::self()->collection(),
                                      field_));
  return true;
}

bool Kernel::modifyField(Tellico::Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
  Data::FieldPtr oldField = Data::Document::self()->collection()->fieldByName(field_->name());
  if(!oldField) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldModify,
                                      Tellico::Data::Document::self()->collection(),
                                      field_,
                                      oldField));
  return true;
}

bool Kernel::removeField(Tellico::Data::FieldPtr field_) {
  if(!field_) {
    return false;
  }
  doCommand(new Command::FieldCommand(Command::FieldCommand::FieldRemove,
                                      Tellico::Data::Document::self()->collection(),
                                      field_));
  return true;
}

void Kernel::addEntries(Tellico::Data::EntryList entries_, bool checkFields_) {
  if(entries_.isEmpty()) {
    return;
  }

  QUndoCommand* cmd = new Command::AddEntries(Tellico::Data::Document::self()->collection(), entries_);
  if(checkFields_) {
    beginCommandGroup(cmd->text());

    // this is the same as in Command::UpdateEntries::redo()
    Tellico::Data::CollPtr c = Data::Document::self()->collection();
    Tellico::Data::FieldList fields = entries_[0]->collection()->fields();

    auto p = Merge::mergeFields(c, fields, entries_);
    Data::FieldList modifiedFields = p.first;
    Data::FieldList addedFields = p.second;

    foreach(Tellico::Data::FieldPtr field, modifiedFields) {
      if(c->hasField(field->name())) {
        doCommand(new Command::FieldCommand(Command::FieldCommand::FieldModify, c,
                                            field, c->fieldByName(field->name())));
      }
    }

    foreach(Tellico::Data::FieldPtr field, addedFields) {
      doCommand(new Command::FieldCommand(Command::FieldCommand::FieldAdd, c, field));
    }
  }
  doCommand(cmd);
  if(checkFields_) {
    endCommandGroup();
  }
}

void Kernel::modifyEntries(Tellico::Data::EntryList oldEntries_, Tellico::Data::EntryList newEntries_, const QStringList& modifiedFields_) {
  if(newEntries_.isEmpty()) {
    return;
  }

  doCommand(new Command::ModifyEntries(Data::Document::self()->collection(), oldEntries_, newEntries_, modifiedFields_));
}

void Kernel::updateEntry(Tellico::Data::EntryPtr oldEntry_, Tellico::Data::EntryPtr newEntry_, bool overWrite_) {
  if(!newEntry_) {
    return;
  }

  doCommand(new Command::UpdateEntries(Tellico::Data::Document::self()->collection(), oldEntry_, newEntry_, overWrite_));
}

void Kernel::removeEntries(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return;
  }

  doCommand(new Command::RemoveEntries(Tellico::Data::Document::self()->collection(), entries_));
}

bool Kernel::addLoans(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return false;
  }

  LoanDialog dlg(entries_, m_widget);
  if(dlg.exec() != QDialog::Accepted) {
    return false;
  }

  QUndoCommand* cmd = dlg.createCommand();
  if(!cmd) {
    return false;
  }
  doCommand(cmd);
  return true;
}

bool Kernel::modifyLoan(Tellico::Data::LoanPtr loan_) {
  if(!loan_) {
    return false;
  }

  LoanDialog dlg(loan_, m_widget);
  if(dlg.exec() != QDialog::Accepted) {
    return false;
  }

  QUndoCommand* cmd = dlg.createCommand();
  if(!cmd) {
    return false;
  }
  doCommand(cmd);
  return true;
}

bool Kernel::removeLoans(Tellico::Data::LoanList loans_) {
  if(loans_.isEmpty()) {
    return true;
  }

  doCommand(new Command::RemoveLoans(loans_));
  return true;
}

void Kernel::addFilter(Tellico::FilterPtr filter_) {
  if(!filter_) {
    return;
  }

  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterAdd, filter_));
}

bool Kernel::modifyFilter(Tellico::FilterPtr filter_) {
  if(!filter_) {
    return false;
  }

  FilterDialog filterDlg(FilterDialog::ModifyFilter, m_widget);
  // need to create a new filter object
  FilterPtr newFilter(new Filter(*filter_));
  filterDlg.setFilter(newFilter);
  if(filterDlg.exec() != QDialog::Accepted) {
    return false;
  }

  newFilter = filterDlg.currentFilter();
  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterModify, newFilter, filter_));
  return true;
}

bool Kernel::removeFilter(Tellico::FilterPtr filter_) {
  if(!filter_) {
    return false;
  }

  QString str = i18n("Do you really want to delete this filter?");
  QString dontAsk = QStringLiteral("DeleteFilter");
  int ret = KMessageBox::questionTwoActions(m_widget, str, i18n("Delete Filter?"),
                                            KStandardGuiItem::del(), KStandardGuiItem::cancel(), dontAsk);
  if(ret != KMessageBox::ButtonCode::PrimaryAction) {
    return false;
  }

  doCommand(new Command::FilterCommand(Command::FilterCommand::FilterRemove, filter_));
  return true;
}

void Kernel::reorderFields(const Tellico::Data::FieldList& fields_) {
  doCommand(new Command::ReorderFields(Data::Document::self()->collection(),
                                       Data::Document::self()->collection()->fields(),
                                       fields_));
}

void Kernel::appendCollection(Tellico::Data::CollPtr coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Append,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::mergeCollection(Tellico::Data::CollPtr coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Merge,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::replaceCollection(Tellico::Data::CollPtr coll_) {
  doCommand(new Command::CollectionCommand(Command::CollectionCommand::Replace,
                                           Data::Document::self()->collection(),
                                           coll_));
}

void Kernel::renameCollection() {
  bool ok;
  QString newTitle = QInputDialog::getText(m_widget, i18n("Rename Collection"), i18n("New collection name:"),
                                           QLineEdit::Normal, Data::Document::self()->collection()->title(), &ok);
  if(ok) {
    doCommand(new Command::RenameCollection(Data::Document::self()->collection(), newTitle));
  }
}

void Kernel::doCommand(QUndoCommand* command_) {
  m_commandHistory->push(command_);
}

int Kernel::askAndMerge(Tellico::Data::EntryPtr entry1_, Tellico::Data::EntryPtr entry2_, Tellico::Data::FieldPtr field_,
                        QString value1_, QString value2_) {
  QString title1 = entry1_->field(QStringLiteral("title"));
  QString title2 = entry2_->field(QStringLiteral("title"));
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
  QString text = QLatin1String("<qt>")
                + i18n("Conflicting values for %1 were found while merging entries.", field_->title())
                + QString::fromLatin1("<br/><center><table><tr>"
                                      "<th>%1</th>"
                                      "<th>%2</th></tr>").arg(title1, title2)
                + QStringLiteral("<tr><td><em>%1</em></td>").arg(value1_)
                + QStringLiteral("<td><em>%1</em></td></tr></table></center>").arg(value2_)
                + i18n("Please choose which value to keep.")
                + QLatin1String("</qt>");

  auto ret = KMessageBox::warningTwoActionsCancel(Kernel::self()->widget(),
                                                  text,
                                                  i18n("Merge Entries"),
                                                  KGuiItem(i18n("Select value from %1", title1)),
                                                  KGuiItem(i18n("Select value from %1", title2)));
  switch(ret) {
    case KMessageBox::ButtonCode::PrimaryAction:   return Merge::ConflictResolver::KeepFirst; // keep original value
    case KMessageBox::ButtonCode::SecondaryAction: return Merge::ConflictResolver::KeepSecond; // use newer value
    case KMessageBox::ButtonCode::Cancel:
    default:                                       return Merge::ConflictResolver::CancelMerge;
  }
  return Merge::ConflictResolver::CancelMerge;
}
