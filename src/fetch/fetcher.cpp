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

#include <kconfig.h>

using Tellico::Fetch::Fetcher;
using Tellico::Fetch::SearchResult;

void Fetcher::readConfig(KConfig* config_, const QString& group_) {
  KConfigGroupSaver groupSaver(config_, group_);
  QString s = config_->readEntry("Name");
  if(!s.isEmpty()) {
    m_name = s;
  }
  m_updateOverwrite = config_->readBoolEntry("UpdateOverwrite", false);
  // be sure to read config for subclass
  readConfigHook(config_, group_);
}

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
