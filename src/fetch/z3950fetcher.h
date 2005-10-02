/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 *   In addition, as a special exception, the author gives permission to   *
 *   link the code of this program with the OpenSSL library released by    *
 *   the OpenSSL Project (or with modified versions of OpenSSL that use    *
 *   the same license as OpenSSL), and distribute linked combinations      *
 *   including the two.  You must obey the GNU General Public License in   *
 *   all respects for all of the code used other than OpenSSL.  If you     *
 *   modify this file, you may extend this exception to your version of    *
 *   the file, but you are not obligated to do so.  If you do not wish to  *
 *   do so, delete this exception statement from your version.             *
 *                                                                         *
 ***************************************************************************/

#ifndef Z3950FETCHER_H
#define Z3950FETCHER_H

namespace Tellico {
  class XSLTHandler;
  namespace GUI {
    class LineEdit;
  }
}

class KIntSpinBox;
class KComboBox;

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <kconfig.h>

#include <qguardedptr.h>

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
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
  virtual bool canFetch(int type) const;
  virtual void readConfig(KConfig* config, const QString& group);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent, const Z3950Fetcher* fetcher = 0);

    virtual void saveConfig(KConfig* config_);

  private:
    GUI::LineEdit* m_hostEdit;
    KIntSpinBox* m_portSpinBox;
    GUI::LineEdit* m_databaseEdit;
    GUI::LineEdit* m_userEdit;
    GUI::LineEdit* m_passwordEdit;
    KComboBox* m_charSetCombo;
    // have to remember syntax
    QString m_syntax;
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
  QString m_syntax;
  QString m_pqn; // prefix query notation
  QString m_esn; // element set name
  size_t m_max; // max number of records

  QGuardedPtr<KConfig> m_config;
  QString m_configGroup;

  FetchKey m_key;
  QString m_value;
  QMap<int, Data::ConstEntryPtr> m_entries;
  bool m_started;

  static XSLTHandler* s_MARC21XMLHandler;
  static XSLTHandler* s_UNIMARCXMLHandler;
  static XSLTHandler* s_MODSHandler;
  static void initHandlers();
  static QString toXML(const QCString& marc, const QCString& charSet);
};

  } // end namespace
} // end namespace
#endif
