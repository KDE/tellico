/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMDBFETCHER_H
#define TELLICO_IMDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QUrl>
#include <QPointer>

class QSpinBox;
class QRadioButton;

class KJob;
namespace KIO {
  class Job;
  class StoredTransferJob;
}

class ImdbFetcherTest;
namespace Tellico {
  namespace GUI {
    class ComboBox;
    class LineEdit;
  }
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class IMDBFetcher : public Fetcher {
Q_OBJECT

public:
  IMDBFetcher(QObject* parent);
  /**
   */
  virtual ~IMDBFetcher();

  virtual QString source() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return IMDB; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);

private:
  friend class ::ImdbFetcherTest;
  static QString searchQuery();
  static QString titleQuery();
  static QString episodeQuery();

  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;

  void configureJob(QPointer<KIO::StoredTransferJob> job);
  Data::EntryPtr readGraphQL(const QString& imdbId, const QString& titleType);
  Data::EntryPtr parseResult(const QByteArray& data);

  QHash<uint, Data::EntryPtr> m_entries;
  QHash<uint, QString> m_matches;
  QHash<uint, QString> m_titleTypes;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;

  enum ImageSize {
    NoImage=0,
    SmallImage=1, // 256x256 max
    MediumImage=2, // 640x640 max
    LargeImage=3 // max returned
  };
  ImageSize m_imageSize;
  int m_numCast;
  QUrl m_url;
  bool m_useSystemLocale;
  QString m_customLocale;
};

class IMDBFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher = nullptr);

  virtual void saveConfigHook(KConfigGroup& config) Q_DECL_OVERRIDE;
  virtual QString preferredName() const Q_DECL_OVERRIDE;

private Q_SLOTS:
  void slotLocaleChanged(int id);

private:
  GUI::ComboBox* m_imageCombo;
  QSpinBox* m_numCast;
  QRadioButton* m_systemLocaleRadioButton;
  QRadioButton* m_customLocaleRadioButton;
  GUI::LineEdit* m_customLocaleEdit;
};

  } // end namespace
} // end namespace

#endif
