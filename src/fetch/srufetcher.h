/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_SRUFETCHER_H
#define TELLICO_SRUFETCHER_H

namespace Tellico {
  class XSLTHandler;
  namespace GUI {
    class LineEdit;
  }
}

class KIntSpinBox;
class KComboBox;
namespace KIO {
  class Job;
}

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <qcstring.h> // for QByteARray
#include <qguardedptr.h>

namespace Tellico {
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
  SRUFetcher(QObject* parent, const char* name = 0);
  SRUFetcher(const QString& name, const QString& host, uint port, const QString& dbname,
             QObject* parent);
  /**
   */
  virtual ~SRUFetcher();

  /**
   */
  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  // only search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const { return k != FetchFirst && k != FetchLast && k != UPC && k != Raw; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return SRU; }
  virtual bool canFetch(int type) const;
  virtual void readConfig(KConfig* config, const QString& group);

  virtual void updateEntry(Data::EntryPtr entry);

  static StringMap customFields();

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const SRUFetcher* fetcher = 0);
    virtual void saveConfig(KConfig* config);

  private:
    GUI::LineEdit* m_hostEdit;
    KIntSpinBox* m_portSpinBox;
    GUI::LineEdit* m_databaseEdit;
  };

  static QString defaultName();

  static Fetcher::Ptr libraryOfCongress(QObject* parent);

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  void initXSLTHandler();

  QString m_name;
  QString m_host;
  uint m_port;
  QString m_dbname;

  QByteArray m_data;
  QMap<int, Data::EntryPtr> m_entries;
  QGuardedPtr<KIO::Job> m_job;
  XSLTHandler* m_xsltHandler;
  bool m_started;
  QStringList m_fields;
};

  } // end namespace
} // end namespace
#endif
