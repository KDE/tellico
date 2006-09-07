/***************************************************************************
    copyright            : (C) 2002-2006 by Robby Stephenson
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
#include "tellico_utils.h"
#include "tellico_debug.h"

#include <kpopupmenu.h>
#include <kstringhandler.h>
#include <kiconloader.h>
#include <kwordwrap.h>
#include <kimageeffect.h>

#include <qpainter.h>

namespace {
  static const int MIN_ENTRY_ICON_SIZE = 32;
  static const int MAX_ENTRY_ICON_SIZE = 128;
  static const int ENTRY_ICON_SIZE_PAD = 6;
  static const int ENTRY_ICON_SHADOW_RIGHT = 1;
  static const int ENTRY_ICON_SHADOW_BOTTOM = 1;
}

using Tellico::EntryIconView;
using Tellico::EntryIconViewItem;

EntryIconView::EntryIconView(QWidget* parent_, const char* name_/*=0*/)
    : KIconView(parent_, name_), m_coll(0), m_maxAllowedIconWidth(MAX_ENTRY_ICON_SIZE),
      m_maxIconWidth(MIN_ENTRY_ICON_SIZE), m_maxIconHeight(MIN_ENTRY_ICON_SIZE) {
  setAutoArrange(true);
  setSorting(true);
  setItemsMovable(false);
  setSelectionMode(QIconView::Extended);
  setResizeMode(QIconView::Adjust);
  setMode(KIconView::Select);
  setSpacing(4);
//  setWordWrapIconText(false);

  m_defaultPixmaps.setAutoDelete(true);

  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(doubleClicked(QIconViewItem*)), SLOT(slotDoubleClicked(QIconViewItem*)));
  connect(this, SIGNAL(contextMenuRequested(QIconViewItem*, const QPoint&)),
          SLOT(slotShowContextMenu(QIconViewItem*, const QPoint&)));
}

EntryIconViewItem* EntryIconView::firstItem() const {
  return static_cast<EntryIconViewItem*>(KIconView::firstItem());
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
//  myDebug() << "EntryIconView::findImageField() - image field = " << m_imageField << endl;
}

const QString& EntryIconView::imageField() {
  return m_imageField;
}

const QPixmap& EntryIconView::defaultPixmap() {
  QPixmap* pix = m_defaultPixmaps[m_coll->type()];
  if(pix) {
    return *pix;
  }
  KIconLoader* loader = KGlobal::instance()->iconLoader();
  QPixmap tmp = loader->loadIcon(QString::fromLatin1("nocover_") + CollectionFactory::typeName(m_coll->type()),
                                 KIcon::User, 0, KIcon::DefaultState, 0, true /*canReturnNull */);
  if(tmp.isNull()) {
    myLog() << "EntryIconView::defaultPixmap() - null nocover image, loading tellico.png" << endl;
    tmp = UserIcon(QString::fromLatin1("tellico"));
  }
  if(QMAX(tmp.width(), tmp.height()) > static_cast<int>(MIN_ENTRY_ICON_SIZE)) {
    tmp.convertFromImage(tmp.convertToImage().smoothScale(m_maxIconWidth, m_maxIconHeight, QImage::ScaleMin));
  }
  pix = new QPixmap(tmp);
  m_defaultPixmaps.insert(m_coll->type(), pix);
  return *pix;
}

void EntryIconView::setMaxAllowedIconWidth(int width_) {
  m_maxAllowedIconWidth = QMAX(MIN_ENTRY_ICON_SIZE, QMIN(MAX_ENTRY_ICON_SIZE, width_));
  setMaxItemWidth(m_maxAllowedIconWidth + 2*ENTRY_ICON_SIZE_PAD);
  m_defaultPixmaps.clear();
  refresh();
}

void EntryIconView::fillView() {
  setSorting(false);
  setGridX(m_maxAllowedIconWidth + 2*ENTRY_ICON_SIZE_PAD); // set default spacing initially

  GUI::CursorSaver cs(Qt::waitCursor);

  bool allDefaultImages = true;
  m_maxIconWidth = QMAX(MIN_ENTRY_ICON_SIZE, m_maxIconWidth);
  m_maxIconHeight = QMAX(MIN_ENTRY_ICON_SIZE, m_maxIconHeight);
  EntryIconViewItem* item;
  for(Data::EntryVecIt it = m_entries.begin(); it != m_entries.end(); ++it) {
    item = new EntryIconViewItem(this, it);
    m_maxIconWidth = QMAX(m_maxIconWidth, item->width());
    m_maxIconHeight = QMAX(m_maxIconHeight, item->pixmapRect().height());
    if(item->usesImage()) {
      allDefaultImages = false;
    }
  }
  if(allDefaultImages) {
    m_maxIconWidth = m_maxAllowedIconWidth;
    m_maxIconHeight = m_maxAllowedIconWidth;
  }
  // if both width and height are min, then that means there are no images
  m_defaultPixmaps.clear();
  // now reset size of all default pixmaps
  for(item = firstItem(); item; item = item->nextItem()) {
    if(!item->usesImage()) {
      item->updatePixmap();
    }
  }
  setGridX(m_maxIconWidth + 2*ENTRY_ICON_SIZE_PAD); // the pad is so the text can be wider than the icon
  setGridY(m_maxIconHeight + fontMetrics().height());
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

void EntryIconView::addEntries(Data::EntryVec entries_) {
  if(entries_.isEmpty()) {
    return;
  }
  if(!m_coll) {
    m_coll = entries_[0]->collection();
  }
  // since the view probably doesn't show all the current entries
  // only add the new ones if the count is the total
  if(m_entries.count() + entries_.count() < m_coll->entryCount()) {
    return;
  }
  int w = MIN_ENTRY_ICON_SIZE;
  int h = MIN_ENTRY_ICON_SIZE;
  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    m_entries.append(entry);
    EntryIconViewItem* item = new EntryIconViewItem(this, entry);
    w = QMAX(w, item->width());
    h = QMAX(h, item->pixmapRect().height());
  }
  if(w > m_maxIconWidth || h > m_maxIconHeight) {
    refresh();
  }
}

void EntryIconView::modifyEntries(Data::EntryVec entries_) {
  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    EntryIconViewItem* item = 0;
    for(EntryIconViewItem* it = firstItem(); it; it = it->nextItem()) {
      if(it->entry() == entry) {
        item = it;
        break;
      }
    }
    if(!item) {
      continue;
    }
    item->update();
  }
}

void EntryIconView::removeEntries(Data::EntryVec entries_) {
  for(Data::EntryVecIt entry = entries_.begin(); entry != entries_.end(); ++entry) {
    m_entries.remove(entry);
  }
  bool found = false;
  EntryIconViewItem* item = firstItem();
  while(item) {
    if(entries_.contains(item->entry())) {
      EntryIconViewItem* prev = item;
      item = item->nextItem();
      delete prev;
      found = true;
    } else {
      item = item->nextItem();
    }
  }
  if(found) {
    arrangeItemsInGrid();
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
    Controller::self()->editEntry(i->entry());
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
  if(!item_) {
    return;
  }
  KPopupMenu menu(this);
  Controller::self()->plugEntryActions(&menu);
  menu.exec(point_);
}

EntryIconViewItem::EntryIconViewItem(EntryIconView* parent_, Data::EntryPtr entry_)
    : KIconViewItem(parent_, entry_->title()), m_entry(entry_), m_usesImage(false) {
  setDragEnabled(false);
  const QString& imageField = parent_->imageField();
  if(!imageField.isEmpty()) {
    QPixmap p = ImageFactory::pixmap(m_entry->field(imageField),
                                     parent_->maxAllowedIconWidth(),
                                     parent_->maxAllowedIconWidth());
    if(!p.isNull()) {
      setPixmap(p);
      m_usesImage = true;
    }
  }
}

EntryIconViewItem::~EntryIconViewItem() {
  // be sure to remove from selected list when it's deleted
  EntryIconView* view = iconView();
  if(view) {
    view->updateSelected(this, false);
  }
}

void EntryIconViewItem::setSelected(bool s_) {
  setSelected(s_, false);
}

void EntryIconViewItem::setSelected(bool s_, bool cb_) {
  EntryIconView* view = iconView();
  if(!view) {
    return;
  }
  if(s_ != isSelected()) {
    view->updateSelected(this, s_);
    KIconViewItem::setSelected(s_, cb_);
  }
}

void EntryIconViewItem::updatePixmap() {
  EntryIconView* view = iconView();
  const QString& imageField = view->imageField();
  m_usesImage = false;
  if(imageField.isEmpty()) {
    setPixmap(view->defaultPixmap());
  } else {
    QPixmap p = ImageFactory::pixmap(m_entry->field(imageField),
                                     view->maxAllowedIconWidth(),
                                     view->maxAllowedIconWidth());
    if(p.isNull()) {
      setPixmap(view->defaultPixmap());
    } else {
      setPixmap(p);
      m_usesImage = true;
      calcRect();
    }
  }
}

void EntryIconViewItem::update() {
  setText(m_entry->title());
  updatePixmap();
  iconView()->arrangeItemsInGrid();
}

void EntryIconViewItem::calcRect(const QString& text_) {
  KIconViewItem::calcRect(text_);
  QRect r = pixmapRect();
  r.rRight()  += ENTRY_ICON_SHADOW_RIGHT;
  r.rBottom() += ENTRY_ICON_SHADOW_BOTTOM;
  setPixmapRect(r);
}

void EntryIconViewItem::paintItem(QPainter* p_, const QColorGroup &cg_) {
  p_->save();
  paintPixmap(p_, cg_);
  paintText(p_, cg_);
  p_->restore();
}

void EntryIconViewItem::paintFocus(QPainter*, const QColorGroup&) {
  // don't draw anything
}

void EntryIconViewItem::paintPixmap(QPainter* p_, const QColorGroup& cg_) {
  // only draw the shadow if there's an image
  // oh, and don't draw it if it's a file catalog, it doesn't look right
  if(m_usesImage && !isSelected() && m_entry->collection()->type() != Data::Collection::File) {
    // pixmapRect() already includes shadow size, so shift the rect by that amount
    QRect r = pixmapRect(false);
    r.setLeft(r.left() + ENTRY_ICON_SHADOW_RIGHT);
    r.setTop(r.top() + ENTRY_ICON_SHADOW_BOTTOM);
    QColor c = Tellico::blendColors(iconView()->backgroundColor(), Qt::black, 10);
    p_->fillRect(r, c);
  }
  KIconViewItem::paintPixmap(p_, cg_);
}

void EntryIconViewItem::paintText(QPainter* p_, const QColorGroup &cg_) {
  QRect tr = textRect(false);
  int textX = tr.x() + 2;
  int textY = tr.y();

  if(isSelected()) {
    p_->setBrush(QBrush(cg_.highlight()));
    p_->setPen(QPen(cg_.highlight()));
    p_->drawRoundRect(tr, 1000/tr.width(), 1000/tr.height());
    p_->setPen(QPen(cg_.highlightedText()));
  } else {
    if(iconView()->itemTextBackground() != NoBrush) {
      p_->fillRect(tr, iconView()->itemTextBackground());
    }
    p_->setPen(cg_.text());
  }

  int align = iconView()->itemTextPos() == QIconView::Bottom ? AlignHCenter : AlignAuto;
  wordWrap()->drawText(p_, textX, textY, align | KWordWrap::Truncate);
}

#include "entryiconview.moc"
