/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "borrowerdialog.h"
#include "document.h"
#include "collection.h"

#include <klocale.h>
#include <klineedit.h>
#include <kabc/addressee.h>
#include <kabc/stdaddressbook.h>
#include <kiconloader.h>

#include <qlayout.h>

using Tellico::BorrowerDialog;

BorrowerDialog::Item::Item(KListView* parent_, const KABC::Addressee& add_)
    : KListViewItem(parent_), m_uid(add_.uid()) {
  setText(0, add_.realName());
  setPixmap(0, SmallIcon(QString::fromLatin1("kaddressbook")));
}

BorrowerDialog::Item::Item(KListView* parent_, const Data::Borrower& bor_)
    : KListViewItem(parent_), m_uid(bor_.uid()) {
  setText(0, bor_.name());
  setPixmap(0, SmallIcon(QString::fromLatin1("tellico")));
}

// default button is going to be used as a print button, so it's separated
BorrowerDialog::BorrowerDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, true, i18n("Select Borrower"), Ok|Cancel) {
  QWidget* mainWidget = new QWidget(this, "BorrowerDialog mainWidget");
  setMainWidget(mainWidget);
  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget, 0, KDialog::spacingHint());

  m_listView = new KListView(mainWidget);
  topLayout->addWidget(m_listView);
  m_listView->addColumn(i18n("Name"));
  m_listView->setFullWidth(true);
  connect(m_listView, SIGNAL(doubleClicked(QListViewItem*)), SLOT(slotOk()));
  connect(m_listView, SIGNAL(selectionChanged(QListViewItem*)), SLOT(updateEdit(QListViewItem*)));

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
  m_listView->clear();
  m_itemDict.clear();
  m_lineEdit->completionObject()->clear();

  const KABC::AddressBook* const abook = KABC::StdAddressBook::self(true);
  for(KABC::AddressBook::ConstIterator it = abook->begin(), end = abook->end();
      it != end; ++it) {
    // skip people with no name
    if((*it).realName().isEmpty()) {
      continue;
    }
    Item* item = new Item(m_listView, *it);
    m_itemDict.insert((*it).realName(), item);
    m_lineEdit->completionObject()->addItem((*it).realName());
  }

  // add current borrowers, too
  const Data::BorrowerVec& borrowers = Data::Document::self()->collection()->borrowers();
  for(Data::BorrowerVec::ConstIterator it = borrowers.constBegin(); it != borrowers.constEnd(); ++it) {
    if(m_itemDict[it->name()]) {
      continue; // if an item already exists with this name
    }
    Item* item = new Item(m_listView, *it);
    m_itemDict.insert(it->name(), item);
    m_lineEdit->completionObject()->addItem(it->name());
  }
  m_listView->setSorting(0, true);
  m_listView->sort();
}

void BorrowerDialog::selectItem(const QString& str_) {
  if(str_.isEmpty()) {
    return;
  }

  QListViewItem* item = m_itemDict.find(str_);
  if(item) {
    m_listView->blockSignals(true);
    m_listView->setSelected(item, true);
    m_listView->ensureItemVisible(item);
    m_listView->blockSignals(false);
  }
}

void BorrowerDialog::updateEdit(QListViewItem* item_) {
  m_lineEdit->setText(item_->text(0));
  m_lineEdit->setSelection(0, item_->text(0).length());
  m_uid = static_cast<Item*>(item_)->uid();
}

Tellico::Data::BorrowerPtr BorrowerDialog::borrower() {
  return new Data::Borrower(m_lineEdit->text(), m_uid);
}

// static
Tellico::Data::BorrowerPtr BorrowerDialog::getBorrower(QWidget* parent_) {
  BorrowerDialog dlg(parent_);

  if(dlg.exec() == QDialog::Accepted) {
    return dlg.borrower();
  }
  return 0;
}

#include "borrowerdialog.moc"
