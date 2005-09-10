/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef LOANDIALOG_H
#define LOANDIALOG_H

class KLineEdit;
class KTextEdit;
class KCommand;
class QCheckBox;

#include "datavectors.h"
#include "borrower.h"

#include <kdialogbase.h>

namespace Tellico {
  namespace GUI {
    class DateWidget;
  }

/**
 * @author Robby Stephenson
 */
class LoanDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  LoanDialog(const Data::EntryVec& entries, QWidget* parent, const char* name=0);
  LoanDialog(Data::Loan* loan, QWidget* parent, const char* name=0);
  virtual ~LoanDialog();

  KCommand* createCommand();

private slots:
  void slotBorrowerNameChanged(const QString& str);
  void slotGetBorrower();
  void slotLoadAddressBook();
  void slotDueDateChanged();

private:
  void init();
  KCommand* addLoansCommand();
  KCommand* modifyLoansCommand();

  enum Mode {
    Add,
    Modify
  };

  const Mode m_mode;
  Data::BorrowerPtr m_borrower;
  Data::EntryVec m_entries;
  Data::LoanPtr m_loan;

  KLineEdit* m_borrowerEdit;
  GUI::DateWidget* m_loanDate;
  GUI::DateWidget* m_dueDate;
  KTextEdit* m_note;
  QCheckBox* m_addEvent;

  QString m_uid;
};

} // end namespace
#endif
