/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "guiproxy.h"
#include "cursorsaver.h"
#include "../tellico_debug.h"

#include <KMessageBox>

#include <QWidget>

using Tellico::GUI::Proxy;

QWidget* Proxy::s_widget = nullptr;

void Proxy::setMainWidget(QWidget* widget_) {
  s_widget = widget_;
}

QWidget* Proxy::widget() {
  return s_widget;
}

void Proxy::sorry(const QString& text_, QWidget* widget_/* =0 */) {
  if(text_.isEmpty()) {
    return;
  }
  myLog() << text_;
  if(widget_ || s_widget) {
    GUI::CursorSaver cs(Qt::ArrowCursor);
    KMessageBox::error(widget_ ? widget_ : s_widget, text_);
  }
}
