/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_XMLFETCHER_H
#define TELLICO_XMLFETCHER_H

#include "fetcher.h"
#include "../datavectors.h"

#include <QPointer>
#include <QHash>

class QUrl;
class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {

  class XSLTHandler;

  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class XMLFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  XMLFetcher(QObject* parent);
  /**
   */
  virtual ~XMLFetcher();

  virtual bool isSearching() const override { return m_started; }
  virtual void continueSearch() override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;

protected:
  void setXSLTFilename(const QString& filename);
  int limit() const { return m_limit; }
  void setLimit(int limit);
  XSLTHandler* xsltHandler();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search() override;
  virtual void resetSearch() = 0;
  virtual QUrl searchUrl() = 0;
  virtual void parseData(QByteArray& data) = 0;
  virtual void checkMoreResults(int count) { Q_UNUSED(count); }
  virtual Data::EntryPtr fetchEntryHookData(Data::EntryPtr entry) = 0;

  void initXSLTHandler();
  void doSearch();

  QString m_xsltFilename;
  XSLTHandler* m_xsltHandler;

  QPointer<KIO::StoredTransferJob> m_job;
  QHash<uint, Data::EntryPtr> m_entries;

  bool m_started;
  int m_limit;
};

  } // end namespace
} // end namespace
#endif
