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

#ifndef TELLICO_BORROWER_H
#define TELLICO_BORROWER_H

#include "datavectors.h"
#include "entry.h"

#include <QDate>
#include <QString>
#include <QExplicitlySharedDataPointer>

namespace Tellico {
  namespace Data {

class Loan : public QSharedData {

public:
  Loan(Data::EntryPtr entry, QDate loanDate, QDate dueDate, const QString& note);
  Loan(const Loan& other);

  Data::BorrowerPtr borrower() const;
  void setBorrower(Data::BorrowerPtr b) { m_borrower = b; }

  const QString& uid() const { return m_uid; }
  void setUID(const QString& uid) { m_uid = uid; }

  Data::EntryPtr entry() const;

  const QDate& loanDate() const { return m_loanDate; }

  const QDate& dueDate() const { return m_dueDate; }
  void setDueDate(QDate date) { m_dueDate = date; }

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

typedef QExplicitlySharedDataPointer<Loan> LoanPtr;
typedef QList<LoanPtr> LoanList;

/**
 * @author Robby Stephenson
 */
class Borrower : public QSharedData {

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

  bool hasEntry(Data::EntryPtr entry);

private:
  QString m_name;
  QString m_uid; // uid used by KABC
  LoanList m_loans;
};

  } // end namespace
} // end namespace
#endif
