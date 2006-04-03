/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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

#include <qpixmap.h>
#include <qguardedptr.h>

namespace Tellico {
  namespace Data {
    class EntryGroup;
  }

/**
 * @author Robby Stephenson
 */
class EntryGroupItem : public GUI::CountedItem {
public:
  EntryGroupItem(GUI::ListView* parent, Data::EntryGroup* group, int fieldType);

  virtual bool isEntryGroupItem() const { return true; }
  /**
   * Returns the id reference number of the ParentItem.
   *
   * @return The id number
   */
  Data::EntryGroup* group() const { return m_group; }
  void setGroup(Data::EntryGroup* group) { m_group = group; }

  QPixmap ratingPixmap();

  virtual void setPixmap(int col, const QPixmap& pix);
  virtual void paintCell(QPainter* p, const QColorGroup& cg,
                         int column, int width, int align);
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

  virtual int count() const;

private:
  QGuardedPtr<Data::EntryGroup> m_group;
  int m_fieldType;
  QPixmap m_pix;
  bool m_emptyGroup : 1;

// since I do an expensive RegExp match for the surname prefixes, I want to
// cache the text and the resulting key
  mutable QString m_text;
  mutable QString m_key;
};

}

#endif
