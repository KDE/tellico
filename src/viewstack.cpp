/***************************************************************************
    copyright            : (C) 2002-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "viewstack.h"
#include "entryview.h"
#include "entryiconview.h"
#include "tellico_debug.h"
#include "imagefactory.h"
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
