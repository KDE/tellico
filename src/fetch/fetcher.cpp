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

#include "fetcher.h"
#include "messagehandler.h"
#include "../entry.h"

using Tellico::Fetch::Fetcher;
using Tellico::Fetch::SearchResult;

void Fetcher::message(const QString& message_, int type_) const {
  if(m_messager) {
    m_messager->send(message_, static_cast<MessageHandler::Type>(type_));
  }
}

void Fetcher::infoList(const QString& message_, const QStringList& list_) const {
  if(m_messager) {
    m_messager->infoList(message_, list_);
  }
}

void Fetcher::updateEntry(Data::EntryPtr) {
  emit signalDone(this);
}

Tellico::Data::EntryPtr SearchResult::fetchEntry() {
  return fetcher->fetchEntry(uid);
}

#include "fetcher.moc"
