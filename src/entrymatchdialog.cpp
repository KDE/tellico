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

#include <klocale.h>
#include <ktextedit.h>
#include <KHBox>
#include <KIconLoader>

#include <QSplitter>
#include <QLabel>
#include <QVBoxLayout>
#include <QTreeWidget>

namespace {
  static const int DIALOG_MIN_WIDTH = 600;
}

using namespace Tellico;
using Tellico::EntryMatchDialog;

EntryMatchDialog::EntryMatchDialog(QWidget* parent_, Data::EntryPtr entryToUpdate_,
                                   Fetch::Fetcher::Ptr fetcher_, const EntryUpdater::ResultList& matchResults_)
    : KDialog(parent_) {
  Q_ASSERT(entryToUpdate_);
  Q_ASSERT(fetcher_);

  setModal(true);
  setCaption(i18n("Select Match"));
  setButtons(KDialog::Ok|KDialog::Cancel);

  QWidget* mainWidget = new QWidget(this);
  setMainWidget(mainWidget);
  QBoxLayout* topLayout = new QVBoxLayout(mainWidget);

  KHBox* hbox = new KHBox(mainWidget);
  hbox->setSpacing(10);
  topLayout->addWidget(hbox);

  QLabel* icon = new QLabel(hbox);
  icon->setPixmap(Fetch::Manager::fetcherIcon(fetcher_, KIconLoader::Panel, 48));
  icon->setAlignment(Qt::Alignment(Qt::AlignLeft) | Qt::AlignTop);

  QString s = i18n("<qt><b>%1</b> returned multiple results which could match <b>%2</b>, "
                   "the entry currently in the collection. Please select the correct match.</qt>",
                   fetcher_->source(),
                   entryToUpdate_->title());

  KTextEdit* l = new KTextEdit(hbox);
  l->setHtml(s);
  l->setReadOnly(true);
  l->setMaximumHeight(48);
  l->setFrameStyle(0);

  QSplitter* split = new QSplitter(Qt::Vertical, mainWidget);
  split->setMinimumHeight(400);
  topLayout->addWidget(split);

  m_treeWidget = new QTreeWidget(split);
  m_treeWidget->setAllColumnsShowFocus(true);
  m_treeWidget->setSortingEnabled(true);
  m_treeWidget->setHeaderLabels(QStringList() << i18n("Title") << i18n("Description"));
  connect(m_treeWidget, SIGNAL(itemSelectionChanged()), SLOT(slotShowEntry()));

  foreach(const EntryUpdater::UpdateResult& res, matchResults_) {
    Data::EntryPtr matchingEntry = res.first->fetchEntry();
    QTreeWidgetItem* item = new QTreeWidgetItem(m_treeWidget, QStringList() << matchingEntry->title() << res.first->desc);
    m_itemResults.insert(item, res);
    m_itemEntries.insert(item, matchingEntry);
  }

  m_entryView = new EntryView(split);
  // don't bother creating funky gradient images for compact view
  m_entryView->setUseGradientImages(false);
  // set the xslt file AFTER setting the gradient image option
  m_entryView->setXSLTFile(QLatin1String("Compact.xsl"));
  m_entryView->addXSLTStringParam("skip-fields", "id,mdate,cdate");

  setMinimumWidth(qMax(minimumWidth(), DIALOG_MIN_WIDTH));
  // have the entry view be taller than the tree widget
  split->setStretchFactor(1, 10);
}

void EntryMatchDialog::slotShowEntry() {
  QTreeWidgetItem* item = m_treeWidget->currentItem();
  if(!item) {
    return;
  }

  m_entryView->showEntry(m_itemEntries[item]);
}


Tellico::EntryUpdater::UpdateResult EntryMatchDialog::updateResult() const {
  QTreeWidgetItem* item = m_treeWidget->currentItem();
  if(!item) {
    return EntryUpdater::UpdateResult(0, false);
  }
  return m_itemResults[item];
}

