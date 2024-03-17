/***************************************************************************
    Copyright (C) 2019 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_COLNECTFETCHER_H
#define TELLICO_COLNECTFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <QPointer>
#include <QVariantMap>

class KJob;
namespace KIO {
  class StoredTransferJob;
}

class ColnectFetcherTest;
namespace Tellico {

  namespace GUI {
    class ComboBox;
    class LineEdit;
  }

  namespace Fetch {

/**
 * A fetcher for colnect.org
 *
 * @author Robby Stephenson
 */
class ColnectFetcher : public Fetcher {
Q_OBJECT

friend class ::ColnectFetcherTest;

public:
  /**
   */
  ColnectFetcher(QObject* parent);
  /**
   */
  virtual ~ColnectFetcher();

  /**
   */
  virtual QString source() const Q_DECL_OVERRIDE;
  virtual QString attribution() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return Colnect; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;

  /**
   * Returns a widget for modifying the fetcher's config.
   */
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  static QString URLize(const QString& name);

  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  void populateEntry(Data::EntryPtr entry, const QVariantList& resultList);
  void populateCoinEntry(Data::EntryPtr entry, const QVariantList& resultList);
  void populateStampEntry(Data::EntryPtr entry, const QVariantList& resultList);
  void populateComicEntry(Data::EntryPtr entry, const QVariantList& resultList);
  void populateCardEntry(Data::EntryPtr entry, const QVariantList& resultList);
  void populateGameEntry(Data::EntryPtr entry, const QVariantList& resultList);
  void loadImage(Data::EntryPtr entry, const QString& fieldName);

  QString imageUrl(const QString& name, const QString& id);
  void readDataList();
  void readItemNames(const QByteArray& item);

  QHash<uint, Data::EntryPtr> m_entries;

  bool m_started;
  QString m_locale;
  QPointer<KIO::StoredTransferJob> m_job;
  QString m_year;
  QString m_category;

  // map from field name to position in result list
  QHash<QString, int> m_colnectFields;
  // color names, for stamp collections
  // players and teams for sports cards
  QHash<QByteArray, QHash<int, QString> > m_itemNames;

  enum ImageSize {
    NoImage=0,
    SmallImage=1, // small is really the thumb size
    LargeImage=2
  };
  ImageSize m_imageSize;
  QString m_countryCode;
  int m_lastCollType;
};

class ColnectFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const ColnectFetcher* fetcher = nullptr);
  virtual void saveConfigHook(KConfigGroup&) Q_DECL_OVERRIDE;
  virtual QString preferredName() const Q_DECL_OVERRIDE;

private Q_SLOTS:
  void slotLangChanged();

private:
  GUI::ComboBox* m_langCombo;
  GUI::ComboBox* m_imageCombo;
  GUI::LineEdit* m_countryEdit;
};

  } // end namespace
} // end namespace
#endif
