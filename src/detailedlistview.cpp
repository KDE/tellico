/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#include "detailedlistview.h"
#include "collection.h"
#include "collectionfactory.h"
#include "images/imagefactory.h"
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

#include <QPixmap>
#include <QMouseEvent>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QStyledItemDelegate>

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

DetailedListView::DetailedListView(QWidget* parent_) : GUI::TreeView(parent_), m_selectionChanging(false) {
  setHeaderHidden(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setAlternatingRowColors(true);
  setRootIsDecorated(false);

//  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(slotDoubleClicked(const QModelIndex&)));

  // header menu
  header()->installEventFilter(this);
  header()->setMinimumSectionSize(20);

  m_headerMenu = new KMenu(this);
  connect(m_headerMenu, SIGNAL(triggered(QAction*)),
          SLOT(slotHeaderMenuActivated(QAction*)));

  EntryModel* entryModel = new EntryModel(this);
  EntrySortModel* sortModel = new EntrySortModel(this);
  sortModel->setSortRole(EntryPtrRole);
  sortModel->setSourceModel(entryModel);
  setModel(sortModel);
  setItemDelegate(new DetailedEntryItemDelegate(this));

  connect(model(), SIGNAL(headerDataChanged(Qt::Orientation, int, int)), SLOT(updateHeaderMenu()));
  connect(header(), SIGNAL(sectionCountChanged(int, int)), SLOT(updateHeaderMenu()));
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

  const QString configGroup = QString::fromLatin1("Options - %1").arg(CollectionFactory::typeName(coll_));
  KConfigGroup config(KGlobal::config(), configGroup);

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

  sourceModel()->setImagesAreAvailable(false);
  sourceModel()->setFields(coll_->fields());

  // we're not using saveState() and restoreState() since our columns are variable
  QStringList columnNames = config.readEntry(QLatin1String("ColumnNames") + configN, QStringList());
  QList<int> columnWidths = config.readEntry(QLatin1String("ColumnWidths") + configN, QList<int>());
  QList<int> columnOrder = config.readEntry(QLatin1String("ColumnOrder") + configN, QList<int>());

  // just a broken-world check
  while(columnWidths.size() < columnNames.size()) {
    columnWidths << 0;
  }
  while(columnOrder.size() < columnNames.size()) {
    columnOrder << columnOrder.size();
  }
  QList<int> currentColumnOrder;
  // now restore widths and order
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    int idx = columnNames.indexOf(columnFieldName(ncol));
    // column width of 0 means hidden
    if(idx < 0 || columnWidths.at(idx) <= 0) {
      hideColumn(ncol);
      if(idx > -1) {
        currentColumnOrder << ncol;
      }
    } else {
      setColumnWidth(ncol, columnWidths.at(idx));
      currentColumnOrder << ncol;
    }
  }
  const int maxCount = qMin(currentColumnOrder.size(), columnOrder.size());
  for(int i = 0; i < maxCount; ++i) {
    header()->moveSection(header()->visualIndex(currentColumnOrder.at(i)), columnOrder.at(i));
  }

  // because some of the fields got hidden...
  updateHeaderMenu();
  checkHeader();

  sortModel()->setSecondarySortColumn(config.readEntry(QLatin1String("PrevSortColumn") + configN, -1));
  sortModel()->setTertiarySortColumn(config.readEntry(QLatin1String("Prev2SortColumn") + configN, -1));

  setUpdatesEnabled(false);
  m_loadingCollection = true;
  addEntries(coll_->entries());
  m_loadingCollection = false;
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
  if(!m_loadingCollection) {
    setState(entries_, NewState);
    if(!m_selectionChanging) {
      setEntriesSelected(entries_);
    }
  }
}

void DetailedListView::modifyEntries(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return;
  }
  sourceModel()->modifyEntries(entries_);
  setState(entries_, ModifiedState);
  if(!m_selectionChanging) {
    setEntriesSelected(entries_);
  }
}

void DetailedListView::removeEntries(Tellico::Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    return;
  }
  sourceModel()->removeEntries(entries_);
}

void DetailedListView::setState(Tellico::Data::EntryList entries_, int state) {
  for(QModelIndex index = sourceModel()->index(0, 0); index.isValid(); index = index.sibling(index.row()+1, 0)) {
    foreach(Data::EntryPtr entry, entries_) {
      Data::EntryPtr tmpEntry = sourceModel()->data(index, EntryPtrRole).value<Data::EntryPtr>();
      if(tmpEntry == entry) {
        sourceModel()->setData(index, state, SaveStateRole);
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
  EntrySortModel* proxyModel = dynamic_cast<EntrySortModel*>(model());
  for(QModelIndex index = sourceModel()->index(0, 0); index.isValid();
      index = index.sibling(index.row()+1, 0)) {
    foreach(Data::EntryPtr entry_, entries) {
      Data::EntryPtr tmpEntry = sourceModel()->data(index, EntryPtrRole).value<Data::EntryPtr>();
      if(tmpEntry == entry_) {
        if(!proxyModel->mapFromSource(index).isValid()) {
          Controller::self()->clearFilter();
          break;
        }
      }
    }
  }
  for(QModelIndex index = model()->index(0, 0); index.isValid();
      index = index.sibling(index.row()+1, 0)) {
    foreach(Data::EntryPtr entry_, entries) {
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
  m_selectionChanging = true;
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
  m_selectionChanging = false;
}

void DetailedListView::slotDoubleClicked(const QModelIndex& index_) {
  Data::EntryPtr entry = sourceModel()->data(index_, EntryPtrRole).value<Data::EntryPtr>();
  if(entry) {
    Controller::self()->editEntry(entry);
  }
}

void DetailedListView::slotHeaderMenuActivated(QAction* action_) {
  const int col = action_->data().toInt();
  if(col == -1) { // hide all
    for(int ncol = 0; ncol < header()->count(); ++ncol) {
      hideColumn(ncol);
    }
    foreach(QAction* action, m_headerMenu->actions()) {
      if(action->isCheckable()) {
        action->setChecked(false);
      }
    }
  } else if(col < header()->count()) {
    setColumnHidden(col, !action_->isChecked());
    // if we're showing a column, resize all sections
    if(action_->isChecked()) {
      resizeColumnToContents(col);
      adjustColumnWidths();
    }
  }
  checkHeader();
}

void DetailedListView::slotRefresh() {
}

void DetailedListView::setFilter(Tellico::FilterPtr filter_) {
  static_cast<EntrySortModel*>(sortModel())->setFilter(filter_);
}

Tellico::FilterPtr DetailedListView::filter() const {
  return static_cast<EntrySortModel*>(sortModel())->filter();
}

void DetailedListView::addField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr field) {
  sourceModel()->addFields(Data::FieldList() << field);
}

void DetailedListView::modifyField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr oldField_, Tellico::Data::FieldPtr newField_) {
  Q_UNUSED(oldField_)
  sourceModel()->modifyFields(Data::FieldList() << newField_);
}

void DetailedListView::removeField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr field_) {
  sourceModel()->removeFields(Data::FieldList() << field_);
}

void DetailedListView::reorderFields(const Tellico::Data::FieldList& fields_) {
  sourceModel()->setFields(fields_);
}

void DetailedListView::saveConfig(Tellico::Data::CollPtr coll_, int configIndex_) {
  const QString configGroup = QString::fromLatin1("Options - %1").arg(CollectionFactory::typeName(coll_));
  KConfigGroup config(KGlobal::config(), configGroup);

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
        ci.prevSort  = config.readEntry(QLatin1String("PrevSortColumn") + configN, 0);
        ci.prev2Sort = config.readEntry(QLatin1String("Prev2SortColumn") + configN, 0);
        info.append(ci);
      }
    }
    // subtract one since we're writing the current settings, too
    int limit = qMin(info.count(), Config::maxCustomURLSettings()-1);
    for(int i = 0; i < limit; ++i) {
      // starts at one since the current config will be written below
      configN = QString::fromLatin1("_%1").arg(i+1);
      config.writeEntry(QLatin1String("ColumnNames")     + configN, info[i].cols);
      config.writeEntry(QLatin1String("ColumnWidths")    + configN, info[i].widths);
      config.writeEntry(QLatin1String("ColumnOrder")     + configN, info[i].order);
      config.writeEntry(QLatin1String("PrevSortColumn")  + configN, info[i].prevSort);
      config.writeEntry(QLatin1String("Prev2SortColumn") + configN, info[i].prev2Sort);
      // legacy entry item
      config.deleteEntry(QLatin1String("ColumnState")    + configN);
    }
    configN = QLatin1String("_0");
  }

  QStringList colNames;
  QList<int> widths, order;
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    // ignore hidden columns
    if(!isColumnHidden(ncol)) {
      colNames << columnFieldName(ncol);
      widths << columnWidth(ncol);
      order << header()->visualIndex(ncol);
    }
  }

  config.writeEntry(QLatin1String("ColumnNames") + configN, colNames);
  config.writeEntry(QLatin1String("ColumnWidths") + configN, widths);
  config.writeEntry(QLatin1String("ColumnOrder") + configN, order);

  // the main sort order gets saved in the state
  // the secondary and tertiary need saving separately
  const int sortCol2 = sortModel()->secondarySortColumn();
  const int sortCol3 = sortModel()->tertiarySortColumn();
  config.writeEntry(QLatin1String("PrevSortColumn")  + configN, sortCol2);
  config.writeEntry(QLatin1String("Prev2SortColumn") + configN, sortCol3);
  // remove old entry item
  config.deleteEntry(QLatin1String("ColumnState")    + configN);
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
  // we want the visual order, so use a QMap and sort by visualIndex
  QMap<int, QString> titleMap;
  for(int i = 0; i < header()->count(); ++i) {
    if(!isColumnHidden(i)) {
      titleMap.insert(header()->visualIndex(i), model()->headerData(i, Qt::Horizontal).toString());
    }
  }
  return titleMap.values();
}

// can't be const
Tellico::Data::EntryList DetailedListView::visibleEntries() {
  // We could just return the full collection entry list if the filter is 0
  // but printing depends on the sorted order
  Data::EntryList entries;
  for(int i = 0; i < model()->rowCount(); ++i) {
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
  // we only want to update the menu when the header count and model count agree
  if(model()->columnCount() != header()->count()) {
    myDebug() << "column counts diagree";
    return;
  }
  m_headerMenu->clear();
  m_headerMenu->addTitle(i18n("View Columns"));
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
//    Data::FieldPtr field = model()->headerData(ncol, Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    QAction* act = m_headerMenu->addAction(model()->headerData(ncol, Qt::Horizontal).toString());
    act->setData(ncol);
    act->setCheckable(true);
    act->setChecked(!isColumnHidden(ncol));
  }
  m_headerMenu->addSeparator();
  QAction* act = m_headerMenu->addAction(i18n("Hide All Columns"));
  act->setData(-1); // means hide all
}

void DetailedListView::slotRefreshImages() {
  sourceModel()->setImagesAreAvailable(true);
}

void DetailedListView::adjustColumnWidths() {
  // this function is called when a column is shown
  // reduce all visible columns to their size hint, if they are wider than that
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    if(!isColumnHidden(ncol)) {
      const int width = sizeHintForColumn(ncol);
      if(columnWidth(ncol) > width) {
        resizeColumnToContents(ncol);
      }
    }
  }
}

void DetailedListView::checkHeader() {
  // the header disappears if all columns are hidden, so if the user hides all
  // columns, we turn around and show the title
  //
  // normally, I would expect a check like header()->count() == header()->hiddenSectionCount()
  // to tell me if all sections are hidden, but it often doesn't work, with the hiddenSectionCount()
  // being greater than count()! From testing, if the sizeHint() width is 0, then the header is hidden
  if(header()->sizeHint().isEmpty()) {
    // find title action in menu and activate it
    QAction* action = 0;
    foreach(QAction* tryAction, m_headerMenu->actions()) {
      const int ncol = tryAction->data().toInt();
      if(ncol > -1 && columnFieldName(ncol) == QLatin1String("title")) {
        action = tryAction;
        break;
      }
    }
    if(action) {
      action->activate(QAction::Trigger);
    } else {
      myDebug() << "found no action to show, still empty header!";
    }
  }
}

QString DetailedListView::columnFieldName(int ncol_) const {
  Data::FieldPtr field = model()->headerData(ncol_, Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
  return field ? field->name() : QString();
}

#include "detailedlistview.moc"
