/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GCSTARPLUGINFETCHER_H
#define TELLICO_GCSTARPLUGINFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <qintdict.h>

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

  GCstarPluginFetcher(QObject* parent, const char* name=0);
  /**
   */
  virtual ~GCstarPluginFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual bool canSearch(FetchKey k) const { return k == Title; }

  virtual void search(FetchKey key, const QString& value);
  virtual void updateEntry(Data::EntryPtr entry);
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
  void slotData(KProcess* proc, char* buffer, int len);
  void slotError(KProcess* proc, char* buffer, int len);
  void slotProcessExited(KProcess* proc);

private:
  // map Author, Name, Lang, etc...
  typedef QMap<QString, QVariant> PluginInfo;
  typedef QValueList<PluginInfo> PluginList;
  // map collection type to all available plugins
  typedef QMap<int, PluginList> PluginMap;
  static PluginMap pluginMap;
  static PluginList plugins(int collType);
  // we need to keep track if we've searched for plugins yet and by what method
  enum PluginParse {NotYet, Old, New};
  static PluginParse pluginParse;
  static void readPluginsNew(int collType, const QString& exe);
  static void readPluginsOld(int collType, const QString& exe);
  static QString gcstarType(int collType);

  bool m_started;
  int m_collType;
  QString m_plugin;
  KProcess* m_process;
  QByteArray m_data;
  QMap<int, Data::EntryPtr> m_entries; // map from search result id to entry
  QStringList m_errors;
};

class GCstarPluginFetcher::ConfigWidget : public Fetch::ConfigWidget {
Q_OBJECT

public:
  ConfigWidget(QWidget* parent, const GCstarPluginFetcher* fetcher = 0);
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
