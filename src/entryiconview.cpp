/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
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

#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kpopupmenu.h>
#include <kstringhandler.h>

namespace {
  static const uint MIN_ENTRY_ICON_SIZE = 32;
  static const uint MAX_ENTRY_ICON_SIZE = 128;
  static const uint ENTRY_ICON_SIZE_PAD = 4;
}

using Tellico::EntryIconView;
using Tellico::EntryIconViewItem;

EntryIconView::EntryIconView(QWidget* parent_, const char* name_/*=0*/)
    : KIconView(parent_, name_), m_coll(0), m_group(0), m_maxIconWidth(MAX_ENTRY_ICON_SIZE) {
  setAutoArrange(true);
  setSorting(true);
  setItemsMovable(false);
  setSelectionMode(Extended);
  setResizeMode(QIconView::Adjust);
  setMode(Select);
  setSpacing(4);
//  setWordWrapIconText(false);

  m_defaultPixmap = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("tellico"), KIcon::User);

  m_itemMenu = new KPopupMenu(this);
  Controller::self()->plugEntryActions(m_itemMenu);

  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(doubleClicked(QIconViewItem*)), SLOT(slotDoubleClicked(QIconViewItem*)));
  connect(this, SIGNAL(contextMenuRequested(QIconViewItem*, const QPoint&)),
          SLOT(slotShowContextMenu(QIconViewItem*, const QPoint&)));
}

void EntryIconView::findImageField() {
  m_imageField.truncate(0);
  if(!m_coll && (!m_group || m_group->isEmpty())) {
    return;
  }
  const Data::FieldList& list = m_coll ? m_coll->imageFields() : m_group->getFirst()->collection()->imageFields();
  if(!list.isEmpty()) {
    m_imageField = list.getFirst()->name();
  }
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

void EntryIconView::fillView(const Data::EntryList& list_) {
  setSorting(false);
  setGridX(m_maxIconWidth + 2*ENTRY_ICON_SIZE_PAD); // set default spacing initially
  int maxWidth = MIN_ENTRY_ICON_SIZE;
  int maxHeight = 0;
  QIconViewItem* item;
  for(Data::EntryListIterator it(list_); it.current(); ++it) {
    item = new EntryIconViewItem(this, it.current());
    maxWidth = KMAX(maxWidth, item->width());
    maxHeight = KMAX(maxWidth, item->pixmapRect().height());
  }
  setGridX(maxWidth + 2*ENTRY_ICON_SIZE_PAD); // the pad is so the text can be wider than the icon
  setGridY(maxHeight + fontMetrics().height());
  sort();
  setSorting(true);
}

void EntryIconView::refresh() {
  if(!m_coll && !m_group) {
    return;
  }
  // force a reset of the image field
  m_imageField.truncate(0);
  if(m_coll) {
    showCollection(m_coll);
  } else {
    showEntryGroup(m_group);
  }
}

void EntryIconView::clear() {
  QIconView::clear();
  m_coll = 0;
  m_group = 0;
  m_imageField.truncate(0);
}

void EntryIconView::showCollection(const Data::Collection* coll_) {
  QIconView::clear();
  m_coll = 0;
  m_group = 0;
  // don't call clear(), forces unneccesary resetting of the imageField name
  if(!coll_) {
    return;
  }
  m_coll = coll_;
  fillView(coll_->entryList());
}

void EntryIconView::showEntryGroup(const Data::EntryGroup* group_) {
  QIconView::clear();
  m_coll = 0;
  m_group = 0;
  // don't call clear(), forces unneccesary resetting of the imageField name
  if(!group_) {
    return;
  }
  m_group = group_;
  fillView(*group_);
}

void EntryIconView::addEntry(Data::Entry* entry_) {
  new EntryIconViewItem(this, entry_);
}

void EntryIconView::removeEntry(Data::Entry* entry_) {
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
  Data::EntryList list;
  const QPtrList<EntryIconViewItem>& items = selectedItems();
  for(QPtrListIterator<EntryIconViewItem> it(items); it.current(); ++it) {
    list.append(it.current()->entry());
  }
  Controller::self()->slotUpdateSelection(this, list);
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

// sole purpose is to nullify group pointer when it gets deleted
void EntryIconView::slotGroupModified(Tellico::Data::Collection*, const Data::EntryGroup* group_) {
  if(group_ == m_group && m_group->isEmpty()) {
    clear();
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
