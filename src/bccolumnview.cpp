/***************************************************************************
                               bclistview.cpp
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

#include "bccolumnview.h"
#include "bcunit.h"
#include "bcunititem.h"
#include "bccollection.h"
#include "bookcase.h"

#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <klistview.h>

#include <qlist.h>
#include <qlayout.h>
#include <qstringlist.h>
//#include <qregexp.h>
#include <qwidgetstack.h>
#include <qvaluelist.h>

BCColumnView::BCColumnView(QWidget* parent_, const char* name_/*=0*/)
 : QWidget(parent_, name_) {
  // kdDebug() << "BCColumnView()\n";
  //setAllColumnsShowFocus(true);
  //setShowSortIndicator(true);
  QVBoxLayout* l = new QVBoxLayout(this);
  m_stack = new QWidgetStack(this);
  l->addWidget(m_stack);
  m_stack->show();

  QPixmap remove = KGlobal::iconLoader()->loadIcon("remove", KIcon::Small);
  m_menu.insertItem(remove, i18n("Delete"), this, SLOT(slotHandleDelete()));
}

BCColumnView::~BCColumnView() {
}

void BCColumnView::slotReset() {
  delete m_stack;
  m_stack = new QWidgetStack(this);
  static_cast<QVBoxLayout*>(layout())->addWidget(m_stack);
  m_stack->show();
}

void BCColumnView::slotAddPage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }

  kdDebug() << "BCColumnView::slotAddPage() - " << coll_->title() << endl;
  m_currColl = coll_;

  KListView* w = new KListView(m_stack);
  w->setAllColumnsShowFocus(true);
  w->setShowSortIndicator(true);

  connect(w, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));
// if a list view item is clicked...something is modified, and then the user
// clicks on it again, no signal is sent because the selection didn't change...so the
// next connection must be made as well. The side effect is that two signals are sent when
// the user clicks on a different list view item
  connect(w, SIGNAL(clicked(QListViewItem*)), SLOT(slotSelected(QListViewItem*)));

  connect(w, SIGNAL(rightButtonPressed(QListViewItem*, const QPoint&, int)),
          SLOT(slotRMB(QListViewItem*, const QPoint&, int)));

  QStringList colNames = coll_->attributeTitles(false);
  QStringList::Iterator it = colNames.begin();
  for( ; it != colNames.end(); ++it) {
    kdDebug() << QString("BCColumnView::addPage() -- adding column (%1)").arg(QString(*it)) << endl;
    w->addColumn(static_cast<QString>(*it));
  }

  Bookcase* app = static_cast<Bookcase*>(parent());
  QValueList<int> widthList = app->readColumnWidths(coll_->unitName());
  QValueList<int>::Iterator wIt;
  int i = 0;
  for(wIt = widthList.begin(); wIt != widthList.end() && i < w->columns(); ++wIt, ++i) {
    w->setColumnWidthMode(i, QListView::Manual);
    w->setColumnWidth(i, static_cast<int>(*wIt));
  }

  QListIterator<BCUnit> unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    slotAddItem(unitIt.current());
  }

  m_stack->addWidget(w, coll_->id());
  // not sure if addWidget() is guaranteed to put it on top or not
  m_stack->raiseWidget(w);
}

void BCColumnView::slotRemovePage(BCCollection* coll_) {
  if(!coll_) {
    return;
  }
  kdDebug() << "BCColumnView::slotRemovePage() - " << coll_->title() << endl;

  QWidget* w = m_stack->widget(coll_->id());
  m_stack->removeWidget(w);
  delete w;
}

void BCColumnView::slotAddItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCColumnView::slotAddItem() - " << unit_->attribute("title") << "\n";
  KListView* view = static_cast<KListView*>(m_stack->widget(unit_->collection()->id()));
  if(!view) {
    kdWarning() << "BCColumnView::slotAddItem - adding unit for which no collection widget exists." << endl;
    slotAddPage(unit_->collection());
    // now the visible widget is the collection listview
    view = static_cast<KListView*>(m_stack->visibleWidget());
    //return;
  }

  BCUnitItem* item = new BCUnitItem(view, unit_);
  KIconLoader* loader = KGlobal::iconLoader();
  if(loader) {
    item->setPixmap(0, loader->loadIcon(unit_->collection()->iconName(), KIcon::User));
  }
  populateItem(item);
  if(isUpdatesEnabled()) {
    view->ensureItemVisible(item);
    view->setSelected(item, true);
  }
}

void BCColumnView::slotModifyItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

//  kdDebug() << "BCColumnView::slotModifyItem() - " << unit_->attribute("title") << "\n";
  KListView* view = static_cast<KListView*>(m_stack->widget(unit_->collection()->id()));
  if(!view) {
    kdWarning() << "BCColumnView::slotModifyItem - modifying unit for which no collection widget exists." << endl;
    return;
  }

  BCUnitItem* item = locateItem(unit_, view);
  populateItem(item);
  view->ensureItemVisible(item);
}

void BCColumnView::slotRemoveItem(BCUnit* unit_) {
  if(!unit_) {
    return;
  }

  kdDebug() << "BCColumnView::slotRemoveItem() - " << unit_->attribute("title") << "\n";

  KListView* view = static_cast<KListView*>(m_stack->widget(unit_->collection()->id()));
  if(!view) {
    kdDebug() << "BCColumnView::slotRemoveItem - removing unit for which no collection widget exists." << endl;
    return;
  }

  delete locateItem(unit_, view);
}

void BCColumnView::slotHandleDelete() {
  KListView* view = static_cast<KListView*>(m_stack->visibleWidget());
  if(!view) {
    return;
  }

  BCUnitItem* item = static_cast<BCUnitItem*>(view->currentItem());
  if(!item) {
    return;
  }

  emit signalDoUnitDelete(item->unit());
}

void BCColumnView::populateItem(BCUnitItem* item_) {
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

      if(item_->listView()->columnText(colNum) != it.current()->title()) {
        // TODO: if the visible columns have changed, need to account for that
        kdDebug() << "BCColumnView::populateItem() - Column header does not match attribute name." << endl;
      }
      item_->setText(colNum, text);
      // only increment counter if the attribute is a visible one
      colNum++;
    }
  } // end attribute loop
}

void BCColumnView::slotRMB(QListViewItem* item_, const QPoint& point_, int) {
  if(item_ && m_menu.count() > 0) {
    item_->listView()->setSelected(item_, true);
    m_menu.popup(point_);
  }
}

void BCColumnView::slotSelected(QListViewItem* item_) {
  if(!item_ && !visibleListView()->selectedItem()) {
    emit signalClear();
    return;
  }

  // there may still be a null pointer, so set it to the selected item
  // TODO: somewhat inefficient since the selection probably did not change
  if(!item_) {
    item_ = visibleListView()->selectedItem();
  }

  // all items in the listview are unitItems
  BCUnitItem* item = static_cast<BCUnitItem*>(item_);
  if(item->unit()) {
    emit signalUnitSelected(item->unit());
  }
}

void BCColumnView::slotSetSelected(BCUnit* unit_) {
  // if unit_ is null pointer, set no selected
  if(!unit_) {
    if(m_stack->visibleWidget()) {
      KListView* view = static_cast<KListView*>(m_stack->visibleWidget());
      view->setSelected(view->currentItem(), false);
    }
    return;
  }

  KListView* view = static_cast<KListView*>(m_stack->widget(unit_->collection()->id()));
  if(!view) {
    return;
  }
  m_stack->raiseWidget(view);

  BCUnitItem* item = locateItem(unit_, view);
  // this ends up calling BCUnitEditWidget::slotSetContents() twice
  // since the selectedItem() signal gets sent by both this object and the other listview
  view->setSelected(item, true);
  view->ensureItemVisible(item);
}

BCUnitItem* BCColumnView::locateItem(BCUnit* unit_, KListView* view_/*=0*/) {
  if(!view_) {
    view_ = static_cast<KListView*>(m_stack->widget(unit_->collection()->id()));
    if(!view_) {
      return NULL;
    }
  }

  QListViewItemIterator it(view_);
  for( ; it.current(); ++it) {
    BCUnitItem* item = static_cast<BCUnitItem*>(it.current());
    if(item->unit() == unit_) {
      return item;
    }
  }

  return NULL;
}

KListView* BCColumnView::visibleListView() {
  return static_cast<KListView*>(m_stack->visibleWidget());
}

KListView* BCColumnView::listView(int id_) {
  return static_cast<KListView*>(m_stack->widget(id_));
}

void BCColumnView::slotClearSelection() {
  KListView* view = visibleListView();
  if(view) {
    view->selectAll(false);
  }
}

void BCColumnView::slotShowUnit(BCUnit* unit_) {
  m_stack->raiseWidget(unit_->collection()->id());
}


