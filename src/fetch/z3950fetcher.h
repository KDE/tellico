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

#ifndef Z3950FETCHER_H
#define Z3950FETCHER_H

namespace Tellico {
  class XSLTHandler;
  namespace Data {
    class Entry;
  }
}

class KLineEdit;
class KIntSpinBox;
class KComboBox;

#include "fetcher.h"
#include "configwidget.h"

#include <qintdict.h>

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 * @version $Id: z3950fetcher.h 960 2004-11-18 05:41:01Z robby $
 */
class Z3950Fetcher : public Fetcher {
Q_OBJECT

public:
  Z3950Fetcher(QObject* parent, const char* name = 0);
  Z3950Fetcher(const QString& name, const QString& host, uint port, const QString& dbname,
               const QString& user, const QString& password, const QString& sourceCharSet,
               QObject* parent);

  virtual ~Z3950Fetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value, bool multiple);
  // amazon can search title, person, isbn, or keyword. No Raw for now.
  virtual bool canSearch(FetchKey k) const { return k != FetchFirst && k != FetchLast && k != Raw; }
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);
  virtual Type type() const { return Z3950; }
  virtual bool canFetch(Data::Collection::Type type) {
    return type == Data::Collection::Book || type == Data::Collection::Bibtex;
  }
  virtual void readConfig(KConfig* config, const QString& group);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent);

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent, Z3950Fetcher* fetcher = 0);

    virtual void saveConfig(KConfig* config_);

  private:
    KLineEdit* m_hostEdit;
    KIntSpinBox* m_portSpinBox;
    KLineEdit* m_databaseEdit;
    KLineEdit* m_userEdit;
    KLineEdit* m_passwordEdit;
    KComboBox* m_charSetCombo;
  };
  friend class ConfigWidget;
  static Z3950Fetcher* libraryOfCongress(QObject* parent);

private:
  void process();

  QString m_name;
  QString m_host;
  uint m_port;
  QString m_dbname;
  QString m_user;
  QString m_password;
  QString m_sourceCharSet;
  QString m_pqn; // prefix query notation
  QString m_esn; // element set name
  QString m_rs; // preferred record syntax
  size_t m_max; // max number of records

  FetchKey m_key;
  QString m_value;
  QIntDict<SearchResult> m_results;
  QIntDict<Data::Entry> m_entries;
  bool m_started;

  static XSLTHandler* s_MARCXMLHandler;
  static XSLTHandler* s_MODSHandler;
  static void initHandlers();
};

  } // end namespace
} // end namespace
#endif
