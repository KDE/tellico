/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAZONFETCHER_H
#define AMAZONFETCHER_H

namespace Bookcase {
  class XSLTHandler;
}
namespace KIO {
  class Job;
}

#include "fetcher.h"

#include <kurl.h>
#include <kio/job.h>

#include <qcstring.h> // for QByteArray
#include <qintdict.h>
#include <qguardedptr.h>

namespace Bookcase {
  namespace Fetch {

/**
 * A fetcher for Amazon.com.
 *
 * @author Robby Stephenson
 * @version $Id: amazonfetcher.h 762 2004-08-12 01:26:48Z robby $
 */
class AmazonFetcher : public Fetcher {
Q_OBJECT

public:
  enum Site {
    US = 0,
    UK = 1,
    DE = 2,
    JP = 3
  };

  enum ImageSize {
    SmallImage=0,
    MediumImage=1,
    LargeImage=2
  };

  /**
   */
  AmazonFetcher(Site site, Data::Collection* coll, QObject* parent, const char* name = 0);
  /**
   */
  virtual ~AmazonFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  struct SiteData {
    QString title;
    KURL url;
    QString locale;
    QString books;
    QString dvd;
    QString vhs;
    QString music;
    QString classical;
  };
  static const SiteData& siteData(Site site);

  void initXSLTHandler();
  void cleanUp();

  Site m_site;
  bool m_primaryMode;
  ImageSize m_imageSize;
  QString m_token;
  QString m_assoc;

  QByteArray m_data;
  int m_page;
  int m_total;
  QIntDict<SearchResult> m_results;
  QIntDict<Data::Entry> m_entries;
  QGuardedPtr<KIO::Job> m_job;
  XSLTHandler* m_xsltHandler;

  FetchKey m_key;
  QString m_value;
  bool m_started;
  bool m_addLinkField;
};

  } // end namespace
} // end namespace
#endif
