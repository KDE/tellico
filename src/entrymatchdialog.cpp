/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "entrymatchdialog.h"
#include "entryview.h"
#include "entry.h"
#include "fetch/fetchmanager.h"

#include <KLocalizedString>
#include <KTextEdit>
#include <KIconLoader>

#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QDialogButtonBox>
#include <QPushButton>

namespace {
  static const int DIALOG_MIN_WIDTH = 600;
}

using namespace Tellico;
using Tellico::EntryMatchDialog;

EntryMatchDialog::EntryMatchDialog(QWidget* parent_, Data::EntryPtr entryToUpdate_,
                                   Fetch::Fetcher* fetcher_, const EntryUpdater::ResultList& matchResults_)
    : QDialog(parent_) {
  Q_ASSERT(entryToUpdate_);
  Q_ASSERT(fetcher_);

  setModal(true);
  setWindowTitle(i18n("Select Match"));

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  setLayout(mainLayout);

  QWidget* mainWidget = new QWidget(this);
  mainLayout->addWidget(mainWidget);

  QWidget* hbox = new QWidget(mainWidget);
  mainLayout->addWidget(hbox);
  QHBoxLayout* hboxHBoxLayout = new QHBoxLayout(hbox);
  hboxHBoxLayout->setContentsMargins(0, 0, 0, 0);
  hboxHBoxLayout->setSpacing(10);

  QLabel* icon = new QLabel(hbox);
  hboxHBoxLayout->addWidget(icon);
  icon->setPixmap(Fetch::Manager::fetcherIcon(fetcher_, KIconLoader::Dialog, 48));
  icon->setAlignment(Qt::Alignment(Qt::AlignLeft) | Qt::Alignment(Qt::AlignTop));

  QString s = i18n("<qt><b>%1</b> returned multiple results which could match <b>%2</b>, "
                   "the entry currently in the collection. Please select the correct match.</qt>",
                   fetcher_->source(),
                   entryToUpdate_->title());

  KTextEdit* l = new KTextEdit(hbox);
  hboxHBoxLayout->addWidget(l);
  l->setHtml(s);
  l->setReadOnly(true);
  l->setMaximumHeight(48);
  l->setFrameStyle(0);

  QSplitter* split = new QSplitter(Qt::Vertical, mainWidget);
  mainLayout->addWidget(split);
  split->setMinimumHeight(400);

  m_treeWidget = new QTreeWidget(split);
  m_treeWidget->setAllColumnsShowFocus(true);
  m_treeWidget->setSortingEnabled(true);
  m_treeWidget->setHeaderLabels(QStringList() << i18n("Title") << i18n("Description"));
  connect(m_treeWidget, &QTreeWidget::itemSelectionChanged, this, &EntryMatchDialog::slotShowEntry);

  foreach(const EntryUpdater::UpdateResult& res, matchResults_) {
    Data::EntryPtr matchingEntry = res.result->fetchEntry();
    QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget, QStringList() << matchingEntry->title() << res.result->desc);
    m_itemResults.insert(item, res);
  }

  m_entryView = new EntryView(split);
  m_entryView->setUseImageConfigLocation(true);
  // don't bother creating funky gradient images for compact view
  m_entryView->setUseGradientImages(false);
  // set the xslt file AFTER setting the gradient image option
  m_entryView->setXSLTFile(QStringLiteral("Compact.xsl"));
  m_entryView->addXSLTStringParam("skip-fields", "id,mdate,cdate");

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  mainLayout->addWidget(buttonBox);

  setMinimumWidth(qMax(minimumWidth(), DIALOG_MIN_WIDTH));
  // have the entry view be taller than the tree widget
  split->setStretchFactor(1, 10);
}

void EntryMatchDialog::slotShowEntry() {
  QTreeWidgetItem* item = m_treeWidget->currentItem();
  if(!item) {
    return;
  }

  m_entryView->showEntry(m_itemResults[item].result->fetchEntry());
}

Tellico::EntryUpdater::UpdateResult EntryMatchDialog::updateResult() const {
  QTreeWidgetItem* item = m_treeWidget->currentItem();
  if(!item) {
    return EntryUpdater::UpdateResult();
  }
  return m_itemResults[item];
}
