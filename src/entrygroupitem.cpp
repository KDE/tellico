/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entrygroupitem.h"
#include "entry.h"
#include "field.h"
#include "collection.h"

#include <kiconloader.h>

using Tellico::EntryGroupItem;

EntryGroupItem::EntryGroupItem(GUI::ListView* parent_, Data::EntryGroup* group_)
    : GUI::CountedItem(parent_), m_group(group_) {
  setText(0, group_->groupName());
}

// prepend a tab character to always sort the empty group name first
// also check for surname prefixes
QString EntryGroupItem::key(int col_, bool) const {
  if(col_ > 0) {
    return text(col_);
  }

  if(text(col_) == Data::Collection::s_emptyGroupTitle) {
    return QChar('\t');
  }

  if(m_text.isEmpty() || m_text != text(col_)) {
    m_text = text(col_);
    if(Data::Field::autoFormat()) {
      // build a regexp to match surname prefixes
      // complicated by fact that prefix could have an apostrophe
      QString prefixes;
      for(QStringList::ConstIterator it = Data::Field::surnamePrefixList().begin();
                                     it != Data::Field::surnamePrefixList().end();
                                     ++it) {
        prefixes += (*it);
        if(!(*it).endsWith(QChar('\''))) {
          prefixes += QString::fromLatin1("\\s");
        }
        // if it's not the last item, add a pipe
        if(!(*it) != Data::Field::surnamePrefixList().last()) {
          prefixes += QChar('|');
        }
      }
      QRegExp rx(QString::fromLatin1("^(") + prefixes + QChar(')'), false);
      // expensive
      if(rx.search(m_text) > -1) {
        m_key = m_text.mid(rx.matchedLength());
      } else {
        m_key = m_text;
      }
    } else {
      m_key = m_text;
    }
  }

  return m_key;
}

