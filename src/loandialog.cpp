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

#include <config.h>

#include "loandialog.h"
#include "borrowerdialog.h"
#include "gui/datewidget.h"
#include "collection.h"
#include "commands/addloans.h"
#include "commands/modifyloans.h"
#include "utils/string_utils.h"
#include "tellico_debug.h"

#include <KLocalizedString>
#include <KLineEdit>
#include <KTextEdit>
#include <KJob>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KWindowConfig>

#ifdef HAVE_KABC
#include <kcontacts/addressee.h>
#include <Akonadi/Contact/ContactSearchJob>
#endif

#include <QLabel>
#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTimer>

namespace {
  static const char* dialogOptionsString = "Loan Dialog Options";
}

using Tellico::LoanDialog;

LoanDialog::LoanDialog(const Tellico::Data::EntryList& entries_, QWidget* parent_)
    : QDialog(parent_),
      m_mode(Add), m_borrower(nullptr), m_entries(entries_), m_loan(nullptr) {
  setModal(true);
  setWindowTitle(i18n("Loan Dialog"));

  init();
}

LoanDialog::LoanDialog(Tellico::Data::LoanPtr loan_, QWidget* parent_)
    : QDialog(parent_),
      m_mode(Modify), m_borrower(loan_->borrower()), m_loan(loan_) {
  m_entries.append(m_loan->entry());

  setModal(true);
  setWindowTitle(i18n("Modify Loan"));

  init();

  m_borrowerEdit->setText(m_loan->borrower()->name());
  m_loanDate->setDate(m_loan->loanDate());
  if(m_loan->dueDate().isValid()) {
    m_dueDate->setDate(m_loan->dueDate());
#ifdef HAVE_KCAL
    m_addEvent->setEnabled(true);
    if(m_loan->inCalendar()) {
      m_addEvent->setChecked(true);
    }
#endif
  }
  m_note->setPlainText(m_loan->note());
}

void LoanDialog::init() {
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  setLayout(mainLayout);

  QWidget* mainWidget = new QWidget(this);
  mainLayout->addWidget(mainWidget);

  QGridLayout* topLayout = new QGridLayout(mainWidget);
  // middle column gets to be the widest
  topLayout->setColumnStretch(1, 1);

  int row = -1;

  QLabel* pixLabel = new QLabel(mainWidget);
  mainLayout->addWidget(pixLabel);
  pixLabel->setPixmap(QIcon::fromTheme(QStringLiteral("tellico"),
                                       QIcon(QLatin1String(":/icons/tellico")))
                                      .pixmap(QSize(64, 64)));
  pixLabel->setAlignment(Qt::Alignment(Qt::AlignLeft) | Qt::Alignment(Qt::AlignTop));
  topLayout->addWidget(pixLabel, ++row, 0);

  QString entryString = QStringLiteral("<qt><p>");
  if(m_mode == Add) {
    entryString += i18n("The following items are being checked out:");
    entryString += QLatin1String("</p><ol>");
    foreach(Data::EntryPtr entry, m_entries) {
      entryString += QLatin1String("<li>") + entry->title() + QLatin1String("</li>");
    }
  } else {
    entryString += i18n("The following item is on-loan:");
    entryString += QLatin1String("</p><ol>");
    entryString += QLatin1String("<li>") + m_loan->entry()->title() + QLatin1String("</li>");
  }
  entryString += QLatin1String("</ol></qt>");
  KTextEdit* entryLabel = new KTextEdit(mainWidget);
  mainLayout->addWidget(entryLabel);
  entryLabel->setHtml(entryString);
  entryLabel->setReadOnly(true);
  topLayout->addWidget(entryLabel, row, 1, 1, 2);

  QLabel* l = new QLabel(i18n("&Lend to:"), mainWidget);
  mainLayout->addWidget(l);
  topLayout->addWidget(l, ++row, 0);
  m_borrowerEdit = new KLineEdit(mainWidget); //krazy:exclude=qclasses
  mainLayout->addWidget(m_borrowerEdit);
  topLayout->addWidget(m_borrowerEdit, row, 1);
  l->setBuddy(m_borrowerEdit);
  m_borrowerEdit->completionObject()->setIgnoreCase(true);
  connect(m_borrowerEdit, &QLineEdit::textChanged,
          this, &LoanDialog::slotBorrowerNameChanged);
  QPushButton* pb = new QPushButton(QIcon::fromTheme(QStringLiteral("kaddressbook")), QString(), mainWidget);
  mainLayout->addWidget(pb);
  topLayout->addWidget(pb, row, 2);
  connect(pb, &QAbstractButton::clicked, this, &LoanDialog::slotGetBorrower);
  QString whats = i18n("Enter the name of the person borrowing the items from you. "
                       "Clicking the button allows you to select from your address book.");
  l->setWhatsThis(whats);
  // only enable for new loans
  if(m_mode == Modify) {
    m_borrowerEdit->setEnabled(false);
    pb->setEnabled(false);
  }

  l = new QLabel(i18n("&Loan date:"), mainWidget);
  mainLayout->addWidget(l);
  topLayout->addWidget(l, ++row, 0);
  m_loanDate = new GUI::DateWidget(mainWidget);
  m_loanDate->setDate(QDate::currentDate());
  l->setBuddy(m_loanDate);
  topLayout->addWidget(m_loanDate, row, 1, 1, 2);
  whats = i18n("The check-out date is the date that you lent the items. By default, "
               "today's date is used.");
  l->setWhatsThis(whats);
  m_loanDate->setWhatsThis(whats);
  // only enable for new loans
  if(m_mode == Modify) {
    m_loanDate->setEnabled(false);
  }

  l = new QLabel(i18n("D&ue date:"), mainWidget);
  mainLayout->addWidget(l);
  topLayout->addWidget(l, ++row, 0);
  m_dueDate = new GUI::DateWidget(mainWidget);
  l->setBuddy(m_dueDate);
  topLayout->addWidget(m_dueDate, row, 1, 1, 2);
  // valid due dates will enable the calendar adding checkbox
  connect(m_dueDate, &GUI::DateWidget::signalModified, this, &LoanDialog::slotDueDateChanged);
  whats = i18n("The due date is when the items are due to be returned. The due date "
               "is not required, unless you want to add the loan to your active calendar.");
  l->setWhatsThis(whats);
  m_dueDate->setWhatsThis(whats);

  l = new QLabel(i18n("&Note:"), mainWidget);
  mainLayout->addWidget(l);
  topLayout->addWidget(l, ++row, 0);
  m_note = new KTextEdit(mainWidget);
  mainLayout->addWidget(m_note);
  l->setBuddy(m_note);
  topLayout->addWidget(m_note, row, 1, 1, 2);
  topLayout->setRowStretch(row, 1);
  whats = i18n("You can add notes about the loan.");
  l->setWhatsThis(whats);
  m_note->setWhatsThis(whats);

  m_addEvent = new QCheckBox(i18n("&Add a reminder to the active calendar"), mainWidget);
  mainLayout->addWidget(m_addEvent);
  topLayout->addWidget(m_addEvent, ++row, 0, 1, 3);
  m_addEvent->setEnabled(false); // gets enabled when valid due date is entered
  m_addEvent->setWhatsThis(i18n("<qt>Checking this box will add a <em>To-do</em> item "
                                   "to your active calendar, which can be viewed using KOrganizer. "
                                   "The box is only active if you set a due date.</qt>"));

#ifdef HAVE_KABC
  // Search for all existing contacts
  Akonadi::ContactSearchJob* job = new Akonadi::ContactSearchJob();
  connect(job, SIGNAL(result(KJob*)), this, SLOT(akonadiSearchResult(KJob*)));
#endif
  populateBorrowerList();

  m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QPushButton* okButton = m_buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  okButton->setEnabled(false); // disable until a name is entered
  connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  mainLayout->addWidget(m_buttonBox);

  QTimer::singleShot(0, this, &LoanDialog::slotUpdateSize);
}

LoanDialog::~LoanDialog() {
  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Loan Dialog Options"));
  KWindowConfig::saveWindowSize(windowHandle(), config);
}

void LoanDialog::slotBorrowerNameChanged(const QString& str_) {
  m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!str_.isEmpty());
}

void LoanDialog::slotDueDateChanged() {
#ifdef HAVE_KCAL
  m_addEvent->setEnabled(m_dueDate->date().isValid());
#endif
}

void LoanDialog::slotGetBorrower() {
  Data::BorrowerPtr borrower = BorrowerDialog::getBorrower(this);
  if(borrower) {
    m_borrowerEdit->setText(borrower->name());
    m_uid = borrower->uid();
  }
}

void LoanDialog::akonadiSearchResult(KJob* job_) {
  if(job_->error() != 0) {
    myDebug() << job_->errorString();
    return;
  }

#ifdef HAVE_KABC
  Akonadi::ContactSearchJob* searchJob = qobject_cast<Akonadi::ContactSearchJob*>(job_);
  Q_ASSERT(searchJob);

  populateBorrowerList();

  foreach(const KContacts::Addressee& addressee, searchJob->contacts()) {
    // skip people with no name
    const QString name = addressee.realName().trimmed();
    if(name.isEmpty()) {
      continue;
    }
    m_borrowerEdit->completionObject()->addItem(name);
  }
#endif
}

void LoanDialog::populateBorrowerList() {
  m_borrowerEdit->completionObject()->clear();

  // add current borrowers
  Data::BorrowerList borrowers = m_entries.at(0)->collection()->borrowers();
  foreach(Data::BorrowerPtr borrower, borrowers) {
    m_borrowerEdit->completionObject()->addItem(borrower->name());
  }
}

QUndoCommand* LoanDialog::createCommand() {
  // first, check to see if the borrower is empty
  QString name = m_borrowerEdit->text();
  if(name.isEmpty()) {
    return nullptr;
  }

  // ok, first handle creating new loans
  if(m_mode == Add) {
    return addLoansCommand();
  } else {
    return modifyLoansCommand();
  }
}

QUndoCommand* LoanDialog::addLoansCommand() {
  if(m_entries.isEmpty()) {
    return nullptr;
  }

  const QString name = m_borrowerEdit->text();

  // see if there's a borrower with this name already
  m_borrower = nullptr;
  Data::BorrowerList borrowers = m_entries.at(0)->collection()->borrowers();
  foreach(Data::BorrowerPtr borrower, borrowers) {
    if(borrower->name() == name) {
      m_borrower = borrower;
      break;
    }
  }

  if(!m_borrower) {
    if(m_uid.isEmpty()) {
      m_uid = Tellico::uid();
    }
    m_borrower = new Data::Borrower(name, m_uid);
  }

  Data::LoanList loans;
  foreach(Data::EntryPtr entry, m_entries) {
    loans.append(Data::LoanPtr(new Data::Loan(entry, m_loanDate->date(), m_dueDate->date(), m_note->toPlainText())));
  }

  return new Command::AddLoans(m_borrower, loans, m_addEvent->isChecked());
}

QUndoCommand* LoanDialog::modifyLoansCommand() {
  if(!m_loan) {
    return nullptr;
  }

  Data::LoanPtr newLoan(new Data::Loan(*m_loan));
  newLoan->setDueDate(m_dueDate->date());
  newLoan->setNote(m_note->toPlainText());
  return new Command::ModifyLoans(m_loan, newLoan, m_addEvent->isChecked());
}

void LoanDialog::slotUpdateSize() {
  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String(dialogOptionsString));
  KWindowConfig::restoreWindowSize(windowHandle(), config);
}
