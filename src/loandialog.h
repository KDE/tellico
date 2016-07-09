/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_LOANDIALOG_H
#define TELLICO_LOANDIALOG_H

#include "datavectors.h"
#include "borrower.h"

#include <QDialog>

class KLineEdit;
class KTextEdit;
class KJob;

class QUndoCommand;
class QCheckBox;
class QDialogButtonBox;

namespace Tellico {
  namespace GUI {
    class DateWidget;
  }

/**
 * @author Robby Stephenson
 */
class LoanDialog : public QDialog {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  LoanDialog(const Data::EntryList& entries, QWidget* parent);
  LoanDialog(Data::LoanPtr loan, QWidget* parent);
  virtual ~LoanDialog();

  QUndoCommand* createCommand();

private Q_SLOTS:
  void slotBorrowerNameChanged(const QString& str);
  void slotGetBorrower();
  void slotDueDateChanged();
  void akonadiSearchResult(KJob*);

private:
  void init();
  QUndoCommand* addLoansCommand();
  QUndoCommand* modifyLoansCommand();
  void populateBorrowerList();

  enum Mode {
    Add,
    Modify
  };

  const Mode m_mode;
  Data::BorrowerPtr m_borrower;
  Data::EntryList m_entries;
  Data::LoanPtr m_loan;

  KLineEdit* m_borrowerEdit;
  GUI::DateWidget* m_loanDate;
  GUI::DateWidget* m_dueDate;
  KTextEdit* m_note;
  QCheckBox* m_addEvent;
  QDialogButtonBox* m_buttonBox;

  QString m_uid;
};

} // end namespace
#endif
