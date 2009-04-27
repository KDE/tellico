/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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
