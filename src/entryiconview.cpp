/***************************************************************************
    Copyright (C) 2002-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entryiconview.h"
#include "constants.h"
#include "collection.h"
#include "controller.h"
#include "entry.h"
#include "field.h"
#include "document.h"
#include "models/models.h" // for EntryPtrRole
#include "tellico_kernel.h"

#include <KLocalizedString>

#include <QMenu>
#include <QIcon>
#include <QContextMenuEvent>
#include <QDesktopServices>

namespace {
  static const int ENTRY_ICON_SIZE_PAD = 2;
}

using Tellico::EntryIconView;

EntryIconView::EntryIconView(QWidget* parent_)
    : QListView(parent_), m_maxAllowedIconWidth(DEFAULT_ENTRY_ICON_SIZE) {
  setViewMode(QListView::IconMode);
  setMovement(QListView::Static);
  setDragEnabled(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setResizeMode(QListView::Adjust);
  setWordWrap(true);
  setSpacing(ENTRY_ICON_SIZE_PAD);
  setLayoutMode(QListView::Batched);

  connect(this, &EntryIconView::doubleClicked, this, &EntryIconView::slotDoubleClicked);

  setWhatsThis(i18n("<qt>The <i>Icon View</i> shows each entry in the collection or group using "
                    "an icon, which may be an image in the entry.</qt>"));
}

EntryIconView::~EntryIconView() {
}

void EntryIconView::setModel(QAbstractItemModel* model_) {
  QListView::setModel(model_);
  if(!model_) {
    return;
  }
  connect(model_, &QAbstractItemModel::columnsInserted, this, &EntryIconView::updateModelColumn);
}

void EntryIconView::setMaxAllowedIconWidth(int width_) {
  m_maxAllowedIconWidth = qMax(16, width_);
  QSize iconSize(m_maxAllowedIconWidth, m_maxAllowedIconWidth);
  setIconSize(iconSize);

  QSize gridSize(m_maxAllowedIconWidth + 2*ENTRY_ICON_SIZE_PAD,
                 m_maxAllowedIconWidth + 3*(fontMetrics().lineSpacing() + ENTRY_ICON_SIZE_PAD));
  setGridSize(gridSize);
}

void EntryIconView::slotDoubleClicked(const QModelIndex& index_) {
  Data::EntryPtr entry = index_.data(EntryPtrRole).value<Data::EntryPtr>();
  if(entry) {
    Controller::self()->editEntry(entry);
  }
}

void EntryIconView::contextMenuEvent(QContextMenuEvent* ev_) {
  QMenu menu(this);

  // only insert entry items if one is selected
  QModelIndex index = indexAt(ev_->pos());
  if(index.isValid()) {
    Controller::self()->plugEntryActions(&menu);

    // add a menu item to open each URL field
    Data::EntryPtr entry = index.data(EntryPtrRole).value<Data::EntryPtr>();
    Data::FieldList urlFields;
    foreach(Data::FieldPtr field, Data::Document::self()->collection()->fields()) {
      if(field->type() == Data::Field::URL) {
        urlFields += field;
      }
    }
    foreach(Data::FieldPtr urlField, urlFields) {
      QAction* act = menu.addAction(QIcon::fromTheme(QStringLiteral("bookmarks")),
                                    i18nc("Open URL", "Open %1", urlField->title()));
      const QString value = entry->field(urlField);
      act->setData(value);
      act->setWhatsThis(value);
      act->setEnabled(!value.isEmpty());
      menu.addAction(act);
    }
    if(!urlFields.isEmpty()) {
      connect(&menu, &QMenu::triggered, this, &EntryIconView::slotOpenUrlMenuActivated);
    }

    menu.addSeparator();
  }

  QMenu* sortMenu = Controller::self()->plugSortActions(&menu);
  connect(sortMenu, &QMenu::triggered, this, &EntryIconView::slotSortMenuActivated);

  menu.exec(ev_->globalPos());
}

void EntryIconView::slotSortMenuActivated(QAction* action_) {
  Data::FieldPtr field = action_->data().value<Data::FieldPtr>();
  Q_ASSERT(field);
  if(!field) {
    return;
  }
  // EntryIconModel is a proxy to EntryModel which uses the field list as the full set of columns
  // find which columns matches the field and sort on that column
  for(int i = 0; i < model()->columnCount(); ++i) {
    if(model()->headerData(i, Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>() == field) {
      model()->sort(i);
      break;
    }
  }
}

void EntryIconView::slotOpenUrlMenuActivated(QAction* action_/*=0*/) {
  if(!action_) {
    return;
  }
  const QString link = action_->data().toString();
  if(!link.isEmpty()) {
    QDesktopServices::openUrl(Kernel::self()->URL().resolved(QUrl::fromUserInput(link)));
  }
}

void EntryIconView::updateModelColumn() {
  //default model column is 0
  int modelColumn = 0;
  // iterate over model columns, find the one whose name is "title"
  for(int ncol = 0; ncol < model()->columnCount(); ++ncol) {
    Data::FieldPtr field = model()->headerData(ncol, Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field && field->name() == QLatin1String("title")) {
      modelColumn = ncol;
      break;
    }
  }
  setModelColumn(modelColumn);
}
