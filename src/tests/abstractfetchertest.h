/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#ifndef ABSTRACTFETCHERTEST_H
#define ABSTRACTFETCHERTEST_H

#include <QObject>
#include <QEventLoop>

#include "../datavectors.h"
#include "../fetch/fetcher.h"
#include "../fetch/fetchrequest.h"

class KJob;

class AbstractFetcherTest : public QObject {
Q_OBJECT
public:
  AbstractFetcherTest();

protected:
  bool hasNetwork() const { return m_hasNetwork; }
  Tellico::Data::EntryList doFetch(Tellico::Fetch::Fetcher::Ptr fetcher,
                                   const Tellico::Fetch::FetchRequest& request,
                                   int maxResults = 0 /* means no limit */);

  /**
   * Some data sources change the order in which they return results with multiple values,
   * like authors or directors, so use sort and have some helper functions
   **/
  static QString set(Tellico::Data::EntryPtr entry, const char* field);
  static QString set(const char* value);
  static QString set(const QString& value);

protected:
  QStringList m_resultTitles;

private Q_SLOTS:
  void slotResult(KJob* job);

private:
  QEventLoop m_loop;
  bool m_hasNetwork;
  Tellico::Data::EntryList m_results;
};

#define DO_FETCH(fetcher, request) \
  hasNetwork() ? doFetch(fetcher, request) : Tellico::Data::EntryList(); \
    if(!hasNetwork()) QSKIP("This test requires network access", SkipSingle);

#define DO_FETCH1(fetcher, request, limit) \
  hasNetwork() ? doFetch(fetcher, request, limit) : Tellico::Data::EntryList(); \
    if(!hasNetwork()) QSKIP("This test requires network access", SkipSingle);

#endif
