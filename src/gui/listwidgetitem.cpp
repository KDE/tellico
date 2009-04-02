/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "listwidgetitem.h"

#include <KColorScheme>

using Tellico::GUI::ListWidgetItem;

ListWidgetItem::ListWidgetItem(const QString& text_, QListWidget* parent_)
    : QListWidgetItem(text_, parent_, QListWidgetItem::UserType+1), m_colored(false) {
}

void ListWidgetItem::setColored(bool colored_) {
  if(colored_ == m_colored) {
    return;
  }
  m_colored = colored_;

  QFont font = listWidget()->font();
  if(m_colored) {
    font.setBold(true);
    font.setItalic(true);
  }
  setFont(font);

  QBrush brush = listWidget()->palette().text();
  if(m_colored) {
    KColorScheme cs(QPalette::Active);
    brush = cs.foreground(KColorScheme::PositiveText);
  }
  setForeground(brush);
}
