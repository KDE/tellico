/***************************************************************************
    Copyright (C) 2002-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entryiconview.h"
#include "collection.h"
#include "collectionfactory.h"
#include "images/imagefactory.h"
#include "controller.h"
#include "entry.h"
#include "field.h"
#include "document.h"
#include "tellico_utils.h"
#include "tellico_debug.h"
#include "models/entrytitlemodel.h"
#include "models/entrysortmodel.h"

#include <kmenu.h>
#include <klocale.h>
#include <kicon.h>

#include <QPainter>
#include <QPixmap>
#include <QContextMenuEvent>

namespace {
  static const int ENTRY_ICON_SIZE_PAD = 6;
  static const int ENTRY_ICON_SHADOW_RIGHT = 1;
  static const int ENTRY_ICON_SHADOW_BOTTOM = 1;
}

using Tellico::EntryIconView;

EntryIconView::EntryIconView(QWidget* parent_)
    : QListView(parent_), m_maxAllowedIconWidth(MAX_ENTRY_ICON_SIZE) {
  setViewMode(QListView::IconMode);
  setMovement(QListView::Static);
//  setUniformItemSizes(true);
  setDragEnabled(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setResizeMode(QListView::Adjust);
  setWordWrap(true);
  setSpacing(ENTRY_ICON_SIZE_PAD);

  EntryTitleModel* baseModel = new EntryTitleModel(this);
  EntrySortModel* sortModel = new EntrySortModel(this);
  sortModel->setSortRole(EntryPtrRole);
  sortModel->setSourceModel(baseModel);
  setModel(sortModel);

  connect(this, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(slotDoubleClicked(const QModelIndex&)));

  setWhatsThis(i18n("<qt>The <i>Icon View</i> shows each entry in the collection or group using "
                    "an icon, which may be an image in the entry.</qt>"));
}

EntryIconView::~EntryIconView() {
}

Tellico::EntrySortModel* EntryIconView::sortModel() const {
  return static_cast<EntrySortModel*>(model());
}

Tellico::AbstractEntryModel* EntryIconView::sourceModel() const {
  return static_cast<AbstractEntryModel*>(sortModel()->sourceModel());
}

void EntryIconView::setMaxAllowedIconWidth(int width_) {
  m_maxAllowedIconWidth = qBound(MIN_ENTRY_ICON_SIZE, width_, MAX_ENTRY_ICON_SIZE);
  QSize iconSize(m_maxAllowedIconWidth, m_maxAllowedIconWidth);
  setIconSize(iconSize);

  QSize gridSize(m_maxAllowedIconWidth + 2*ENTRY_ICON_SIZE_PAD,
                 m_maxAllowedIconWidth + 3*(fontMetrics().lineSpacing() + ENTRY_ICON_SIZE_PAD));
  setGridSize(gridSize);

  refresh();
}

void EntryIconView::clear() {
  sourceModel()->clear();
}

void EntryIconView::refresh() {
  sourceModel()->reset();
}

void EntryIconView::showEntries(const Tellico::Data::EntryList& entries_) {
  sourceModel()->setEntries(entries_);
}

void EntryIconView::addEntries(Tellico::Data::EntryList entries_) {
  sourceModel()->addEntries(entries_);
}

void EntryIconView::modifyEntries(Tellico::Data::EntryList entries_) {
  sourceModel()->modifyEntries(entries_);
}

void EntryIconView::removeEntries(Tellico::Data::EntryList entries_) {
  sourceModel()->removeEntries(entries_);
}

void EntryIconView::selectionChanged(const QItemSelection& selected_, const QItemSelection& deselected_) {
  QAbstractItemView::selectionChanged(selected_, deselected_);
  Data::EntryList entries;
  // ignore the selected_ and deselected_, just use selection model
  foreach(const QModelIndex& index, selectionModel()->selectedIndexes()) {
    Data::EntryPtr tmp = model()->data(index, EntryPtrRole).value<Data::EntryPtr>();
    if(tmp) {
      entries += tmp;
    }
  }
  Controller::self()->slotUpdateSelection(this, entries);
}

void EntryIconView::slotDoubleClicked(const QModelIndex& index_) {
  Data::EntryPtr entry = sourceModel()->data(index_, EntryPtrRole).value<Data::EntryPtr>();
  if(entry) {
    Controller::self()->editEntry(entry);
  }
}

void EntryIconView::contextMenuEvent(QContextMenuEvent* ev_) {
  KMenu menu(this);

  // only insert entry items if one is selected
  QModelIndex index = indexAt(ev_->pos());
  if(index.isValid()) {
    Controller::self()->plugEntryActions(&menu);
    menu.addSeparator();
  }

  QMenu* sortMenu = menu.addMenu(i18n("&Sort By"));
  foreach(Data::FieldPtr field, Data::Document::self()->collection()->fields()) {
    sortMenu->addAction(field->title())->setData(qVariantFromValue(field));
  }
  connect(sortMenu, SIGNAL(triggered(QAction*)), SLOT(slotSortMenuActivated(QAction*)));

  menu.exec(ev_->globalPos());
}

void EntryIconView::slotSortMenuActivated(QAction* action_) {
  Data::FieldPtr field = action_->data().value<Data::FieldPtr>();
  Q_ASSERT(field);
  if(!field) {
    return;
  }
  // could have just put the index of the field in the list as the actionn data
  // but instead, we need to to iterate over the current fields and find the index since EntryTitleModel
  // uses the field list index as the column value
  Data::FieldList fields = Data::Document::self()->collection()->fields();
  for(int i = 0; i < fields.count(); ++i) {
    if(fields.at(i)->name() == field->name()) {
      model()->sort(i);
      break;
    }
  }
}

#include "entryiconview.moc"
