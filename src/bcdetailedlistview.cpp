/***************************************************************************
                               bclistview.cpp
                             -------------------
    begin                : Tue Sep 4 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "bcdetailedlistview.h"
#include "bcunit.h"
#include "bcunititem.h"
#include "bccollection.h"
#include "bookcase.h"

#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>
//#include <klistview.h>

#include <qlist.h>
//#include <qlayout.h>
#include <qstringlist.h>
//#include <qregexp.h>
//#include <qwidgetstack.h>
#include <qvaluelist.h>

BCDetailedListView::BCDetailedListView(QWidget* parent_, const char* name_/*=0*/)
 : KListView(parent_, name_) {
//  kdDebug() << "BCDetailedListView()\n";
  setAllColumnsShowFocus(true);
  setShowSortIndicator(true);

  connect(this, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));
// if a list view item is clicked...something is modified, and then the user
// clicks on it again, no signal is sent because the selection didn't change...so the
// next connection must be made as well. The side effect is that two signals are sent when
// the user clicks on a different list view item
  connect(this, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(this, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

  QPixmap remove = KGlobal::iconLoader()->loadIcon("remove", KIcon::Small);
  m_menu.insertItem(remove, i18n("Delete"), this, SLOT(slotHandleDelete()));
}

BCDetailedListView::~BCDetailedListView() {
//  kdDebug() << "~BCDetailedListWidget()" << endl;
}

void BCDetailedListView::slotReset() {
	clear();
}

void BCDetailedListView::slotSetContents(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "BCDetailedListView::slotSetContents() - " << coll_->title() << endl;
// for now, reset, but if multiple collections are supported, this has to change
  slotReset();
  m_currColl = coll_;

  if(columns() == 0) {
    QStringList colNames = coll_->attributeTitles(false);
    QStringList::Iterator it = colNames.begin();
    for( ; it != colNames.end(); ++it) {
//      kdDebug() << QString("BCDetailedListView::slotSetContents() -- adding column (%1)").arg(QString(*it)) << endl;
      addColumn(static_cast<QString>(*it));
    }
  }

  Bookcase* app = static_cast<Bookcase*>(parent());
  QValueList<int> widthList = app->readColumnWidths(coll_->unitName());
  QValueList<int>::Iterator wIt;
  int i = 0;
  for(wIt = widthList.begin(); wIt != widthList.end() && i < columns(); ++wIt, ++i) {
    setColumnWidthMode(i, QListView::Manual);
    setColumnWidth(i, static_cast<int>(*wIt));
  }

  QListIterator<BCUnit> unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    slotAddItem(unitIt.current());
  }
}

void BCDetailedListView::slotAddItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCDetailedListView::slotAddItem() - " << unit_->attribute("title") << endl;

  BCUnitItem* item = new BCUnitItem(this, unit_);
  KIconLoader* loader = KGlobal::iconLoader();
  if(loader) {
    item->setPixmap(0, loader->loadIcon(unit_->collection()->iconName(), KIcon::User));
  }
  populateItem(item);
  if(isUpdatesEnabled()) {
    sort();
    ensureItemVisible(item);
    setSelected(item, true);
  }
}

void BCDetailedListView::slotModifyItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCDetailedListView::slotModifyItem() - " << unit_->attribute("title") << "\n";
  BCUnitItem* item = locateItem(unit_);
  populateItem(item);
  sort();
  ensureItemVisible(item);
}

void BCDetailedListView::slotRemoveItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

  kdDebug() << "BCDetailedListView::slotRemoveItem() - " << unit_->attribute("title") << "\n";

  delete locateItem(unit_);
}

void BCDetailedListView::slotHandleDelete() {
  BCUnitItem* item = static_cast<BCUnitItem*>(currentItem());
  if(!item) {
    return;
  }

  emit signalDeleteUnit(item->unit());
}

void BCDetailedListView::populateItem(BCUnitItem* item_) {
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
        kdDebug() << "BCDetailedListView::populateItem() - Column header does not match attribute name." << endl;
      }
      item_->setText(colNum, text);
      // only increment counter if the attribute is a visible one
      colNum++;
    }
  } // end attribute loop
}

void BCDetailedListView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(item_ && m_menu.count() > 0) {
    setSelected(item_, true);
    m_menu.popup(point_);
  }
}

void BCDetailedListView::slotSelected(QListViewItem* item_) {
  if(!item_ && !selectedItem()) {
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

void BCDetailedListView::slotSetSelected(BCUnit* unit_) {
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

BCUnitItem* BCDetailedListView::locateItem(BCUnit* unit_) {
  QListViewItemIterator it(this);
  for( ; it.current(); ++it) {
    BCUnitItem* item = static_cast<BCUnitItem*>(it.current());
    if(item->unit() == unit_) {
      return item;
    }
  }

  return 0;
}

void BCDetailedListView::slotClearSelection() {
  selectAll(false);
}
