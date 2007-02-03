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

#ifndef TELLICO_Z3950FETCHER_H
#define TELLICO_Z3950FETCHER_H

namespace Tellico {
  class XSLTHandler;
  namespace GUI {
    class LineEdit;
    class ComboBox;
  }
}

class KIntSpinBox;
class KComboBox;

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <qguardedptr.h>

namespace Tellico {
  namespace Fetch {
    class Z3950Connection;

/**
 * @author Robby Stephenson
 */
class Z3950Fetcher : public Fetcher {
Q_OBJECT

public:
  Z3950Fetcher(QObject* parent, const char* name = 0);

  virtual ~Z3950Fetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual void search(FetchKey key, const QString& value);
  virtual void continueSearch();
  // can search title, person, isbn, or keyword. No UPC or Raw for now.
  virtual bool canSearch(FetchKey k) const { return k != FetchFirst && k != FetchLast && k!= UPC && k != Raw; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return Z3950; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(KConfig* config, const QString& group);

  virtual void updateEntry(Data::EntryPtr entry);
  const QString& host() const { return m_host; }

  static StringMap customFields();

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();

protected:
  virtual void customEvent(QCustomEvent* event);

private:
  bool initMARC21Handler();
  bool initUNIMARCHandler();
  bool initMODSHandler();
  void process();
  void handleResult(const QString& result);
  void done();

  Z3950Connection* m_conn;

  QString m_host;
  uint m_port;
  QString m_dbname;
  QString m_user;
  QString m_password;
  QString m_sourceCharSet;
  QString m_syntax;
  QString m_pqn; // prefix query notation
  QString m_esn; // element set name

  QGuardedPtr<KConfig> m_config;
  QString m_configGroup;

  FetchKey m_key;
  QString m_value;
  QMap<int, Data::EntryPtr> m_entries;
  bool m_started;
  QString m_preset;

  XSLTHandler* m_MARC21XMLHandler;
  XSLTHandler* m_UNIMARCXMLHandler;
  XSLTHandler* m_MODSHandler;
  QStringList m_fields;

  friend class Z3950Connection;
};

class Z3950Fetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  ConfigWidget(QWidget* parent, const Z3950Fetcher* fetcher = 0);
  virtual ~ConfigWidget();
  virtual void saveConfig(KConfig* config_);
  virtual QString preferredName() const;

private slots:
  void slotTogglePreset(bool on);
  void slotPresetChanged();

private:
  void loadPresets(const QString& current);

  QCheckBox* m_usePreset;
  GUI::ComboBox* m_serverCombo;
  GUI::LineEdit* m_hostEdit;
  KIntSpinBox* m_portSpinBox;
  GUI::LineEdit* m_databaseEdit;
  GUI::LineEdit* m_userEdit;
  GUI::LineEdit* m_passwordEdit;
  KComboBox* m_charSetCombo;
  GUI::ComboBox* m_syntaxCombo;
  // have to remember syntax
  QString m_syntax;
};

  } // end namespace
} // end namespace
#endif
