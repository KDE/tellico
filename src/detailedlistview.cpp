/***************************************************************************
    copyright            : (C) 2001-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "detailedlistview.h"
#include "collection.h"
#include "imagefactory.h"
#include "controller.h"
#include "field.h"
#include "entry.h"
#include "tellico_debug.h"
#include "tellico_kernel.h"
#include "core/tellico_config.h"
#include "models/entrymodel.h"
#include "models/entrysortmodel.h"

#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kaction.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kcolorutils.h>

#include <QPixmap>
#include <QMouseEvent>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QStyledItemDelegate>

namespace {
  static const int MIN_COL_WIDTH = 50;
}

namespace Tellico {

class DetailedEntryItemDelegate : public QStyledItemDelegate {
public:
  DetailedEntryItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

protected:
  void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const {
    QStyledItemDelegate::initStyleOption(option, index);

    QStyleOptionViewItemV4* opt = ::qstyleoption_cast<QStyleOptionViewItemV4*>(option);
    const int state = index.data(SaveStateRole).toInt();
    if(state == NewState || state == ModifiedState) {
      opt->font.setBold(true);
      opt->font.setItalic(true);
    }
  }
};

}

using Tellico::DetailedListView;

DetailedListView::DetailedListView(QWidget* parent_) : GUI::TreeView(parent_) {
  setHeaderHidden(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setAlternatingRowColors(true);
  setRootIsDecorated(false);

//  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(slotDoubleClicked(const QModelIndex&)));

  // header menu
  header()->installEventFilter(this);
//  connect(header(), SIGNAL(sizeChange(int, int, int)),
//          this, SLOT(slotCacheColumnWidth(int, int, int)));

  m_headerMenu = new KMenu(this);
  connect(m_headerMenu, SIGNAL(triggered(QAction*)),
          SLOT(slotHeaderMenuActivated(QAction*)));

  EntryModel* entryModel = new EntryModel(this);
  EntrySortModel* sortModel = new EntrySortModel(this);
  sortModel->setSortRole(EntryPtrRole);
  sortModel->setSourceModel(entryModel);
  setModel(sortModel);
  setItemDelegate(new DetailedEntryItemDelegate(this));
}

DetailedListView::~DetailedListView() {
}

Tellico::EntryModel* DetailedListView::sourceModel() const {
  return static_cast<EntryModel*>(sortModel()->sourceModel());
}

void DetailedListView::addCollection(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    return;
  }

  KConfigGroup config(KGlobal::config(), QString::fromLatin1("Options - %1").arg(coll_->typeName()));

  QString configN;
  if(coll_->type() == Data::Collection::Base) {
    KUrl url = Kernel::self()->URL();
    for(int i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KUrl u = config.readEntry(QString::fromLatin1("URL_%1").arg(i), KUrl());
      if(u == url) {
        configN = QString::fromLatin1("_%1").arg(i);
        break;
      }
    }
  }

  QStringList colNames = config.readEntry(QLatin1String("ColumnNames") + configN, QStringList());
  QList<int> colWidths = config.readEntry(QLatin1String("ColumnWidths") + configN, QList<int>());
  QList<int> colOrder = config.readEntry(QLatin1String("ColumnOrder") + configN, QList<int>());

  sourceModel()->setFields(coll_->fields());

  setUpdatesEnabled(false);

  QByteArray state = QByteArray::fromBase64(config.readEntry(QLatin1String("ColumnState") + configN).toAscii());
  if(!state.isEmpty()) {
    // the easy case first. I fwe have saved state, just restore it
    header()->restoreState(state);
  } else {
    // what's a good way to determine which columns to show by default?
    // definitely the title, maybe that's enough
    if(colNames.isEmpty()) {
      colNames << QLatin1String("title");
      colWidths = QList<int>() << 1;
    }
    // I'm being lazy and not trying to figrue out the order and column width
    // just restore the columns that the user had before
    for(int ncol = 0; ncol < header()->count(); ++ncol) {
      int idx = colNames.indexOf(coll_->fields().at(ncol)->name());
      // column width of 0 meant hidden for old versions of Tellico
      if(idx == -1 || idx >= colWidths.size() || colWidths.at(idx) == 0) {
        setColumnHidden(ncol, true);
      }
    }
  }

  //  int sortCol = config.readEntry("SortColumn" + configN, 0);
  //  bool sortAsc = config.readEntry("SortAscending" + configN, true);
  //  model()->sort(sortCol, sortAsc ? Qt::AscendingOrder : Qt::DescendingOrder);
  sortModel()->setSecondarySortColumn(config.readEntry(QLatin1String("PrevSortColumn") + configN, -1));
  sortModel()->setTertiarySortColumn(config.readEntry(QLatin1String("Prev2SortColumn") + configN, -1));
  //  updateComparison(header()->sortIndicatorSection());

  m_loadingCollection = true;
  addEntries(coll_->entries());
  m_loadingCollection = false;

  // must be after adding fields and entries
  updateHeaderMenu();

  setUpdatesEnabled(true);
  sortModel()->invalidate();
}

void DetailedListView::slotReset() {
  //clear() does not remove columns
  sourceModel()->clear();
  sortModel()->clear();
}

void DetailedListView::addEntries(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return;
  }
  sourceModel()->addEntries(entries_);
  if (!m_loadingCollection) {
    setState(entries_, NewState);
    setEntriesSelected(entries_);
  }
}

void DetailedListView::modifyEntries(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return;
  }
  sourceModel()->modifyEntries(entries_);
  setState(entries_, ModifiedState);
  setEntriesSelected(entries_);
}

void DetailedListView::removeEntries(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return;
  }
  sourceModel()->removeEntries(entries_);
}

void DetailedListView::setState(Tellico::Data::EntryList entries_, int state) {
  for(QModelIndex index = model()->index(0, 0); index.isValid(); index = index.sibling(index.row()+1, 0)) {
    foreach (Data::EntryPtr entry, entries_) {
      Data::EntryPtr tmpEntry = model()->data(index, EntryPtrRole).value<Data::EntryPtr>();
      if(tmpEntry == entry) {
        model()->setData(index, state, SaveStateRole);
      }
    }
  }
}

void DetailedListView::removeCollection(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    myWarning() << "null coll pointer!";
    return;
  }

  sourceModel()->clear();
  updateHeaderMenu();
}

void DetailedListView::contextMenuEvent(QContextMenuEvent* event_) {
  QModelIndex index = indexAt(event_->pos());
  if(!index.isValid()) {
    return;
  }

  KMenu menu(this);
  Controller::self()->plugEntryActions(&menu);
  menu.exec(event_->globalPos());
}

// don't shadow QListView::setSelected
void DetailedListView::setEntriesSelected(Data::EntryList entries) {
//  DEBUG_LINE;
  if(entries.count() == 0) {
    // don't move this one outside the block since it calls setCurrentItem(0)
    clearSelection();
    return;
  }

  clearSelection();
  QModelIndex index = model()->index(0, 0);
  for(; index.isValid(); index = index.sibling(index.row()+1, 0)) {
    foreach (Data::EntryPtr entry_, entries) {
      Data::EntryPtr tmpEntry = model()->data(index, EntryPtrRole).value<Data::EntryPtr>();
      if(tmpEntry == entry_) {
        blockSignals(true);
        selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        //setCurrentIndex(index);
        blockSignals(false);
        scrollTo(index);
        break;
      }
    }
  }
}

bool DetailedListView::eventFilter(QObject* obj_, QEvent* event_) {
  if(event_->type() == QEvent::ContextMenu
     && obj_ == header()) {
    m_headerMenu->exec(static_cast<QContextMenuEvent*>(event_)->globalPos());
    return true;
  }
  return GUI::TreeView::eventFilter(obj_, event_);
}

void DetailedListView::selectionChanged(const QItemSelection& selected_, const QItemSelection& deselected_) {
  GUI::TreeView::selectionChanged(selected_, deselected_);
  Data::EntryList entries;
  foreach(const QModelIndex& index, selectionModel()->selectedRows()) {
    QModelIndex realIndex = sortModel()->mapToSource(index);
    Data::EntryPtr tmp = sourceModel()->data(realIndex, EntryPtrRole).value<Data::EntryPtr>();
    if(tmp) {
      entries += tmp;
    }
  }
  Controller::self()->slotUpdateSelection(this, entries);
}

void DetailedListView::slotDoubleClicked(const QModelIndex& index_) {
  Data::EntryPtr entry = sourceModel()->data(index_, EntryPtrRole).value<Data::EntryPtr>();
  if(entry) {
    Controller::self()->editEntry(entry);
  }
}

void DetailedListView::slotHeaderMenuActivated(QAction* action_) {
  int col = action_->data().toInt();
  setColumnHidden(col, !action_->isChecked());
}

void DetailedListView::slotRefresh() {
}

void DetailedListView::setFilter(Tellico::FilterPtr filter_) {
  sortModel()->setFilter(filter_);
}

Tellico::FilterPtr DetailedListView::filter() const {
  return sortModel()->filter();
}

void DetailedListView::addField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr field) {
  sourceModel()->addFields(Data::FieldList() << field);
  updateHeaderMenu();
}

void DetailedListView::modifyField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr oldField_, Tellico::Data::FieldPtr newField_) {
  Q_UNUSED(oldField_)
  sourceModel()->modifyFields(Data::FieldList() << newField_);
  updateHeaderMenu();
}

void DetailedListView::removeField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr field_) {
  sourceModel()->removeFields(Data::FieldList() << field_);
  updateHeaderMenu();
}

void DetailedListView::reorderFields(const Tellico::Data::FieldList& fields_) {
  sourceModel()->setFields(fields_);
  updateHeaderMenu();
}

void DetailedListView::saveConfig(Tellico::Data::CollPtr coll_, int configIndex_) {
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("Options - %1").arg(coll_->typeName()));

  // all of this is to have custom settings on a per file basis
  QString configN;
  if(coll_->type() == Data::Collection::Base) {
    QList<ConfigInfo> info;
    for(int i = 0; i < Config::maxCustomURLSettings(); ++i) {
      KUrl u = config.readEntry(QString::fromLatin1("URL_%1").arg(i));
      if(!u.isEmpty() && i != configIndex_) {
        configN = QString::fromLatin1("_%1").arg(i);
        ConfigInfo ci;
        ci.cols      = config.readEntry(QLatin1String("ColumnNames") + configN, QStringList());
        ci.widths    = config.readEntry(QLatin1String("ColumnWidths") + configN, QList<int>());
        ci.order     = config.readEntry(QLatin1String("ColumnOrder") + configN, QList<int>());
        ci.colSorted = config.readEntry(QLatin1String("SortColumn") + configN, 0);
        ci.ascSort   = config.readEntry(QLatin1String("SortAscending") + configN, true);
        ci.prevSort  = config.readEntry(QLatin1String("PrevSortColumn") + configN, 0);
        ci.prev2Sort = config.readEntry(QLatin1String("Prev2SortColumn") + configN, 0);
        ci.state     = config.readEntry(QLatin1String("ColumnState") + configN, QString());
        info.append(ci);
      }
    }
    // subtract one since we're writing the current settings, too
    int limit = qMin(info.count(), Config::maxCustomURLSettings()-1);
    for(int i = 0; i < limit; ++i) {
      // starts at one since the current config will be written below
      configN = QString::fromLatin1("_%1").arg(i+1);
      config.writeEntry(QLatin1String("PrevSortColumn")  + configN, info[i].prevSort);
      config.writeEntry(QLatin1String("Prev2SortColumn") + configN, info[i].prev2Sort);
      config.writeEntry(QLatin1String("ColumnState")     + configN, info[i].state);
    }
    configN = QLatin1String("_0");
  }

  /*
  config.writeEntry("ColumnWidths"    + configN, widths);
  config.writeEntry("ColumnOrder"     + configN, order);
  config.writeEntry("SortColumn"      + configN, columnSorted());
  config.writeEntry("SortAscending"   + configN, ascendingSort());

  QStringList colNames;
  for(int col = 0; col < columns(); ++col) {
    colNames += coll_->fieldNameByTitle(columnText(col));
  }
  config.writeEntry("ColumnNames" + configN, colNames);
*/
  QByteArray state = header()->saveState();
  config.writeEntry(QLatin1String("ColumnState") + configN, state.toBase64());
  // the main sort order gets saved in the state
  // the secondary and tertiary need saving separately
  int sortCol2 = sortModel()->secondarySortColumn();
  int sortCol3 = sortModel()->tertiarySortColumn();
  config.writeEntry(QLatin1String("PrevSortColumn") + configN, sortCol2);
  config.writeEntry(QLatin1String("Prev2SortColumn") + configN, sortCol3);
}

QString DetailedListView::sortColumnTitle1() const {
  return model()->headerData(header()->sortIndicatorSection(), Qt::Horizontal).toString();
}

QString DetailedListView::sortColumnTitle2() const {
  return model()->headerData(sortModel()->secondarySortColumn(), Qt::Horizontal).toString();
}

QString DetailedListView::sortColumnTitle3() const {
  return model()->headerData(sortModel()->tertiarySortColumn(), Qt::Horizontal).toString();
}

QStringList DetailedListView::visibleColumns() const {
  QStringList titles;
  for(int i = 0; i < header()->count(); ++i) {
    if(!header()->isSectionHidden(i)) {
      titles << model()->headerData(i, Qt::Horizontal).toString();
    }
  }
  return titles;
}

// can't be const
Tellico::Data::EntryList DetailedListView::visibleEntries() {
  // We could just return the full collection entry list if the filter is 0
  // but printing depends on the sorted order
  Data::EntryList entries;
  for (int i = 0; i < model()->rowCount(); ++i) {
    Data::EntryPtr tmp = model()->data(model()->index(i, 0), EntryPtrRole).value<Data::EntryPtr>();
    if(tmp) {
      entries += tmp;
    }
  }
  return entries;
}

void DetailedListView::selectAllVisible() {
  QModelIndex topLeft = model()->index(0, 0);
  QModelIndex bottomRight = model()->index(model()->rowCount()-1, model()->columnCount()-1);
  QItemSelection selection(topLeft, bottomRight);
  selectionModel()->select(selection, QItemSelectionModel::Select);
}

int DetailedListView::visibleItems() const {
  return model()->rowCount();
}

void DetailedListView::resetEntryStatus() {
  sourceModel()->clearSaveState();
}

void DetailedListView::updateHeaderMenu() {
  m_headerMenu->clear();
  m_headerMenu->addTitle(i18n("View Columns"));
  const int ncols = header()->count();
//  const int ncols = model()->columnCount();
  for(int ncol = 0; ncol < ncols; ++ncol) {
    QAction* act = m_headerMenu->addAction(model()->headerData(ncol, Qt::Horizontal).toString());
    act->setData(ncol);
    act->setCheckable(true);
    act->setChecked(!isColumnHidden(ncol));
  }
}

#include "detailedlistview.moc"
