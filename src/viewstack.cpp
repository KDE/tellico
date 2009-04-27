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

#include "viewstack.h"
#include "entryview.h"
#include "entryiconview.h"
#include "tellico_debug.h"
#include "images/imagefactory.h"
#include "entry.h"

#include <khtmlview.h>
#include <klocale.h>

using Tellico::ViewStack;

ViewStack::ViewStack(QWidget* parent_) : QStackedWidget(parent_),
                     m_entryView(new EntryView(this)), m_iconView(new EntryIconView(this)) {
  addWidget(m_entryView->view());
  addWidget(m_iconView);

  m_entryView->view()->setWhatsThis(i18n("<qt>The <i>Entry View</i> shows a formatted view of the entry's "
                                         "contents.</qt>"));
  m_iconView->setWhatsThis(i18n("<qt>The <i>Icon View</i> shows each entry in the collection or group using "
                                "an icon, which may be an image in the entry.</qt>"));
}

void ViewStack::clear() {
  m_entryView->clear();
  m_iconView->clear();
}

void ViewStack::refresh() {
  m_entryView->slotRefresh();
  m_iconView->refresh();
}

void ViewStack::showEntry(Tellico::Data::EntryPtr entry_) {
  m_entryView->showEntry(entry_);
  setCurrentWidget(m_entryView->view());
}

void ViewStack::showEntries(const Tellico::Data::EntryList& entries_) {
  m_iconView->showEntries(entries_);
  setCurrentWidget(m_iconView);
}

#include "viewstack.moc"
