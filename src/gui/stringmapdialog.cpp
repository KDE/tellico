/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "stringmapdialog.h"

#include <KPushButton>
#include <KLocalizedString>
#include <KGuiItem>

#include <QTreeWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>

using Tellico::StringMapDialog;

StringMapDialog::StringMapDialog(const QMap<QString, QString>& map_, QWidget* parent_, bool modal_/*=false*/)
    : QDialog(parent_) {
  setModal(modal_);

  QWidget* mainWidget = new QWidget(this);
  QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
  setLayout(mainLayout);
  mainLayout->addWidget(mainWidget);

  QWidget* page = new QWidget(mainWidget);
  QBoxLayout* l = new QVBoxLayout(page);

  m_treeWidget = new QTreeWidget(page);
  m_treeWidget->setAllColumnsShowFocus(true);
  m_treeWidget->header()->setSortIndicatorShown(true);
  m_treeWidget->setHeaderHidden(true); // hide header since neither column has a label initially
  connect(m_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), SLOT(slotUpdate(QTreeWidgetItem*)));
  connect(m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(slotUpdate(QTreeWidgetItem*)));
  l->addWidget(m_treeWidget);

  QWidget* box = new QWidget(page);
  QHBoxLayout* boxHBoxLayout = new QHBoxLayout(box);
  boxHBoxLayout->setMargin(0);
  boxHBoxLayout->setSpacing(4);
  l->addWidget(box);

  m_edit1 = new QLineEdit(box);
  boxHBoxLayout->addWidget(m_edit1);
  m_edit1->setFocus();
  m_edit2 = new QLineEdit(box);
  boxHBoxLayout->addWidget(m_edit2);

  QDialogButtonBox* bb = new QDialogButtonBox(box);
  boxHBoxLayout->addWidget(bb);
  KPushButton* pb1 = new KPushButton(KGuiItem(i18nc("set a value", "&Set"), QIcon::fromTheme(QLatin1String("document-new"))), bb);
  connect(pb1, SIGNAL(clicked()), this, SLOT(slotAdd()));
  bb->addButton(pb1, QDialogButtonBox::ActionRole);

  KPushButton* pb2 = new KPushButton(KGuiItem(i18nc("delete a value", "&Delete"), QIcon::fromTheme(QLatin1String("edit-delete"))), bb);
  connect(pb2, SIGNAL(clicked()), this, SLOT(slotDelete()));
  bb->addButton(pb2, QDialogButtonBox::ActionRole);

  l->addWidget(box);
  l->addStretch(1);
  mainLayout->addWidget(page);

  for(QMap<QString, QString>::ConstIterator it = map_.begin(); it != map_.end(); ++it) {
    if(!it.value().isEmpty()) {
      new QTreeWidgetItem(m_treeWidget, QStringList() << it.key() << it.value());
    }
  }
  m_treeWidget->resizeColumnToContents(0);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
  mainLayout->addWidget(buttonBox);

  setMinimumWidth(400);
}

void StringMapDialog::slotAdd() {
  QString s1 = m_edit1->text();
  QString s2 = m_edit2->text();
  if(s1.isEmpty() && s2.isEmpty()) {
    return;
  }
  QTreeWidgetItem* item = m_treeWidget->currentItem();
  if(item && s1 == item->data(0, Qt::DisplayRole).toString()) { // only update values if same key
    item->setData(1, Qt::DisplayRole, s2);
  } else {
    item = new QTreeWidgetItem(m_treeWidget, QStringList() << s1 << s2);
  }
  m_treeWidget->resizeColumnToContents(0);
  m_treeWidget->scrollToItem(item);
  m_treeWidget->setCurrentItem(item);
}

void StringMapDialog::slotDelete() {
  delete m_treeWidget->currentItem();
  slotUpdate(m_treeWidget->currentItem());
}

void StringMapDialog::slotUpdate(QTreeWidgetItem* item_) {
  if(item_) {
    m_edit1->setText(item_->data(0, Qt::DisplayRole).toString());
    m_edit2->setText(item_->data(1, Qt::DisplayRole).toString());
  } else {
    m_edit1->clear();
    m_edit2->clear();
  }
}

void StringMapDialog::setLabels(const QString& label1_, const QString& label2_) {
  m_treeWidget->headerItem()->setData(0, Qt::DisplayRole, label1_);
  m_treeWidget->headerItem()->setData(1, Qt::DisplayRole, label2_);
  m_treeWidget->setHeaderHidden(false);
}

QMap<QString, QString> StringMapDialog::stringMap() {
  QMap<QString, QString> map;
  for(int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
    QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
    map.insert(item->data(0, Qt::DisplayRole).toString(),
               item->data(1, Qt::DisplayRole).toString());
  }
  return map;
}
