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
class KLineEdit;
class KComboBox;
class KConfig;

class QCheckBox;

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

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual bool canSearch(FetchKey k) const { return m_args.contains(k) || (m_canUpdate && k == ExecUpdate); }
  virtual bool canUpdate() const { return m_canUpdate; }
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return ExecExternal; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  const QString& execPath() const { return m_path; }

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    explicit ConfigWidget(QWidget* parent = 0, const ExecExternalFetcher* fetcher = 0);
    ~ConfigWidget();

    void readConfig(const KConfigGroup& config);
    virtual void saveConfig(KConfigGroup& config);
    virtual void removed();
    virtual QString preferredName() const;

  private:
    bool m_deleteOnRemove : 1;
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

private slots:
  void slotData();
  void slotError();
  void slotProcessExited();

private:
  static QStringList parseArguments(const QString& str);

  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);
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
  QHash<int, Data::EntryPtr> m_entries; // map from search result id to entry
  QStringList m_errors;
  bool m_deleteOnRemove : 1;
  QString m_newStuffName;
};

  } // end namespace
} // end namespace

#endif
