/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_EXECEXTERNALFETCHER_H
#define TELLICO_EXECEXTERNALFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QHash>
#include <QPointer>

class KProcess;
class KUrlRequester;
class KComboBox;
class KConfig;

class QCheckBox;
class QLineEdit;

class ExternalFetcherTest;
namespace Tellico {
  namespace GUI {
    class ComboBox;
    class LineEdit;
    class CollectionTypeCombo;
  }
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class ExecExternalFetcher : public Fetcher {
Q_OBJECT

public:
  ExecExternalFetcher(QObject* parent);
  /**
   */
  virtual ~ExecExternalFetcher();

  virtual QString source() const override;
  virtual bool isSearching() const override { return m_started; }
  virtual bool canSearch(FetchKey k) const override;
  virtual bool canUpdate() const override { return m_canUpdate; }
  virtual void stop() override;
  virtual Data::EntryPtr fetchEntryHook(uint uid) override;
  virtual Type type() const override { return ExecExternal; }
  virtual bool canFetch(int type) const override;
  virtual void readConfigHook(const KConfigGroup& config) override;
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const override;

  const QString& execPath() const { return m_path; }

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent = nullptr, const ExecExternalFetcher* fetcher = nullptr);
    ~ConfigWidget();

    void readConfig(const KConfigGroup& config) override;
    virtual void saveConfigHook(KConfigGroup& config) override;
    virtual void removed() override;
    virtual QString preferredName() const override;

  private:
    bool m_deleteOnRemove;
    QString m_name, m_newStuffName;
    KUrlRequester* m_pathEdit;
    GUI::CollectionTypeCombo* m_collCombo;
    GUI::ComboBox* m_formatCombo;
    QHash<int, QCheckBox*> m_cbDict;
    QHash<int, GUI::LineEdit*> m_leDict;
    QCheckBox* m_cbUpdate;
    GUI::LineEdit* m_leUpdate;
  };
  friend class ConfigWidget;

  static QString defaultName();
  static QString defaultIcon();
  static StringHash allOptionalFields() { return StringHash(); }

private Q_SLOTS:
  void slotData();
  void slotError();
  void slotProcessExited();

private:
  friend class ::ExternalFetcherTest;
  static QStringList parseArguments(const QString& str);

  virtual void search() override;
  virtual FetchRequest updateRequest(Data::EntryPtr entry) override;
  void startSearch(const QStringList& args);

  bool m_started;
  int m_collType;
  int m_formatType;
  QString m_path;
  QHash<FetchKey, QString> m_args;
  bool m_canUpdate : 1;
  QString m_updateArgs;
  QPointer<KProcess> m_process;
  QByteArray m_data;
  QHash<uint, Data::EntryPtr> m_entries; // map from search result id to entry
  QStringList m_errors;
  bool m_deleteOnRemove : 1;
  QString m_newStuffName;
};

  } // end namespace
} // end namespace

#endif
