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

#include "fetcher.h"
#include "messagehandler.h"

#include <kglobal.h>
#include <KSharedConfig>
#include <KConfigGroup>

using Tellico::Fetch::Fetcher;

Fetcher::Fetcher(QObject* parent) : QObject(parent)
    , QSharedData()
    , m_collectionType(0)
    , m_updateOverwrite(false)
    , m_hasMoreResults(false)
    , m_messager(0) {
}

Fetcher::~Fetcher() {
  KConfigGroup config(KGlobal::config(), m_configGroup);
  saveConfigHook(config);
}

void Fetcher::startSearch(int collType_, FetchKey key_, const QString& value_) {
  m_collectionType = collType_;
  search(key_, value_);
}

void Fetcher::readConfig(const KConfigGroup& config_, const QString& groupName_) {
  m_configGroup = groupName_;

  QString s = config_.readEntry("Name");
  if(!s.isEmpty()) {
    m_name = s;
  }
  m_updateOverwrite = config_.readEntry("UpdateOverwrite", false);
  // be sure to read config for subclass
  readConfigHook(config_);
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

void Fetcher::updateEntry(Tellico::Data::EntryPtr) {
  emit signalDone(this);
}

#include "fetcher.moc"
