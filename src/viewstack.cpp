/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
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

#include <khtmlview.h>
#include <klocale.h>

#include <qwhatsthis.h>

using Bookcase::ViewStack;

ViewStack::ViewStack(QWidget* parent_, const char* name_/*=0*/) : QWidgetStack(parent_, name_),
    m_entryView(new EntryView(this)), m_iconView(new EntryIconView(this)) {
  QWhatsThis::add(m_entryView->view(), i18n("<qt>The <i>Entry View</i> shows a formatted view of the entry's contents.</qt>"));
  QWhatsThis::add(m_iconView, i18n("<qt>The <i>Icon View</i> shows each entry in the collection or group using "
                                     "an icon, which may be an image in the entry.</qt>"));
}

void ViewStack::clear() {
  m_entryView->clear();
  m_iconView->clear();
}

void ViewStack::refresh() {
  m_entryView->refresh();
  m_iconView->refresh();
}

void ViewStack::showCollection(const Data::Collection* coll_) {
  m_iconView->showCollection(coll_);
  raiseWidget(m_iconView);
}

void ViewStack::showEntry(const Data::Entry* entry_) {
  m_entryView->showEntry(entry_);
  raiseWidget(m_entryView->view());
}

void ViewStack::showEntryGroup(const Data::EntryGroup* group_) {
  m_iconView->showEntryGroup(group_);
  raiseWidget(m_iconView);
}

#include "viewstack.moc"
