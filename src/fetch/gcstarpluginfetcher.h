/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_GCSTARPLUGINFETCHER_H
#define TELLICO_GCSTARPLUGINFETCHER_H

#include "fetcher.h"
#include "configwidget.h"

#include <QShowEvent>
#include <QLabel>
#include <QList>

class QLabel;
class KProcess;

namespace Tellico {
  namespace GUI {
    class ComboBox;
    class CollectionTypeCombo;
  }
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class GCstarPluginFetcher : public Fetcher {
Q_OBJECT

public:

  GCstarPluginFetcher(QObject* parent);
  /**
   */
  virtual ~GCstarPluginFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual bool canSearch(FetchKey k) const { return k == Title; }

  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return GCstarPlugin; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(const KConfigGroup& config);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget;
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotData();
  void slotError();
  void slotProcessExited();

private:
  virtual void search();
  virtual FetchRequest updateRequest(Data::EntryPtr entry);

  // map Author, Name, Lang, etc...
  typedef QMap<QString, QVariant> PluginInfo;
  typedef QList<PluginInfo> PluginList;
  // map collection type to all available plugins
  typedef QHash<int, PluginList> CollectionPlugins;
  static CollectionPlugins collectionPlugins;
  static PluginList plugins(int collType);
  // we need to keep track if we've searched for plugins yet and by what method
  enum PluginParse { NotYet, Old, New };
  static PluginParse pluginParse;
  static void readPluginsNew(int collType, const QString& exe);
  static void readPluginsOld(int collType, const QString& exe);
  static QString gcstarType(int collType);

  bool m_started;
  int m_collType;
  QString m_plugin;
  KProcess* m_process;
  QByteArray m_data;
  QHash<int, Data::EntryPtr> m_entries; // map from search result id to entry
  QStringList m_errors;
};

class GCstarPluginFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  explicit ConfigWidget(QWidget* parent, const GCstarPluginFetcher* fetcher = 0);
  ~ConfigWidget();

  virtual void saveConfig(KConfigGroup& config);
  virtual QString preferredName() const;

private slots:
  void slotTypeChanged();
  void slotPluginChanged();

private:
  void showEvent(QShowEvent* event);

  bool m_needPluginList;
  QString m_originalPluginName;
  GUI::CollectionTypeCombo* m_collCombo;
  GUI::ComboBox* m_pluginCombo;
  QLabel* m_authorLabel;
  QLabel* m_langLabel;
};

  } // end namespace
} // end namespace

#endif
