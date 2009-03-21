/***************************************************************************
    copyright            : (C) 2002-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entryiconview.h"
#include "collection.h"
#include "collectionfactory.h"
#include "imagefactory.h"
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
  static const int MIN_ENTRY_ICON_SIZE = 32;
  static const int MAX_ENTRY_ICON_SIZE = 128;
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
  sortModel->setSourceModel(baseModel);
  setModel(sortModel);

  connect(this, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(slotDoubleClicked(const QModelIndex&)));
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
  foreach(const QModelIndex& index, selected_.indexes()) {
    Data::EntryPtr tmp = sourceModel()->data(index, EntryPtrRole).value<Data::EntryPtr>();
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
  const QStringList titles = Data::Document::self()->collection()->fieldTitles();
  for(int i = 0; i < titles.count(); ++i) {
    sortMenu->addAction(titles.at(i))->setData(i);
  }
  connect(sortMenu, SIGNAL(triggered(QAction*)), SLOT(slotSortMenuActivated(QAction*)));

  menu.exec(ev_->globalPos());
}

void EntryIconView::slotSortMenuActivated(QAction* action_) {
  model()->sort(action_->data().toInt());
}

#include "entryiconview.moc"
