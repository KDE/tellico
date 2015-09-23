/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_SRUFETCHER_H
#define TELLICO_SRUFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class KIntSpinBox;
class KComboBox;
class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  class XSLTHandler;
  namespace GUI {
    class LineEdit;
    class ComboBox;
  }
  namespace Fetch {

/**
 * A fetcher for SRU servers.
 * Right now, only MODS is supported.
 *
 * @author Robby Stephenson
 */
class SRUFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  SRUFetcher(QObject* parent);
  SRUFetcher(const QString& name, const QString& host, uint port,
             const QString& dbname, const QString& format,
             QObject* parent);
  /**
   */
  virtual ~SRUFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  // only search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == ISBN || k == Keyword || k == LCCN; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntryHook(uint uid);
  virtual Type type() const { return SRU; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

  static Fetcher::Ptr libraryOfCongress(QObject* parent);

private slots:
  void slotComplete(KJob* job);

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
  bool initMARCXMLHandler();
  bool initMODSHandler();
  bool initSRWHandler();

  QString m_host;
  uint m_port;
  QString m_path;
  QString m_format;

  QHash<int, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;
  XSLTHandler* m_MARCXMLHandler;
  XSLTHandler* m_MODSHandler;
  XSLTHandler* m_SRWHandler;
  bool m_started;
};

class SRUFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const SRUFetcher* fetcher = 0);
  virtual void saveConfigHook(KConfigGroup& config);
  virtual QString preferredName() const;

private slots:
  void slotCheckHost();

private:
  GUI::LineEdit* m_hostEdit;
  KIntSpinBox* m_portSpinBox;
  GUI::LineEdit* m_pathEdit;
  GUI::ComboBox* m_formatCombo;
};

  } // end namespace
} // end namespace
#endif
