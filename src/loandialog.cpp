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

#include "loandialog.h"
#include "borrowerdialog.h"
#include "gui/datewidget.h"
#include "gui/richtextlabel.h"
#include "collection.h"
#include "commands/addloans.h"
#include "commands/modifyloans.h"

#include <config.h>

#include <klocale.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <ktextedit.h>
#include <kiconloader.h>
#include <kabc/stdaddressbook.h>

#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

using Tellico::LoanDialog;

LoanDialog::LoanDialog(const Data::EntryVec& entries_, QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, true, i18n("Loan Dialog"), Ok|Cancel),
      m_mode(Add), m_borrower(0), m_entries(entries_), m_loan(0) {
  init();
}

LoanDialog::LoanDialog(Data::Loan* loan_, QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, true, i18n("Modify Loan"), Ok|Cancel),
      m_mode(Modify), m_borrower(loan_->borrower()), m_loan(loan_) {
  m_entries.append(m_loan->entry());

  init();

  m_borrowerEdit->setText(m_loan->borrower()->name());
  m_loanDate->setDate(m_loan->loanDate());
  m_dueDate->setDate(m_loan->dueDate());
  if(m_loan->dueDate().isValid()) {
    m_addEvent->setEnabled(true);
    if(m_loan->inCalendar()) {
      m_addEvent->setChecked(true);
    }
  }
  m_note->setText(m_loan->note());
}

void LoanDialog::init() {
  QWidget* mainWidget = new QWidget(this, "LoanDialog mainWidget");
  setMainWidget(mainWidget);
  QGridLayout* topLayout = new QGridLayout(mainWidget, 7, 2, 0, KDialog::spacingHint());

  QHBox* hbox = new QHBox(mainWidget);
  hbox->setSpacing(KDialog::spacingHint());
  QLabel* pixLabel = new QLabel(hbox);
  pixLabel->setPixmap(DesktopIcon(QString::fromLatin1("tellico"), 64));
  pixLabel->setAlignment(Qt::AlignAuto | Qt::AlignTop);
  hbox->setStretchFactor(pixLabel, 0);

  QString entryString = QString::fromLatin1("<qt><p>");
  if(m_mode == Add) {
    entryString += i18n("The following items are being checked out:");
    entryString += QString::fromLatin1("</p><ol>");
    for(Data::EntryVec::ConstIterator entry = m_entries.constBegin(); entry != m_entries.constEnd(); ++entry) {
      entryString += QString::fromLatin1("<li>") + entry->title() + QString::fromLatin1("</li>");
    }
  } else {
    entryString += i18n("The following item is on-loan:");
    entryString += QString::fromLatin1("</p><ol>");
    entryString += QString::fromLatin1("<li>") + m_loan->entry()->title() + QString::fromLatin1("</li>");
  }
  entryString += QString::fromLatin1("</ol></qt>");
  GUI::RichTextLabel* entryLabel = new GUI::RichTextLabel(entryString, hbox);
  hbox->setStretchFactor(entryLabel, 1);

  topLayout->addMultiCellWidget(hbox, 0, 0, 0, 1);

  QLabel* l = new QLabel(i18n("&Lend to:"), mainWidget);
  topLayout->addWidget(l, 1, 0);
  hbox = new QHBox(mainWidget);
  hbox->setSpacing(KDialog::spacingHint());
  topLayout->addWidget(hbox, 1, 1);
  m_borrowerEdit = new KLineEdit(hbox);
  l->setBuddy(m_borrowerEdit);
  m_borrowerEdit->completionObject()->setIgnoreCase(true);
  connect(m_borrowerEdit, SIGNAL(textChanged(const QString&)),
          SLOT(slotBorrowerNameChanged(const QString&)));
  actionButton(Ok)->setEnabled(false); // disable until a name is entered
  KPushButton* pb = new KPushButton(SmallIconSet(QString::fromLatin1("kaddressbook")), QString::null, hbox);
  connect(pb, SIGNAL(clicked()), SLOT(slotGetBorrower()));
  QString whats = i18n("Enter the name of the person borrowing the items from you. "
                       "Clicking the button allows you to select from your address book.");
  QWhatsThis::add(l, whats);
  QWhatsThis::add(hbox, whats);
  // only enable for new loans
  if(m_mode == Modify) {
    m_borrowerEdit->setEnabled(false);
  }

  l = new QLabel(i18n("&Loan date:"), mainWidget);
  topLayout->addWidget(l, 2, 0);
  m_loanDate = new GUI::DateWidget(mainWidget);
  m_loanDate->setDate(QDate::currentDate());
  l->setBuddy(m_loanDate);
  topLayout->addWidget(m_loanDate, 2, 1);
  whats = i18n("The check-out date is the date that you lent the items. By default, "
               "today's date is used.");
  QWhatsThis::add(l, whats);
  QWhatsThis::add(m_loanDate, whats);
  // only enable for new loans
  if(m_mode == Modify) {
    m_loanDate->setEnabled(false);
  }

  l = new QLabel(i18n("D&ue date:"), mainWidget);
  topLayout->addWidget(l, 3, 0);
  m_dueDate = new GUI::DateWidget(mainWidget);
  l->setBuddy(m_dueDate);
  topLayout->addWidget(m_dueDate, 3, 1);
  // valid due dates will enable the calendar adding checkbox
  connect(m_dueDate, SIGNAL(signalModified()), SLOT(slotDueDateChanged()));
  whats = i18n("The due date is when the items are due to be returned. The due date "
               "is not required, unless you want to add the loan to your active calendar.");
  QWhatsThis::add(l, whats);
  QWhatsThis::add(m_dueDate, whats);

  l = new QLabel(i18n("&Note:"), mainWidget);
  topLayout->addWidget(l, 4, 0);
  m_note = new KTextEdit(mainWidget);
  l->setBuddy(m_note);
  topLayout->addMultiCellWidget(m_note, 5, 5, 0, 1);
  topLayout->setRowStretch(5, 1);
  whats = i18n("You can add notes about the loan, as well.");
  QWhatsThis::add(l, whats);
  QWhatsThis::add(m_note, whats);

  m_addEvent = new QCheckBox(i18n("&Add a reminder to the active calendar"), mainWidget);
  topLayout->addMultiCellWidget(m_addEvent, 6, 6, 0, 1);
  m_addEvent->setEnabled(false); // gets enabled when valid due date is entered
  QWhatsThis::add(m_addEvent, i18n("<qt>Checking this box will add a <em>To-do</em> item "
                                   "to your active calendar, which can be viewed using KOrganizer. "
                                   "The box is only active if you set a due date."));

  resize(configDialogSize(QString::fromLatin1("Loan Dialog Options")));

  KABC::AddressBook* abook = KABC::StdAddressBook::self(true);
  connect(abook, SIGNAL(addressBookChanged(AddressBook*)),
          SLOT(slotLoadAddressBook()));
  connect(abook, SIGNAL(loadingFinished(Resource*)),
          SLOT(slotLoadAddressBook()));
  slotLoadAddressBook();
}

LoanDialog::~LoanDialog() {
  saveDialogSize(QString::fromLatin1("Loan Dialog Options"));
}

void LoanDialog::slotBorrowerNameChanged(const QString& str_) {
  actionButton(Ok)->setEnabled(!str_.isEmpty());
}

void LoanDialog::slotDueDateChanged() {
#if HAVE_KCAL
  m_addEvent->setEnabled(m_dueDate->date().isValid());
#endif
}

void LoanDialog::slotGetBorrower() {
  Data::Borrower* borrower = BorrowerDialog::getBorrower(this);
  if(borrower) {
    m_borrowerEdit->setText(borrower->name());
    m_uid = borrower->uid();
  }
}

void LoanDialog::slotLoadAddressBook() {
  m_borrowerEdit->completionObject()->clear();

  const KABC::AddressBook* const abook = KABC::StdAddressBook::self(true);
  for(KABC::AddressBook::ConstIterator it = abook->begin(), end = abook->end();
      it != end; ++it) {
    m_borrowerEdit->completionObject()->addItem((*it).realName());
  }

  // add current borrowers, too
  QStringList items = m_borrowerEdit->completionObject()->items();
  Data::BorrowerVec borrowers = m_entries.begin()->collection()->borrowers();
  for(Data::BorrowerVec::ConstIterator it = borrowers.constBegin(), end = borrowers.constEnd();
      it != end; ++it) {
    if(items.findIndex(it->name()) == -1) {
      m_borrowerEdit->completionObject()->addItem(it->name());
    }
  }
}

KCommand* LoanDialog::createCommand() {
  // first, check to see if the borrower is empty
  QString name = m_borrowerEdit->text();
  if(name.isEmpty()) {
    return 0;
  }

  // ok, first handle creating new loans
  if(m_mode == Add) {
    return addLoansCommand();
  } else {
    return modifyLoansCommand();
  }
}

KCommand* LoanDialog::addLoansCommand() {
  if(m_entries.isEmpty()) {
    return 0;
  }

  const QString name = m_borrowerEdit->text();

  // see if there's a borrower with this name already
  m_borrower = 0;
  Data::BorrowerVec borrowers = m_entries.begin()->collection()->borrowers();
  for(Data::BorrowerVec::Iterator it = borrowers.begin(); it != borrowers.end(); ++it) {
    if(it->name() == name) {
      m_borrower = it;
      break;
    }
  }

  if(!m_borrower) {
    m_borrower = new Data::Borrower(name, m_uid);
  }

  Data::LoanVec loans;
  for(Data::EntryVecIt entry = m_entries.begin(); entry != m_entries.end(); ++entry) {
    loans.append(new Data::Loan(entry, m_loanDate->date(), m_dueDate->date(), m_note->text()));
  }

  return new Command::AddLoans(m_borrower, loans, m_addEvent->isChecked());
}

KCommand* LoanDialog::modifyLoansCommand() {
  if(!m_loan) {
    return 0;
  }

  Data::Loan* newLoan = new Data::Loan(*m_loan);
  newLoan->setDueDate(m_dueDate->date());
  newLoan->setNote(m_note->text());
  return new Command::ModifyLoans(m_loan, newLoan, m_addEvent->isChecked());
}

#include "loandialog.moc"
