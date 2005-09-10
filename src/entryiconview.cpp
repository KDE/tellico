/***************************************************************************
    copyright            : (C) 2002-2005 by Robby Stephenson
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
#include "imagefactory.h"
#include "controller.h"
#include "entry.h"
#include "field.h"
#include "tellico_utils.h"

#include <kdebug.h>
#include <kpopupmenu.h>
#include <kstringhandler.h>
#include <kapplication.h>
#include <kglobal.h> // needed for KMAX
#include <kiconloader.h>

namespace {
  static const uint MIN_ENTRY_ICON_SIZE = 32;
  static const uint MAX_ENTRY_ICON_SIZE = 128;
  static const uint ENTRY_ICON_SIZE_PAD = 4;
}

using Tellico::EntryIconView;
using Tellico::EntryIconViewItem;

EntryIconView::EntryIconView(QWidget* parent_, const char* name_/*=0*/)
    : KIconView(parent_, name_), m_coll(0), m_maxIconWidth(MAX_ENTRY_ICON_SIZE) {
  setAutoArrange(true);
  setSorting(true);
  setItemsMovable(false);
  setSelectionMode(Extended);
  setResizeMode(QIconView::Adjust);
  setMode(Select);
  setSpacing(4);
//  setWordWrapIconText(false);

  m_defaultPixmap = UserIcon(QString::fromLatin1("tellico"));

  m_itemMenu = new KPopupMenu(this);
  Controller::self()->plugEntryActions(m_itemMenu);

  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(doubleClicked(QIconViewItem*)), SLOT(slotDoubleClicked(QIconViewItem*)));
  connect(this, SIGNAL(contextMenuRequested(QIconViewItem*, const QPoint&)),
          SLOT(slotShowContextMenu(QIconViewItem*, const QPoint&)));
}

void EntryIconView::findImageField() {
  m_imageField.truncate(0);
  if(!m_coll) {
    return;
  }
  const Data::FieldVec& fields = m_coll->imageFields();
  if(!fields.isEmpty()) {
    m_imageField = fields[0]->name();
  }
//  kdDebug() << "EntryIconView::findImageField() - image field = " << m_imageField << endl;
}

const QString& EntryIconView::imageField() {
//  if(m_imageField.isNull()) {
//    findImageField();
//  }
  return m_imageField;
}

void EntryIconView::setMaxIconWidth(uint width_) {
  m_maxIconWidth = KMAX(MIN_ENTRY_ICON_SIZE, KMIN(MAX_ENTRY_ICON_SIZE, width_));
  refresh();
}

void EntryIconView::fillView() {
  setSorting(false);
  setGridX(m_maxIconWidth + 2*ENTRY_ICON_SIZE_PAD); // set default spacing initially

  GUI::CursorSaver cs(Qt::waitCursor);

  int maxWidth = MIN_ENTRY_ICON_SIZE;
  int maxHeight = 0;
  QIconViewItem* item;
  for(Data::EntryVecIt it = m_entries.begin(); it != m_entries.end(); ++it) {
    item = new EntryIconViewItem(this, it);
    maxWidth = KMAX(maxWidth, item->width());
    maxHeight = KMAX(maxWidth, item->pixmapRect().height());
  }
  setGridX(maxWidth + 2*ENTRY_ICON_SIZE_PAD); // the pad is so the text can be wider than the icon
  setGridY(maxHeight + fontMetrics().height());
  sort();
  setSorting(true);
}

void EntryIconView::refresh() {
  if(!m_coll) {
    return;
  }
  showEntries(m_entries);
}

void EntryIconView::clear() {
  KIconView::clear();
  m_coll = 0;
  m_entries.clear();
  m_imageField.truncate(0);
}

void EntryIconView::showEntries(const Data::EntryVec& entries_) {
  setUpdatesEnabled(false);
  KIconView::clear(); // don't call EntryIconView::clear() since that clears the entries_ ref
  if(entries_.isEmpty()) {
    return;
  }
  m_coll = entries_[0]->collection();
  m_entries = entries_;
  findImageField();
  fillView();
  setUpdatesEnabled(true);
}

void EntryIconView::addEntry(Data::Entry* entry_) {
  m_entries.append(entry_);
  new EntryIconViewItem(this, entry_);
}

void EntryIconView::removeEntry(Data::Entry* entry_) {
  m_entries.remove(entry_);
  for(QIconViewItem* item = firstItem(); item; item = item->nextItem()) {
    if(static_cast<EntryIconViewItem*>(item)->entry() == entry_) {
      m_selectedItems.removeRef(static_cast<EntryIconViewItem*>(item));
      delete item;
      arrangeItemsInGrid();
      break;
    }
  }
}

void EntryIconView::slotSelectionChanged() {
  Data::EntryVec entries;
  const QPtrList<EntryIconViewItem>& items = selectedItems();
  for(QPtrListIterator<EntryIconViewItem> it(items); it.current(); ++it) {
    entries.append(it.current()->entry());
  }
  Controller::self()->slotUpdateSelection(this, entries);
}

void EntryIconView::slotDoubleClicked(QIconViewItem* item_) {
  EntryIconViewItem* i = static_cast<EntryIconViewItem*>(item_);
  if(i) {
    Controller::self()->editEntry(*i->entry());
  }
}

void EntryIconView::updateSelected(EntryIconViewItem* item_, bool s_) const {
  if(s_) {
    m_selectedItems.append(item_);
  } else {
    m_selectedItems.removeRef(item_);
  }
}

void EntryIconView::slotShowContextMenu(QIconViewItem* item_, const QPoint& point_) {
  if(item_ && m_itemMenu->count() > 0) {
    m_itemMenu->popup(point_);
  }
}

EntryIconViewItem::EntryIconViewItem(EntryIconView* parent_, Data::Entry* entry_)
    : KIconViewItem(parent_, entry_->title()), m_entry(entry_) {
  const QString& imageField = parent_->imageField();
  if(imageField.isEmpty()) {
    setPixmap(parent_->defaultPixmap());
  } else {
    const Data::Image& img = ImageFactory::imageById(entry_->field(imageField));
    if(img.isNull()) {
      setPixmap(parent_->defaultPixmap());
    } else {
      setPixmap(img.convertToPixmap(parent_->maxIconWidth(), parent_->maxIconWidth()));
    }
  }
}

EntryIconViewItem::~EntryIconViewItem() {
  // be sure to remove from selected list when it's deleted
  EntryIconView* v = static_cast<EntryIconView*>(iconView());
  if(v) {
    v->updateSelected(this, false);
  }
}

void EntryIconViewItem::setSelected(bool s_) {
  setSelected(s_, false);
}

void EntryIconViewItem::setSelected(bool s_, bool cb_) {
  EntryIconView* v = static_cast<EntryIconView*>(iconView());
  v->updateSelected(this, s_);
  KIconViewItem::setSelected(s_, cb_);
}

#include "entryiconview.moc"
