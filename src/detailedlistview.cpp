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
#include "models/modelmanager.h"

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

using namespace Tellico;
using Tellico::DetailedListView;

DetailedListView::DetailedListView(QWidget* parent_) : GUI::TreeView(parent_), m_selectionChanging(false) {
  setHeaderHidden(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setAlternatingRowColors(true);
  setRootIsDecorated(false);
  setUniformRowHeights(true);

//  connect(this, SIGNAL(selectionChanged()), SLOT(slotSelectionChanged()));
  connect(this, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(slotDoubleClicked(const QModelIndex&)));

  // header menu
  header()->installEventFilter(this);
  header()->setMinimumSectionSize(20);

  m_headerMenu = new KMenu(this);
  m_columnMenu = new KMenu(this);
  connect(m_columnMenu, SIGNAL(triggered(QAction*)),
          SLOT(slotColumnMenuActivated(QAction*)));

  EntryModel* entryModel = new EntryModel(this);
  EntrySortModel* sortModel = new EntrySortModel(this);
  sortModel->setSortRole(EntryPtrRole);
  sortModel->setSourceModel(entryModel);
  setModel(sortModel);
  setItemDelegate(new DetailedEntryItemDelegate(this));

  ModelManager::self()->setEntryModel(sortModel);

  connect(model(), SIGNAL(headerDataChanged(Qt::Orientation, int, int)), SLOT(updateHeaderMenu()));
  connect(model(), SIGNAL(columnsInserted(const QModelIndex&, int, int)), SLOT(hideNewColumn(const QModelIndex&, int, int)));
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

  // always hide tables and paragraphs
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    Data::FieldPtr field = model()->headerData(ncol, Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field) {
      if(field->type() == Data::Field::Table || field->type() == Data::Field::Para) {
        hideColumn(ncol);
      }
    } else {
      myDebug() << "no field for col" << ncol;
    }
  }

  // because some of the fields got hidden...
  updateHeaderMenu();
  checkHeader();

  sortModel()->setSortColumn(config.readEntry(QLatin1String("SortColumn") + configN, -1));
  sortModel()->setSecondarySortColumn(config.readEntry(QLatin1String("PrevSortColumn") + configN, -1));
  sortModel()->setTertiarySortColumn(config.readEntry(QLatin1String("Prev2SortColumn") + configN, -1));
  const int order = config.readEntry(QLatin1String("SortOrder") + configN, static_cast<int>(Qt::AscendingOrder));
  sortModel()->setSortOrder(static_cast<Qt::SortOrder>(order));

  setUpdatesEnabled(false);
  m_loadingCollection = true;
  addEntries(coll_->entries());
  m_loadingCollection = false;
  setUpdatesEnabled(true);

  sortModel()->invalidate();
  header()->setSortIndicator(sortModel()->sortColumn(), sortModel()->sortOrder());
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
  foreach(Data::EntryPtr entry, entries_) {
    QModelIndex index = sourceModel()->indexFromEntry(entry);
    if(index.isValid()) {
      sourceModel()->setData(index, state, SaveStateRole);
    } else {
      myWarning() << "no index found for" << entry->id() << entry->title();
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
void DetailedListView::setEntriesSelected(Data::EntryList entries_) {
  if(entries_.isEmpty()) {
    // don't move this one outside the block since it calls setCurrentItem(0)
    clearSelection();
    return;
  }

  clearSelection();
  EntrySortModel* proxyModel = dynamic_cast<EntrySortModel*>(model());
  foreach(Data::EntryPtr entry, entries_) {
    QModelIndex index = sourceModel()->indexFromEntry(entry);
    if(!proxyModel->mapFromSource(index).isValid()) {
      // clear the filter if we're trying to select an entry that is currently filtered out
      Controller::self()->clearFilter();
      break;
    }
  }
  blockSignals(true);
  foreach(Data::EntryPtr entry, entries_) {
    QModelIndex index = sourceModel()->indexFromEntry(entry);
    selectionModel()->select(proxyModel->mapFromSource(index), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
  //setCurrentIndex(index);
  blockSignals(false);
  QModelIndex index = sourceModel()->indexFromEntry(entries_.first());
  scrollTo(proxyModel->mapFromSource(index));
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

void DetailedListView::slotColumnMenuActivated(QAction* action_) {
  const int col = action_->data().toInt();
  if(col > -1) { // only column actions have data
    const bool isChecked = action_->isChecked();
    setColumnHidden(col, !isChecked);
    // if we're showing a column, resize all sections
    if(isChecked) {
      resizeColumnToContents(col);
      adjustColumnWidths();
    }
  }
  checkHeader();
}

void DetailedListView::showAllColumns() {
  foreach(QAction* action, m_columnMenu->actions()) {
    if(action->isCheckable() && !action->isChecked()) {
      action->trigger();
    }
  }
}

void DetailedListView::hideAllColumns() {
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    hideColumn(ncol);
  }
  foreach(QAction* action, m_columnMenu->actions()) {
    if(action->isCheckable()) {
      action->setChecked(false);
    }
  }
  checkHeader();
}

void DetailedListView::slotRefresh() {
  sortModel()->invalidate();
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
  sourceModel()->modifyField(oldField_, newField_);
}

void DetailedListView::removeField(Tellico::Data::CollPtr, Tellico::Data::FieldPtr field_) {
  sourceModel()->removeFields(Data::FieldList() << field_);
}

void DetailedListView::reorderFields(const Tellico::Data::FieldList& fields_) {
  QStringList columnNames;
  QList<int> columnWidths, columnOrder;
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    // ignore hidden columns
    if(!isColumnHidden(ncol)) {
      columnNames << columnFieldName(ncol);
      columnWidths << columnWidth(ncol);
      columnOrder << header()->visualIndex(ncol);
    }
  }

  sourceModel()->setFields(fields_);

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
  updateHeaderMenu();
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
        ci.sortOrder = config.readEntry(QLatin1String("SortOrder") + configN, static_cast<int>(Qt::AscendingOrder));
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
      config.writeEntry(QLatin1String("SortOrder")       + configN, info[i].sortOrder);
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

  const int sortCol1 = sortModel()->sortColumn();
  const int sortCol2 = sortModel()->secondarySortColumn();
  const int sortCol3 = sortModel()->tertiarySortColumn();
  const int sortOrder = static_cast<int>(sortModel()->sortOrder());
  config.writeEntry(QLatin1String("SortColumn")      + configN, sortCol1);
  config.writeEntry(QLatin1String("PrevSortColumn")  + configN, sortCol2);
  config.writeEntry(QLatin1String("Prev2SortColumn") + configN, sortCol3);
  config.writeEntry(QLatin1String("SortOrder")       + configN, sortOrder);
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

  m_columnMenu->clear();

  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    Data::FieldPtr field = model()->headerData(ncol, Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field && (field->type() == Data::Field::Table || field->type() == Data::Field::Para)) {
      continue;
    }
    QAction* act = m_columnMenu->addAction(model()->headerData(ncol, Qt::Horizontal).toString());
    act->setData(ncol);
    act->setCheckable(true);
    act->setChecked(!isColumnHidden(ncol));
  }
  QAction* columnAction = m_headerMenu->addMenu(m_columnMenu);
  columnAction->setText(i18nc("Noun, Menu name", "Columns"));
  columnAction->setIcon(KIcon(QLatin1String("view-file-columns")));

  m_headerMenu->addSeparator();

  QAction* actShowAll = m_headerMenu->addAction(i18n("Show All Columns"));
  connect(actShowAll, SIGNAL(triggered(bool)), this, SLOT(showAllColumns()));
  QAction* actHideAll = m_headerMenu->addAction(i18n("Hide All Columns"));
  connect(actHideAll, SIGNAL(triggered(bool)), this, SLOT(hideAllColumns()));
  QAction* actResize = m_headerMenu->addAction(KIcon(QLatin1String("zoom-fit-width")), i18n("Resize to Content"));
  connect(actResize, SIGNAL(triggered(bool)), this, SLOT(resizeColumnsToContents()));
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

void DetailedListView::resizeColumnsToContents() {
  for(int ncol = 0; ncol < header()->count(); ++ncol) {
    if(!isColumnHidden(ncol)) {
      resizeColumnToContents(ncol);
    }
  }
}

void DetailedListView::hideNewColumn(const QModelIndex& index_, int start_, int end_) {
  Q_UNUSED(index_);
  for(int ncol = start_; ncol <= end_; ++ncol) {
    hideColumn(ncol);
  }
  updateHeaderMenu(); // make sure to update checkable actions
}

void DetailedListView::checkHeader() {
  // the header disappears if all columns are hidden, so if the user hides all
  // columns, we turn around and show the title
  //
  // normally, I would expect a check like header()->count() == header()->hiddenSectionCount()
  // to tell me if all sections are hidden, but it often doesn't work, with the hiddenSectionCount()
  // being greater than count()! From testing, if the sizeHint() width is 0, then the header is hidden
  if(!header()->sizeHint().isEmpty()) {
    return;
  }
  // find title action in menu and activate it
  QAction* action = 0;
  foreach(QAction* tryAction, m_columnMenu->actions()) {
    const int ncol = tryAction->data().toInt();
    if(ncol > -1 && columnFieldName(ncol) == QLatin1String("title")) {
      action = tryAction;
      break;
    }
  }
  if(action) {
    action->setChecked(true);
    const int col = action->data().toInt();
    // calling slotColumnMenuActivated() would be infinite loop
    setColumnHidden(col, false);
    resizeColumnToContents(col);
  } else {
    myDebug() << "found no action to show, still empty header!";
  }
}

QString DetailedListView::columnFieldName(int ncol_) const {
  Data::FieldPtr field = model()->headerData(ncol_, Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
  return field ? field->name() : QString();
}

#include "detailedlistview.moc"
