/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_OPDSFETCHER_H
#define TELLICO_OPDSFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>

class QLabel;
class KUrlRequester;
class KJob;
namespace KIO {
  class StoredTransferJob;
}

namespace Tellico {
  class XSLTHandler;
  namespace GUI {
    class LineEdit;
  }
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class OPDSFetcher : public Fetcher {
Q_OBJECT

public:
  /**
   */
  OPDSFetcher(QObject* parent);
  /**
   */
  virtual ~OPDSFetcher();

  /**
   */
  virtual QString source() const Q_DECL_OVERRIDE;
  virtual QString attribution() const Q_DECL_OVERRIDE;
  virtual QString icon() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return OPDS; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;
  virtual void saveConfigHook(KConfigGroup& config) Q_DECL_OVERRIDE;

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class Reader;
  friend class Reader;
  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  void initXSLTHandler();
  void parseData(const QByteArray& data, bool manualSearch=false);
  bool matchesEntry(Data::EntryPtr entry) const;

  QString m_catalog;
  QString m_searchTemplate;
  QString m_icon;
  QString m_attribution;
  XSLTHandler* m_xsltHandler;

  QHash<uint, Data::EntryPtr> m_entries;
  QPointer<KIO::StoredTransferJob> m_job;
  bool m_started;
};

// utility class for reading the OPDS catalog and finding the search information
class OPDSFetcher::Reader {
public:
  Reader(const QUrl& catalog);
  bool parse();
  bool readSearchTemplate();

  QUrl catalog;
  QByteArray opdsText;
  bool isAcquisition;
  QUrl searchUrl;
  QString searchTemplate;
  QString name;
  QString icon;
  QString attribution;
};

class OPDSFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const OPDSFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup& config) Q_DECL_OVERRIDE;
  virtual QString preferredName() const Q_DECL_OVERRIDE;

private Q_SLOTS:
  void verifyCatalog();

private:
  KUrlRequester* m_catalogEdit;
  QLabel* m_statusLabel;
  QString m_name;
  QString m_searchTemplate;
  QString m_icon;
  QString m_attribution;
};

  } // end namespace
} // end namespace
#endif
