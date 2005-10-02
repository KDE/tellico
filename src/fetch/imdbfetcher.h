/***************************************************************************
    copyright            : (C) 2004-2005 by Robby Stephenson
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
  virtual void search(FetchKey key, const QString& value, bool multiple);
  // imdb can search title, person
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person; }
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);
  virtual Type type() const { return IMDB; }
  virtual bool canFetch(int type) const;
  virtual void readConfig(KConfig* config, const QString& group);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const IMDBFetcher* fetcher = 0);

    virtual void saveConfig(KConfig*);

  private:
    KLineEdit* m_hostEdit;
    QCheckBox* m_fetchImageCheck;
  };
  friend class ConfigWidget;

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
  static void doTitle(const QString& s, Data::Entry* e);
  static void doRunningTime(const QString& s, Data::Entry* e);
  static void doAlsoKnownAs(const QString& s, Data::Entry* e);
  static void doPlot(const QString& s, Data::Entry* e, const KURL& baseURL_);
  static void doPerson(const QString& s, Data::Entry* e,
                       const QString& imdbHeader, const QString& fieldName);
  static void doCast(const QString& s, Data::Entry* e, const KURL& baseURL_);
  static void doLists(const QString& s, Data::Entry* e);
  static void doCover(const QString& s, Data::Entry* e, const KURL& baseURL);

  void parseSingleTitleResult();
  void parseSingleNameResult();
  void parseMultipleTitleResults();
  void parseTitleBlock(const QString& str);
  void parseMultipleNameResults();
  Data::Entry* parseEntry(const QString& str);

  QByteArray m_data;
  QMap<int, Data::ConstEntryPtr> m_entries;
  QMap<int, KURL> m_matches;
  QGuardedPtr<KIO::Job> m_job;

  FetchKey m_key;
  QString m_value;
  bool m_started;
  bool m_fetchImages;
  QString m_host;
  KURL m_url;
  bool m_redirected;
};

  } // end namespace
} // end namespace

#endif
