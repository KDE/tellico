/***************************************************************************
                            stringmacrodialog.cpp
                             -------------------
    begin                : Fri Oct 24 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "stringmacrodialog.h"
#include "collections/bibtexcollection.h"

#include <klocale.h>
#include <kbuttonbox.h>

#include <qlayout.h>
#include <qlistview.h>
#include <qmap.h>
#include <qwhatsthis.h>

StringMacroDialog::StringMacroDialog(BCCollection* coll_, QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("String Macros"), Ok),
      m_coll(dynamic_cast<BibtexCollection*>(coll_)) {
  QWidget* page = new QWidget(this);
  QVBoxLayout* l = new QVBoxLayout(page, 0, KDialog::spacingHint());

  m_listView = new QListView(page);
  m_listView->setAllColumnsShowFocus(true);
  m_listView->setShowSortIndicator(true);
  m_listView->addColumn(i18n("Macro"));
  m_listView->addColumn(i18n("String"));
  m_listView->setColumnWidthMode(0, QListView::Maximum);
  m_listView->setColumnWidthMode(1, QListView::Maximum);
  m_listView->setResizeMode(QListView::LastColumn);
  m_listView->setDefaultRenameAction(QListView::Accept);
  l->addWidget(m_listView);
  QWhatsThis::add(m_listView, i18n("Both the macro and the string can be edited by triple-clicking "
                                   "the item."));

  KButtonBox* bb = new KButtonBox(page);
  bb->addStretch();
  bb->addButton(i18n("Add"), this, SLOT(slotAdd()));
  bb->addButton(i18n("Delete"), this, SLOT(slotDelete()));
  bb->addStretch();
  l->addWidget(bb);

  l->addStretch(1);
  setMainWidget(page);

  if(m_coll) {
    StringMap macroMap = m_coll->macroList();
    for(StringMap::Iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
      // the default month macros have no data, so don't show them
      if(!it.data().isEmpty()) {
        QListViewItem* item = new QListViewItem(m_listView, it.key(), it.data());
        item->setRenameEnabled(0, true);
        item->setRenameEnabled(1, true);
      }
    }
  }
  setMinimumWidth(400);
  enableButtonSeparator(true);
  connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
}

void StringMacroDialog::slotAdd() {
  QListViewItem* item = new QListViewItem(m_listView);
  item->setRenameEnabled(0, true);
  item->setRenameEnabled(1, true);
  m_listView->ensureItemVisible(item);
  m_listView->setSelected(item, true);
}

void StringMacroDialog::slotDelete() {
  delete m_listView->currentItem();
}

void StringMacroDialog::slotOk() {
  if(m_coll) {
    // go ahead and do rename
    if(m_listView->isRenaming()) {
      m_listView->clearFocus(); // since renameAction is Accept, this enters the change
    }
    StringMap map;
    for(QListViewItem* item = m_listView->firstChild(); item; item = item->nextSibling()) {
      map.insert(item->text(0), item->text(1));
    }
    m_coll->setMacroList(map);
  }
  hide();
}
