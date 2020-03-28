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

#include "messagehandler.h"
#include "fetchmanager.h"
#include "../utils/guiproxy.h"
#include "../utils/cursorsaver.h"

#include <KMessageBox>

using Tellico::Fetch::ManagerMessage;

// all messages go to manager
void ManagerMessage::send(const QString& message_, Type type_) {
  GUI::CursorSaver cs(Qt::ArrowCursor);
  // plus errors get a message box
  if(type_ == Error) {
    KMessageBox::sorry(GUI::Proxy::widget(), message_);
//                       QString(), // caption
//                       KMessageBox::Options(KMessageBox::Notify | KMessageBox::AllowLink));
  } else if(type_ == Warning) {
    KMessageBox::information(GUI::Proxy::widget(), message_);
  } else {
    Fetch::Manager::self()->updateStatus(message_);
  }
}

void ManagerMessage::infoList(const QString& message_, const QStringList& list_) {
  GUI::CursorSaver cs(Qt::ArrowCursor);
  KMessageBox::informationList(GUI::Proxy::widget(), message_, list_);
}
