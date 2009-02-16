/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_LOANDIALOG_H
#define TELLICO_LOANDIALOG_H

#include "datavectors.h"
#include "borrower.h"

#include <kdialog.h>

class KLineEdit;
class KTextEdit;

class QUndoCommand;
class QCheckBox;

namespace Tellico {
  namespace GUI {
    class DateWidget;
  }

/**
 * @author Robby Stephenson
 */
class LoanDialog : public KDialog {
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

private slots:
  void slotBorrowerNameChanged(const QString& str);
  void slotGetBorrower();
  void slotLoadAddressBook();
  void slotDueDateChanged();

private:
  void init();
  QUndoCommand* addLoansCommand();
  QUndoCommand* modifyLoansCommand();

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

  QString m_uid;
};

} // end namespace
#endif
