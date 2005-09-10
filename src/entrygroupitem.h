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

#ifndef ENTRYGROUPITEM_H
#define ENTRYGROUPITEM_H

#include "gui/counteditem.h"

namespace Tellico {
  namespace Data {
    class EntryGroup;
  }

/**
 * @author Robby Stephenson
 */
class EntryGroupItem : public GUI::CountedItem {
public:
  EntryGroupItem(GUI::ListView* parent, Data::EntryGroup* group);

  virtual bool isEntryGroupItem() const { return true; }
  /**
   * Returns the id reference number of the ParentItem.
   *
   * @return The id number
   */
  Data::EntryGroup* group() const { return m_group; }
  /**
   * Returns the key for sorting the listitems. The text used for an empty
   * value should be sorted first, so the returned key is "\t". Since the text may
   * have the number of entries or something added to the name, only check if the
   * text begins with the empty name. Maybe there should be something better.
   *
   * @param col The column number
   * @return The key
   */
  virtual QString key(int col, bool) const;

private:
  Data::EntryGroup* m_group;

// since I do an expensive RegExp match for the surname prefixes, I want to
// cache the text and the resulting key
  mutable QString m_text;
  mutable QString m_key;
};

}

#endif
