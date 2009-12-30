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

#ifndef TELLICO_FETCH_WINECOMFETCHER_H
#define TELLICO_FETCH_WINECOMFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class KLineEdit;
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
class WineComFetcher : public Fetcher {
Q_OBJECT

public:
  WineComFetcher(QObject* parent = 0);
  ~WineComFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  virtual bool canSearch(FetchKey k) const { return k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return WineCom; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent_, const WineComFetcher* fetcher = 0);
    virtual void saveConfig(KConfigGroup&);
    virtual QString preferredName() const;
  private:
    KLineEdit* m_apiKeyEdit;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash optionalFields() { return StringHash(); }

private slots:
  void slotComplete(KJob* job);

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  void initXSLTHandler();
  void doSearch();

  XSLTHandler* m_xsltHandler;
  int m_limit;
  int m_page;
  int m_total;
  int m_numResults;
  int m_offset;

  QHash<int, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
  QString m_apiKey;
};

  }
}
#endif
