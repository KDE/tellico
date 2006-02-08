/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef IMDBFETCHER_H
#define IMDBFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <kurl.h>
#include <kio/job.h>

#include <qcstring.h> // for QByteArray
#include <qmap.h>
#include <qguardedptr.h>
#include <qregexp.h>

class KLineEdit;
class KIntSpinBox;
class QCheckBox;

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class IMDBFetcher : public Fetcher {
Q_OBJECT

public:
  IMDBFetcher(QObject* parent, const char* name=0);
  /**
   */
  virtual ~IMDBFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  // imdb can search title, person
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return IMDB; }
  virtual bool canFetch(int type) const;
  virtual void readConfig(KConfig* config, const QString& group);

  virtual void updateEntry(Data::EntryPtr entry);

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  static StringMap customFields();

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher = 0);
    virtual void saveConfig(KConfig*);

  private:
    KLineEdit* m_hostEdit;
    QCheckBox* m_fetchImageCheck;
    KIntSpinBox* m_numCast;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);
  void slotRedirection(KIO::Job* job, const KURL& toURL);

private:
  static void initRegExps();
  static QRegExp* s_tagRx;
  static QRegExp* s_anchorRx;
  static QRegExp* s_anchorTitleRx;
  static QRegExp* s_anchorNameRx;
  static QRegExp* s_titleRx;

  void doTitle(const QString& s, Data::EntryPtr e);
  void doRunningTime(const QString& s, Data::EntryPtr e);
  void doAlsoKnownAs(const QString& s, Data::EntryPtr e);
  void doPlot(const QString& s, Data::EntryPtr e, const KURL& baseURL_);
  void doPerson(const QString& s, Data::EntryPtr e,
                const QString& imdbHeader, const QString& fieldName);
  void doCast(const QString& s, Data::EntryPtr e, const KURL& baseURL_);
  void doLists(const QString& s, Data::EntryPtr e);
  void doRating(const QString& s, Data::EntryPtr e);
  void doCover(const QString& s, Data::EntryPtr e, const KURL& baseURL);

  void parseSingleTitleResult();
  void parseSingleNameResult();
  void parseMultipleTitleResults();
  void parseTitleBlock(const QString& str);
  void parseMultipleNameResults();
  Data::EntryPtr parseEntry(const QString& str);

  QByteArray m_data;
  QMap<int, Data::EntryPtr> m_entries;
  QMap<int, KURL> m_matches;
  QGuardedPtr<KIO::Job> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;
  bool m_fetchImages;
  QString m_name;
  QString m_host;
  int m_numCast;
  KURL m_url;
  bool m_redirected;
  uint m_limit;
  QStringList m_fields;
};

  } // end namespace
} // end namespace

#endif
