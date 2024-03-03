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

#include "borrowerdialog.h"
#include "document.h"
#include "collection.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KLineEdit>
#include <KJob>

#ifdef HAVE_KABC
#ifdef QT_STRICT_ITERATORS
#define WAS_STRICT
#undef QT_STRICT_ITERATORS
#endif

#include <kcontacts/addressee.h>
#include <Akonadi/Contact/ContactSearchJob>

#ifdef WAS_STRICT
#define QT_STRICT_ITERATORS
#undef WAS_STRICT
#endif
#endif

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

using Tellico::BorrowerDialog;

#ifdef HAVE_KABC
BorrowerDialog::Item::Item(QTreeWidget* parent_, const KContacts::Addressee& add_)
    : QTreeWidgetItem(parent_), m_uid(add_.uid()) {
  setData(0, Qt::DisplayRole, add_.realName().trimmed());
  setData(0, Qt::DecorationRole, QIcon::fromTheme(QLatin1String("kaddressbook")));
}
#endif

BorrowerDialog::Item::Item(QTreeWidget* parent_, const Tellico::Data::Borrower& bor_)
    : QTreeWidgetItem(parent_), m_uid(bor_.uid()) {
  setData(0, Qt::DisplayRole, bor_.name());
  setData(0, Qt::DecorationRole, QIcon::fromTheme(QStringLiteral("tellico"),
                                                  QIcon(QLatin1String(":/icons/tellico"))));
}

// default button is going to be used as a print button, so it's separated
BorrowerDialog::BorrowerDialog(QWidget* parent_)
    : QDialog(parent_) {
  setModal(true);
  setWindowTitle(i18n("Select Borrower"));

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  setLayout(mainLayout);

  QWidget* mainWidget = new QWidget(this);
  mainLayout->addWidget(mainWidget);

  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  m_treeWidget = new QTreeWidget(mainWidget);
  mainLayout->addWidget(m_treeWidget);
  m_treeWidget->setHeaderLabel(i18n("Name"));
  m_treeWidget->setRootIsDecorated(false);
  connect(m_treeWidget, &QTreeWidget::itemDoubleClicked,
          this, &QDialog::accept);
  connect(m_treeWidget, &QTreeWidget::currentItemChanged,
          this, &BorrowerDialog::updateEdit);

  m_lineEdit = new KLineEdit(mainWidget); //krazy:exclude=qclasses
  mainLayout->addWidget(m_lineEdit);
  connect(m_lineEdit->completionObject(), &KCompletion::match,
          this, &BorrowerDialog::selectItem);
  m_lineEdit->setFocus();
  m_lineEdit->completionObject()->setIgnoreCase(true);

  mainLayout->addWidget(buttonBox);

#ifdef HAVE_KABC
  // Search for all existing contacts
  Akonadi::ContactSearchJob* job = new Akonadi::ContactSearchJob();
  connect(job, SIGNAL(result(KJob*)), this, SLOT(akonadiSearchResult(KJob*)));
#endif

  populateBorrowerList();

  setMinimumWidth(400);
}

void BorrowerDialog::akonadiSearchResult(KJob* job_) {
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
    if(m_itemHash.contains(name)) {
      continue; // if an item already exists with this name
    }
    Item* item = new Item(m_treeWidget, addressee);
    m_itemHash.insert(name, item);
    m_lineEdit->completionObject()->addItem(name);
  }
#endif

  m_treeWidget->sortItems(0, Qt::AscendingOrder);
}

void BorrowerDialog::populateBorrowerList() {
  m_treeWidget->clear();
  m_itemHash.clear();
  m_lineEdit->completionObject()->clear();

  // Bug 307958 - KContacts::Addressee uids are not constant
  // so populate the borrower list with the existing borrowers
  // before adding ones from address book
  Data::BorrowerList borrowers = Data::Document::self()->collection()->borrowers();
  foreach(Data::BorrowerPtr bor, borrowers) {
    if(bor->name().isEmpty()) {
      continue;
    }
    if(m_itemHash.contains(bor->name())) {
      continue; // if an item already exists with this name
    }
    Item* item = new Item(m_treeWidget, *bor);
    m_itemHash.insert(bor->name(), item);
    m_lineEdit->completionObject()->addItem(bor->name());
  }
  m_treeWidget->sortItems(0, Qt::AscendingOrder);
}

void BorrowerDialog::selectItem(const QString& str_) {
  if(str_.isEmpty()) {
    return;
  }

  QTreeWidgetItem* item = m_itemHash.value(str_);
  if(item) {
    m_treeWidget->blockSignals(true);
    m_treeWidget->setCurrentItem(item);
    m_treeWidget->scrollToItem(item);
    m_treeWidget->blockSignals(false);
  }
}

void BorrowerDialog::updateEdit(QTreeWidgetItem* item_) {
  QString s = item_->data(0, Qt::DisplayRole).toString();
  m_lineEdit->setText(s);
  m_lineEdit->setSelection(0, s.length());
  m_uid = static_cast<Item*>(item_)->uid();
}

Tellico::Data::BorrowerPtr BorrowerDialog::borrower() {
  return Data::BorrowerPtr(new Data::Borrower(m_lineEdit->text(), m_uid));
}

// static
Tellico::Data::BorrowerPtr BorrowerDialog::getBorrower(QWidget* parent_) {
  BorrowerDialog dlg(parent_);

  if(dlg.exec() == QDialog::Accepted) {
    return dlg.borrower();
  }
  return Data::BorrowerPtr();
}
