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

#include "messagehandler.h"
#include "fetchmanager.h"
#include "../tellico_kernel.h"

#include <kmessagebox.h>

using Tellico::Fetch::ManagerMessage;

// all messages go to manager
void ManagerMessage::send(const QString& message_, Type type_) {
  Fetch::Manager::self()->updateStatus(message_);
  // plus errors get a message box
  if(type_ == Error) {
    KMessageBox::sorry(Kernel::self()->widget(), message_);
  } else if(type_ == Warning) {
    KMessageBox::information(Kernel::self()->widget(), message_);
  }
}

void ManagerMessage::infoList(const QString& message_, const QStringList& list_) {
  KMessageBox::informationList(Kernel::self()->widget(), message_, list_);
}
