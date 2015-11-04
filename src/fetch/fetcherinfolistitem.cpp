/****************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#include "fetcherinfolistitem.h"
#include "fetchmanager.h"

using Tellico::FetcherInfoListItem;

FetcherInfoListItem::FetcherInfoListItem(const Tellico::Fetch::FetcherInfo& info_, const QString& groupName_)
    : QListWidgetItem(), m_info(info_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()), m_fetcher(0) {
  setData(Qt::DisplayRole, info_.name);
  QPixmap pix = Fetch::Manager::fetcherIcon(info_.type);
  if(!pix.isNull()) {
    setData(Qt::DecorationRole, pix);
  }
}

FetcherInfoListItem::FetcherInfoListItem(QListWidget* parent_, const Tellico::Fetch::FetcherInfo& info_, const QString& groupName_)
    : QListWidgetItem(parent_), m_info(info_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()), m_fetcher(0) {
  setData(Qt::DisplayRole, info_.name);
  QPixmap pix = Fetch::Manager::fetcherIcon(info_.type);
  if(!pix.isNull()) {
    setData(Qt::DecorationRole, pix);
  }
}

void FetcherInfoListItem::setConfigGroup(const QString& s) {
  m_configGroup = s;
  if(m_fetcher) {
    m_fetcher->setConfigGroup(s);
  }
}

const QString& FetcherInfoListItem::configGroup() const {
  return m_configGroup;
}

Tellico::Fetch::Type FetcherInfoListItem::fetchType() const {
  return m_info.type;
}

void FetcherInfoListItem::setUpdateOverwrite(bool b) {
  m_info.updateOverwrite = b;
}

bool FetcherInfoListItem::updateOverwrite() const { 
  return m_info.updateOverwrite;
}

void FetcherInfoListItem::setNewSource(bool b) {
  m_newSource = b;
}

bool FetcherInfoListItem::isNewSource() const {
  return m_newSource;
}

QString FetcherInfoListItem::uuid() const {
  return m_info.uuid;
}

void FetcherInfoListItem::setFetcher(Tellico::Fetch::Fetcher::Ptr fetcher) {
  m_fetcher = fetcher;
  QPixmap pix = Fetch::Manager::fetcherIcon(fetcher);
  if(!pix.isNull()) {
    setData(Qt::DecorationRole, pix);
  }
}

Tellico::Fetch::Fetcher::Ptr FetcherInfoListItem::fetcher() const {
  return m_fetcher;
}
