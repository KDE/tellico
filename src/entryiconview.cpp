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
#include "collection.h"
#include "collectionfactory.h"
#include "images/imagefactory.h"
#include "controller.h"
#include "entry.h"
#include "field.h"
#include "document.h"
#include "utils/tellico_utils.h"
#include "models/entrymodel.h"
#include "models/entrysortmodel.h"
#include "tellico_kernel.h"
#include "tellico_debug.h"

#include <KLocalizedString>

#include <QMenu>
#include <QIcon>
#include <QContextMenuEvent>
#include <QDesktopServices>

namespace {
  static const int ENTRY_ICON_SIZE_PAD = 6;
}

using Tellico::EntryIconView;

EntryIconView::EntryIconView(QAbstractItemModel* model_, QWidget* parent_)
    : QListView(parent_), m_maxAllowedIconWidth(MAX_ENTRY_ICON_SIZE) {
  setViewMode(QListView::IconMode);
  setMovement(QListView::Static);
//  setUniformItemSizes(true);
  setDragEnabled(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setResizeMode(QListView::Adjust);
  setWordWrap(true);
  setSpacing(ENTRY_ICON_SIZE_PAD);

  Q_ASSERT(::qobject_cast<EntryModel*>(model_));
  EntrySortModel* sortModel = new EntrySortModel(this);
  sortModel->setSortRole(EntryPtrRole);
  sortModel->setSourceModel(model_);
  setModel(sortModel);

  connect(this, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(slotDoubleClicked(const QModelIndex&)));

  setWhatsThis(i18n("<qt>The <i>Icon View</i> shows each entry in the collection or group using "
                    "an icon, which may be an image in the entry.</qt>"));
}

EntryIconView::~EntryIconView() {
}

void EntryIconView::setMaxAllowedIconWidth(int width_) {
  m_maxAllowedIconWidth = qBound(MIN_ENTRY_ICON_SIZE, width_, MAX_ENTRY_ICON_SIZE);
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
      QAction* act = menu.addAction(QIcon::fromTheme(QLatin1String("bookmarks")),
                                    i18nc("Open URL", "Open %1", urlField->title()));
      const QString value = entry->field(urlField);
      act->setData(value);
      act->setWhatsThis(value);
      act->setEnabled(!value.isEmpty());
      menu.addAction(act);
    }
    if(!urlFields.isEmpty()) {
      connect(&menu, SIGNAL(triggered(QAction*)), SLOT(slotOpenUrlMenuActivated(QAction*)));
    }

    menu.addSeparator();
  }

  QMenu* sortMenu = menu.addMenu(i18n("&Sort By"));
  foreach(Data::FieldPtr field, Data::Document::self()->collection()->fields()) {
    sortMenu->addAction(field->title())->setData(qVariantFromValue(field));
  }
  connect(sortMenu, SIGNAL(triggered(QAction*)), SLOT(slotSortMenuActivated(QAction*)));

  menu.exec(ev_->globalPos());
}

void EntryIconView::slotSortMenuActivated(QAction* action_) {
  Data::FieldPtr field = action_->data().value<Data::FieldPtr>();
  Q_ASSERT(field);
  if(!field) {
    return;
  }
  // could have just put the index of the field in the list as the action data
  // but instead, we need to iterate over the current fields and find the index since EntryTitleModel
  // uses the field list index as the column value
  Data::FieldList fields = Data::Document::self()->collection()->fields();
  for(int i = 0; i < fields.count(); ++i) {
    if(fields.at(i)->name() == field->name()) {
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
