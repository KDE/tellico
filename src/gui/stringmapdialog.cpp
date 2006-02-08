/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "stringmapdialog.h"

#include <klistview.h>
#include <klocale.h>
#include <klineedit.h>
#include <kbuttonbox.h>
#include <kiconloader.h>

#include <qlayout.h>
#include <qheader.h>
#include <qhbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

using Tellico::StringMapDialog;

StringMapDialog::StringMapDialog(const QMap<QString, QString>& map_, QWidget* parent_, const char* name_/*=0*/, bool modal_/*=false*/)
    : KDialogBase(parent_, name_, modal_, QString::null, Ok|Cancel) {
  QWidget* page = new QWidget(this);
  QVBoxLayout* l = new QVBoxLayout(page, 0, KDialog::spacingHint());

  m_listView = new KListView(page);
  m_listView->setAllColumnsShowFocus(true);
  m_listView->setShowSortIndicator(true);
  m_listView->addColumn(QString::null);
  m_listView->addColumn(QString::null);
  m_listView->header()->hide(); // hide header since neither column has a label initially
  m_listView->setColumnWidthMode(0, QListView::Maximum);
  m_listView->setColumnWidthMode(1, QListView::Maximum);
  m_listView->setResizeMode(QListView::AllColumns);
  connect(m_listView, SIGNAL(currentChanged(QListViewItem*)), SLOT(slotUpdate(QListViewItem*)));
  connect(m_listView, SIGNAL(clicked(QListViewItem*)), SLOT(slotUpdate(QListViewItem*)));
  l->addWidget(m_listView);

  QHBox* box = new QHBox(page);
  box->setMargin(4);
  box->setSpacing(KDialog::spacingHint());

  m_edit1 = new KLineEdit(box);
  m_edit1->setFocus();
  m_edit2 = new KLineEdit(box);
  KButtonBox* bb = new KButtonBox(box);
  bb->addStretch();
  QPushButton* btn = bb->addButton(i18n("&Set"), this, SLOT(slotAdd()));
  btn->setIconSet(BarIcon(QString::fromLatin1("filenew"), KIcon::SizeSmall));
  btn = bb->addButton(i18n("&Delete"), this, SLOT(slotDelete()));
  btn->setIconSet(BarIcon(QString::fromLatin1("editdelete"), KIcon::SizeSmall));

  l->addWidget(box);
  l->addStretch(1);
  setMainWidget(page);

  for(QMap<QString, QString>::ConstIterator it = map_.begin(); it != map_.end(); ++it) {
    if(!it.data().isEmpty()) {
      (void) new KListViewItem(m_listView, it.key(), it.data());
    }
  }

  setMinimumWidth(400);
  enableButtonSeparator(true);
}

void StringMapDialog::slotAdd() {
  QString s1 = m_edit1->text();
  QString s2 = m_edit2->text();
  if(s1.isEmpty() && s2.isEmpty()) {
    return;
  }
  QListViewItem* item = m_listView->currentItem();
  if(item && s1 == item->text(0)) { // only update values if same key
    item->setText(1, s2);
  } else {
    item = new KListViewItem(m_listView, s1, s2);
  }
  m_listView->ensureItemVisible(item);
  m_listView->setSelected(item, true);
  m_listView->setCurrentItem(item);
}

void StringMapDialog::slotDelete() {
  delete m_listView->currentItem();
  m_edit1->clear();
  m_edit2->clear();
  m_listView->setSelected(m_listView->currentItem(), true);
}

void StringMapDialog::slotUpdate(QListViewItem* item_) {
  if(item_) {
    m_edit1->setText(item_->text(0));
    m_edit2->setText(item_->text(1));
    m_listView->header()->adjustHeaderSize();
  } else {
    m_edit1->clear();
    m_edit2->clear();
  }
}

void StringMapDialog::setLabels(const QString& label1_, const QString& label2_) {
  m_listView->header()->setLabel(0, label1_);
  m_listView->header()->setLabel(1, label2_);
  m_listView->header()->show();
}

QMap<QString, QString> StringMapDialog::stringMap() {
  QMap<QString, QString> map;
  for(QListViewItem* item = m_listView->firstChild(); item; item = item->nextSibling()) {
    map.insert(item->text(0), item->text(1));
  }
  return map;
}

#include "stringmapdialog.moc"
