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

#include <kurl.h>

#include <QMap>
#include <QPointer>

class KLineEdit;
class KIntSpinBox;
class KJob;
namespace KIO {
  class Job;
  class StoredTransferJob;
}

class QCheckBox;
class QRegExpr;

namespace Tellico {
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

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void continueSearch();
  // imdb can search title, person
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return IMDB; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  static StringMap customFields();

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher = 0);
    virtual void saveConfig(KConfigGroup& config);
    virtual QString preferredName() const;

  private:
    KLineEdit* m_hostEdit;
    QCheckBox* m_fetchImageCheck;
    KIntSpinBox* m_numCast;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotComplete(KJob* job);
  void slotRedirection(KIO::Job* job, const KUrl& toURL);

private:
  virtual void search(FetchKey key, const QString& value);
  static void initRegExps();
  static QRegExp* s_tagRx;
  static QRegExp* s_anchorRx;
  static QRegExp* s_anchorTitleRx;
  static QRegExp* s_anchorNameRx;
  static QRegExp* s_titleRx;

  void doTitle(const QString& s, Data::EntryPtr e);
  void doRunningTime(const QString& s, Data::EntryPtr e);
  void doAspectRatio(const QString& s, Data::EntryPtr e);
  void doAlsoKnownAs(const QString& s, Data::EntryPtr e);
  void doPlot(const QString& s, Data::EntryPtr e, const KUrl& baseURL_);
  void doPerson(const QString& s, Data::EntryPtr e,
                const QString& imdbHeader, const QString& fieldName);
  void doCast(const QString& s, Data::EntryPtr e, const KUrl& baseURL_);
  void doLists(const QString& s, Data::EntryPtr e);
  void doRating(const QString& s, Data::EntryPtr e);
  void doCover(const QString& s, Data::EntryPtr e, const KUrl& baseURL);

  void parseSingleTitleResult();
  void parseSingleNameResult();
  void parseMultipleTitleResults();
  void parseTitleBlock(const QString& str);
  void parseMultipleNameResults();
  Data::EntryPtr parseEntry(const QString& str);

  QString m_text;
  QMap<int, Data::EntryPtr> m_entries;
  QMap<int, KUrl> m_matches;
  QPointer<KIO::StoredTransferJob> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;
  bool m_fetchImages;

  QString m_host;
  int m_numCast;
  KUrl m_url;
  bool m_redirected;
  int m_limit;
  QStringList m_fields;

  QString m_popularTitles;
  QString m_exactTitles;
  QString m_partialTitles;
  enum TitleBlock { Unknown = 0, Popular = 1, Exact = 2, Partial = 3, SinglePerson = 4};
  TitleBlock m_currentTitleBlock;
  int m_countOffset;
};

  } // end namespace
} // end namespace

#endif
