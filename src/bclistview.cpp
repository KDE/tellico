/***************************************************************************
                          bclistview.cpp  -  description
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "bclistview.h"
#include "bcunit.h"
#include "bcunititem.h"
#include "bccollection.h"
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <klistview.h>
#include <kpopupmenu.h>
#include <qlist.h>
//#include <qlayout.h>
#include <qstringlist.h>
//#include <qregexp.h>
//#include <qwidgetstack.h>

BCListView::BCListView(QWidget* parent_, const char* name_/*=0*/)
 : KListView(parent_, name_) {
  // kdDebug() << "BCListView()\n";
  setAllColumnsShowFocus(true);
  setShowSortIndicator(true);

  m_menu = new KPopupMenu(this);
  KIconLoader* loader = KGlobal::iconLoader();
  QPixmap remove;
  if(loader) {
    remove = loader->loadIcon("remove", KIcon::Small);
  }
  m_menu->insertItem(remove, i18n("Delete"), this, SLOT(slotHandleDelete()));

  connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

//  connect(this, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));
}

BCListView::~BCListView() {
}

void BCListView::slotReset() {
  clear();
}

void BCListView::slotAddPage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
  // for now, only allowing one collection to exist in listview
  // so if there are columns already, just return
  if(columns() > 0) {
    return;
  }

  QStringList colNames = coll_->attributeTitles(false);
  QStringList::Iterator it = colNames.begin();
  for( ; it != colNames.end(); ++it) {
    kdDebug() << QString("BCListView::addPage() -- adding column (%1)").arg(QString(*it)) << endl;
    addColumn(static_cast<QString>(*it));
  }

  QListIterator<BCUnit> unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    slotAddItem(unitIt.current());
  }
}

void BCListView::slotRemovePage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
}

void BCListView::slotAddItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCListView::slotAddItem() - " << unit_->attribute("title") << "\n";
  BCUnitItem* item = new BCUnitItem(this, unit_);
  KIconLoader* loader = KGlobal::iconLoader();
  if(loader) {
    item->setPixmap(0, loader->loadIcon(unit_->collection()->iconName(), KIcon::User));
  }
  populateItem(item);
  ensureItemVisible(item);
}

void BCListView::slotModifyItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCListView::slotModifyItem() - " << unit_->attribute("title") << "\n";
  BCUnitItem* item = locateItem(unit_);
  populateItem(item);
  ensureItemVisible(item);
}

void BCListView::slotRemoveItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCListView::slotRemoveItem() - " << unit_->attribute("title") << "\n";
  BCUnitItem* item = locateItem(unit_);
  delete item;
}

void BCListView::slotHandleDelete() {
  BCUnitItem* item = static_cast<BCUnitItem*>(currentItem());
  if(!item) {
    return;
  }

  BCUnit* unit = item->unit();
  emit signalDoUnitDelete(unit);
}

void BCListView::populateItem(BCUnitItem* item_) {
  BCUnit* unit = item_->unit();
  if(!unit) {
    return;
  }

  unsigned colNum = 0;
  QListIterator<BCAttribute> it(unit->collection()->attributeList());
  for( ; it.current(); ++it) {
    int flags = it.current()->flags();
    if(!(flags & BCAttribute::DontShow)) {
      QString text = unit->attribute(it.current()->name());
      if(flags & BCAttribute::FormatTitle) {
        text = BCAttribute::formatTitle(text);
      } else if(flags & BCAttribute::FormatName) {
        text = BCAttribute::formatName(text, (flags & BCAttribute::AllowMultiple));
      } else if(flags & BCAttribute::FormatDate) {
        text = BCAttribute::formatDate(text);
      }
      if(columnText(colNum) != it.current()->title()) {
        // TODO: if the visible columns have changed, need to account for that
        kdDebug() << "BCListView::populateItem() - Column header does not match attribute name." << endl;
      }
      item_->setText(colNum, text);
      colNum++;
    }
  } // end attribute loop
}

void BCListView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(item_ && m_menu->count() > 0) {
    setSelected(item_, true);
    m_menu->popup(point_);
  }
}

void BCListView::slotSelected(QListViewItem* item_) {
  if(!selectedItem() && !item_) {
    emit signalClear();
    return;
  }

  // there may still be a null pointer, so set it to the selected item
  // TODO: somewhat inefficient since the selection probably did not change
  if(!item_) {
    item_ = selectedItem();
  }

  // all items in the listview are unitItems
  BCUnitItem* item = static_cast<BCUnitItem*>(item_);
  if(item->unit()) {
    emit signalUnitSelected(item->unit());
  }
}

void BCListView::slotSetSelected(BCUnit* unit_) {
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    setSelected(currentItem(), false);
    return;
  }

  BCUnitItem* item = locateItem(unit_);
  // this ends up calling BCUnitEditWidget::slotSetContents() twice
  // since the selectedItem() signal gets sent by both this object and the other listview
  setSelected(item, true);
  ensureItemVisible(item);
}

BCUnitItem* BCListView::locateItem(BCUnit* unit_) {
  QListViewItemIterator it(this);
  for( ; it.current(); ++it) {
    BCUnitItem* item = static_cast<BCUnitItem*>(it.current());
    if(item->unit() == unit_) {
      return item;
    }
  }

  return NULL;
}
