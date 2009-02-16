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

#ifdef QT_STRICT_ITERATORS
#define WAS_STRICT
#undef QT_STRICT_ITERATORS
#endif

#include "borrowerdialog.h"
#include "document.h"
#include "collection.h"

#include <klocale.h>
#include <klineedit.h>
#include <kabc/addressee.h>
#include <kabc/stdaddressbook.h>

#include <QVBoxLayout>

#ifdef WAS_STRICT
#define QT_STRICT_ITERATORS
#undef WAS_STRICT
#endif

using Tellico::BorrowerDialog;

BorrowerDialog::Item::Item(QTreeWidget* parent_, const KABC::Addressee& add_)
    : QTreeWidgetItem(parent_), m_uid(add_.uid()) {
  setData(0, Qt::DisplayRole, add_.realName());
  setData(0, Qt::DecorationRole, KIcon(QLatin1String("kaddressbook")));
}

BorrowerDialog::Item::Item(QTreeWidget* parent_, const Tellico::Data::Borrower& bor_)
    : QTreeWidgetItem(parent_), m_uid(bor_.uid()) {
  setData(0, Qt::DisplayRole, bor_.name());
  setData(0, Qt::DecorationRole, KIcon(QLatin1String("tellico")));
}

// default button is going to be used as a print button, so it's separated
BorrowerDialog::BorrowerDialog(QWidget* parent_)
    : KDialog(parent_) {
  setModal(true);
  setCaption(i18n("Select Borrower"));
  setButtons(Ok | Cancel);

  QWidget* mainWidget = new QWidget(this);
  setMainWidget(mainWidget);
  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);

  m_treeWidget = new QTreeWidget(mainWidget);
  topLayout->addWidget(m_treeWidget);
  m_treeWidget->setHeaderLabel(i18n("Name"));
  connect(m_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*)),
          SLOT(accept()));
  connect(m_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
          SLOT(updateEdit(QTreeWidgetItem*)));

  m_lineEdit = new KLineEdit(mainWidget);
  topLayout->addWidget(m_lineEdit);
  connect(m_lineEdit->completionObject(), SIGNAL(match(const QString&)),
          SLOT(selectItem(const QString&)));
  m_lineEdit->setFocus();
  m_lineEdit->completionObject()->setIgnoreCase(true);

  KABC::AddressBook* abook = KABC::StdAddressBook::self(true);
  connect(abook, SIGNAL(addressBookChanged(AddressBook*)),
          SLOT(slotLoadAddressBook()));
  connect(abook, SIGNAL(loadingFinished(Resource*)),
          SLOT(slotLoadAddressBook()));
  slotLoadAddressBook();

  setMinimumWidth(400);
}

void BorrowerDialog::slotLoadAddressBook() {
  m_treeWidget->clear();
  m_itemHash.clear();
  m_lineEdit->completionObject()->clear();

  const KABC::AddressBook* const abook = KABC::StdAddressBook::self(true);
  for(KABC::AddressBook::ConstIterator it = abook->begin(), end = abook->end();
      it != end; ++it) {
    // skip people with no name
    if((*it).realName().isEmpty()) {
      continue;
    }
    Item* item = new Item(m_treeWidget, *it);
    m_itemHash.insert((*it).realName(), item);
    m_lineEdit->completionObject()->addItem((*it).realName());
  }

  // add current borrowers, too
  Data::BorrowerList borrowers = Data::Document::self()->collection()->borrowers();
  foreach(Data::BorrowerPtr bor, borrowers) {
    if(m_itemHash[bor->name()]) {
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

#include "borrowerdialog.moc"
