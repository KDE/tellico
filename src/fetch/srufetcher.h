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
    class ComboBox;
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

#include <qcstring.h> // for QByteArray
#include <qguardedptr.h>

namespace Tellico {
  namespace Fetch {

class SRUConfigWidget;

/**
 * A fetcher for SRU servers.
 * Right now, only MODS is supported.
 *
 * @author Robby Stephenson
 */
class SRUFetcher : public Fetcher {
Q_OBJECT

friend class SRUConfigWidget;

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
  virtual bool canSearch(FetchKey k) const { return k == Title || k == Person || k == ISBN || k == Keyword; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return SRU; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);

  virtual void updateEntry(Data::EntryPtr entry);

  static StringMap customFields();

  virtual ConfigWidget* configWidget(QWidget* parent) const;

  static QString defaultName();

  static Fetcher::Ptr libraryOfCongress(QObject* parent);

private slots:
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotComplete(KIO::Job* job);

private:
  bool initMARCXMLHandler();
  bool initMODSHandler();

  QString m_host;
  uint m_port;
  QString m_path;
  QString m_format;

  QByteArray m_data;
  QMap<int, Data::EntryPtr> m_entries;
  QGuardedPtr<KIO::Job> m_job;
  XSLTHandler* m_MARCXMLHandler;
  XSLTHandler* m_MODSHandler;
  bool m_started;
  QStringList m_fields;
};

class SRUConfigWidget : public ConfigWidget {
Q_OBJECT

friend class SRUFetcher;

public:
  SRUConfigWidget(QWidget* parent_, const SRUFetcher* fetcher = 0);
  virtual void saveConfig(KConfigGroup& config);
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
