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

class KJob;
namespace KIO {
  class Job;
  class StoredTransferJob;
}

class QCheckBox;
class QRegExpr;
class QRegularExpression;

namespace Tellico {
  namespace GUI {
    class ComboBox;
  }
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class IMDBFetcher : public Fetcher {
Q_OBJECT

public:
  enum Lang {
    EN = 0,
    FR = 1,
    ES = 2,
    DE = 3,
    IT = 4,
    PT = 5
  };

  IMDBFetcher(QObject* parent);
  /**
   */
  virtual ~IMDBFetcher();

  virtual QString source() const Q_DECL_OVERRIDE;
  virtual bool isSearching() const Q_DECL_OVERRIDE { return m_started; }
  virtual void continueSearch() Q_DECL_OVERRIDE;
  virtual bool canSearch(FetchKey k) const Q_DECL_OVERRIDE;
  virtual void stop() Q_DECL_OVERRIDE;
  virtual Data::EntryPtr fetchEntryHook(uint uid) Q_DECL_OVERRIDE;
  virtual Type type() const Q_DECL_OVERRIDE { return IMDB; }
  virtual bool canFetch(int type) const Q_DECL_OVERRIDE;
  virtual void readConfigHook(const KConfigGroup& config) Q_DECL_OVERRIDE;

  struct LangData {
    QString siteTitle;
    QString title_popular;
    QString match_exact;
    QString match_partial;
    QString match_approx;
    QString result_popular;
    QString result_other;
    QString aka;
    QString director;
    QString writer;
    QString producer;
    QString runtime;
    QString aspect_ratio;
    QString also_known_as;
    QString studio;
    QString cast;
    QString cast1;
    QString cast2;
    QString episodes;
    QString genre;
    QString sound;
    QString color;
    QString language;
    QString certification;
    QString country;
    QString plot;
    QString composer;
  };
  static const LangData& langData(int lang);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const Q_DECL_OVERRIDE;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

private Q_SLOTS:
  void slotComplete(KJob* job);
  void slotRedirection(KIO::Job* job, const QUrl& toURL);

private:
  virtual void search() Q_DECL_OVERRIDE;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) Q_DECL_OVERRIDE;
  static void initRegExps();
  static void deleteRegExps();
  static QRegExp* s_tagRx;
  static QRegExp* s_anchorRx;
  static QRegExp* s_anchorTitleRx;
  static QRegExp* s_anchorNameRx;
  static QRegExp* s_titleRx;
  static const QRegularExpression* s_titleIdRx;
  static int s_instanceCount;

  void doJson(const QString& s, Data::EntryPtr e);
  void doTitle(const QString& s, Data::EntryPtr e);
  void doRunningTime(const QString& s, Data::EntryPtr e);
  void doAspectRatio(const QString& s, Data::EntryPtr e);
  void doAlsoKnownAs(const QString& s, Data::EntryPtr e);
  void doPlot(const QString& s, Data::EntryPtr e, const QUrl& baseURL_);
  void doStudio(const QString& s, Data::EntryPtr e);
  void doPerson(const QString& s, Data::EntryPtr e,
                const QString& imdbHeader, const QString& fieldName);
  void doCast(const QString& s, Data::EntryPtr e, const QUrl& baseURL_);
  void doLists(const QString& s, Data::EntryPtr e);
  void doLists2(const QString& s, Data::EntryPtr e);
  void doRating(const QString& s, Data::EntryPtr e);
  void doCover(const QString& s, Data::EntryPtr e, const QUrl& baseURL);
  void doEpisodes(const QString& s, Data::EntryPtr e, const QUrl& baseURL);

  void parseSingleTitleResult();
  void parseMultipleTitleResults();
  void parseTitleBlock(const QString& str);
  Data::EntryPtr parseEntry(const QString& str);
  void configureJob(QPointer<KIO::StoredTransferJob> job);

  QString m_text;
  QHash<uint, Data::EntryPtr> m_entries;
  QHash<uint, QUrl> m_matches;
  // if a new search is started, m_matches is cleared
  // but we might still need to recover an entry by uid
  QHash<uint, QUrl> m_allMatches;
  QPointer<KIO::StoredTransferJob> m_job;

  bool m_started;
  bool m_fetchImages;

  QString m_host;
  int m_numCast;
  QUrl m_url;
  bool m_redirected;
  int m_limit;
  Lang m_lang;

  QString m_popularTitles;
  QString m_exactTitles;
  QString m_partialTitles;
  QString m_approxTitles;
  enum TitleBlock { Unknown = 0, Popular = 1, Exact = 2, Partial = 3, Approx = 4 };
  TitleBlock m_currentTitleBlock;
  int m_countOffset;
};

class IMDBFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher = nullptr);

  virtual void saveConfigHook(KConfigGroup& config) Q_DECL_OVERRIDE;
  virtual QString preferredName() const Q_DECL_OVERRIDE;

private Q_SLOTS:
  void slotSiteChanged();

private:
  QCheckBox* m_fetchImageCheck;
  QSpinBox* m_numCast;
};

  } // end namespace
} // end namespace

#endif
