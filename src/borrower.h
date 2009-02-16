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

#ifndef TELLICO_BORROWER_H
#define TELLICO_BORROWER_H

#include "datavectors.h"
#include "entry.h"

#include <ksharedptr.h>

#include <QDate>
#include <QString>

namespace Tellico {
  namespace Data {

class Loan : public KShared {

public:
  Loan(Data::EntryPtr entry, const QDate& loanDate, const QDate& dueDate, const QString& note);
  Loan(const Loan& other);

  Data::BorrowerPtr borrower() const;
  void setBorrower(Data::BorrowerPtr b) { m_borrower = b; }

  const QString& uid() const { return m_uid; }
  void setUID(const QString& uid) { m_uid = uid; }

  Data::EntryPtr entry() const;

  const QDate& loanDate() const { return m_loanDate; }

  const QDate& dueDate() const { return m_dueDate; }
  void setDueDate(const QDate& date) { m_dueDate = date; }

  const QString& note() const { return m_note; }
  void setNote(const QString& text) { m_note = text; }

  bool inCalendar() const { return m_inCalendar; }
  void setInCalendar(bool inCalendar) { m_inCalendar = inCalendar; }

private:
  Loan& operator=(const Loan&);

  QString m_uid;
  BorrowerPtr m_borrower;
  EntryPtr m_entry;
  QDate m_loanDate;
  QDate m_dueDate;
  QString m_note;
  bool m_inCalendar;
};

typedef KSharedPtr<Loan> LoanPtr;
typedef QList<LoanPtr> LoanList;

/**
 * @author Robby Stephenson
 */
class Borrower : public KShared {

public:
  Borrower(const QString& name, const QString& uid);
  Borrower(const Borrower& other);
  Borrower& operator=(const Borrower& other);

  const QString& uid() const { return m_uid; }
  const QString& name() const { return m_name; }
  const LoanList& loans() const { return m_loans; }
  bool isEmpty() const { return m_loans.isEmpty(); }
  int count() const { return m_loans.count(); }

  Data::LoanPtr loan(Data::EntryPtr entry);
  void addLoan(Data::LoanPtr loan);
  bool removeLoan(Data::LoanPtr loan);

private:
  QString m_name;
  QString m_uid; // uid used by KABC
  LoanList m_loans;
};

  } // end namespace
} // end namespace
#endif
