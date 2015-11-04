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

#ifndef TELLICO_FETCHERINFOLISTITEM_H
#define TELLICO_FETCHERINFOLISTITEM_H

#include "fetcher.h"

#include <QListWidgetItem>

namespace Tellico {
  namespace Fetch {

class FetcherInfo {
public:
  FetcherInfo(Type t, const QString& n, bool o, QString u=QString()) : type(t), name(n), updateOverwrite(o), uuid(u) {}
  Type type;
  QString name;
  bool updateOverwrite;
  QString uuid;
};

  } // end namespace

class FetcherInfoListItem : public QListWidgetItem {
public:
  explicit FetcherInfoListItem(const Fetch::FetcherInfo& info,
                          const QString& groupName = QString());
  FetcherInfoListItem(QListWidget* parent, const Fetch::FetcherInfo& info,
                 const QString& groupName = QString());

  void setConfigGroup(const QString& s);
  const QString& configGroup() const;
  Fetch::Type fetchType() const;
  void setUpdateOverwrite(bool b);
  bool updateOverwrite() const;
  void setNewSource(bool b);
  bool isNewSource() const;
  QString uuid() const;
  void setFetcher(Fetch::Fetcher::Ptr fetcher);
  Fetch::Fetcher::Ptr fetcher() const;

private:
  Fetch::FetcherInfo m_info;
  QString m_configGroup;
  bool m_newSource;
  Fetch::Fetcher::Ptr m_fetcher;
};

} // end namespace
#endif
