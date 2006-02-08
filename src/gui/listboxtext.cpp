/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : $EMAIL
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "listboxtext.h"
#include "../tellico_utils.h"

#include <qpainter.h>

using Tellico::GUI::ListBoxText;

ListBoxText::ListBoxText(QListBox* listbox_, const QString& text_)
    : QListBoxText(listbox_, text_), m_colored(false) {
}

ListBoxText::ListBoxText(QListBox* listbox_, const QString& text_, QListBoxItem* after_)
    : QListBoxText(listbox_, text_, after_), m_colored(false) {
}

int ListBoxText::width(const QListBox* listbox_) const {
  if(m_colored) {
    QFont font = listbox_->font();
    font.setBold(true);
    font.setItalic(true);
    QFontMetrics fm(font);
    return fm.width(text()) + 6;
  } else {
    return QListBoxText::width(listbox_);
  }
}

// I don't want the height to change when colored
// so all the items are at the same level for multi-column boxes
int ListBoxText::height(const QListBox* listbox_) const {
  return QListBoxText::height(listbox_);
}

void ListBoxText::setColored(bool colored_) {
  if(m_colored != colored_) {
    m_colored = colored_;
    listBox()->triggerUpdate(false);
  }
}

void ListBoxText::setText(const QString& text_) {
  QListBoxText::setText(text_);
  listBox()->triggerUpdate(true);
}

// mostly copied from QListBoxText::paint() in Qt 3.1.1
void ListBoxText::paint(QPainter* painter_) {
  if(m_colored) {
    QFont font = painter_->font();
    font.setBold(true);
    font.setItalic(true);
    painter_->setFont(font);
    if(!isSelected()) {
      painter_->setPen(Tellico::contrastColor);
    }
  }
  int itemHeight = height(listBox());
  QFontMetrics fm = painter_->fontMetrics();
  int yPos = ((itemHeight - fm.height()) / 2) + fm.ascent();
  painter_->drawText(3, yPos, text());
}
