/***************************************************************************
    Copyright (C) 2003-2020 Robby Stephenson <robby@periapsis.org>
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

#include "fetcher.h"
#include "configwidget.h"

#include <QPointer>
#include <QEvent>

class QSpinBox;

class KComboBox;

namespace Tellico {
  class XSLTHandler;
  namespace GUI {
    class LineEdit;
    class ComboBox;
  }
  namespace Fetch {
    class Z3950Connection;

/**
 * @author Robby Stephenson
 */
class Z3950Fetcher : public Fetcher {
Q_OBJECT

public:
  Z3950Fetcher(QObject* parent);
  Z3950Fetcher(QObject* parent, const QString& preset);
  Z3950Fetcher(QObject* parent, const QString& host, int port, const QString& dbName, const QString& syntax);

  virtual ~Z3950Fetcher();

  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual void continueSearch() override;
  virtual bool canSearch(FetchKey k) const override;
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return Z3950; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;
  virtual void saveConfigHook(KConfigGroup& config) override;

  const QString& host() const { return m_host; }
  void setCharacterSet(const QString& qcs, const QString& rcs = QString());

  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields();

protected:
  virtual void customEvent(QEvent* event) override;

private:
  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
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
  QString m_queryCharSet;
  QString m_responseCharSet;
  QString m_syntax;
  QString m_pqn; // prefix query notation
  QString m_esn; // element set name

  QHash<uint, Data::EntryPtr> m_entries;
  bool m_started;
  bool m_done;
  QString m_preset;

  XSLTHandler* m_MARC21XMLHandler;
  XSLTHandler* m_UNIMARCXMLHandler;
  XSLTHandler* m_MODSHandler;

  friend class Z3950Connection;
};

class Z3950Fetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent, const Z3950Fetcher* fetcher = nullptr);
  virtual ~ConfigWidget();
  virtual void saveConfigHook(KConfigGroup& config_) override;
  virtual QString preferredName() const override;

private Q_SLOTS:
  void slotTogglePreset(bool on);
  void slotPresetChanged();

private:
  void loadPresets(const QString& current);

  QCheckBox* m_usePreset;
  GUI::ComboBox* m_serverCombo;
  GUI::LineEdit* m_hostEdit;
  QSpinBox* m_portSpinBox;
  GUI::LineEdit* m_databaseEdit;
  GUI::LineEdit* m_userEdit;
  GUI::LineEdit* m_passwordEdit;
  KComboBox* m_charSetCombo1;
  KComboBox* m_charSetCombo2;
  GUI::ComboBox* m_syntaxCombo;
  // have to remember syntax
  QString m_syntax;
};

  } // end namespace
} // end namespace
#endif
