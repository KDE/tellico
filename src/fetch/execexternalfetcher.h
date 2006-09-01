/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_EXECEXTERNALFETCHER_H
#define TELLICO_EXECEXTERNALFETCHER_H

#include "fetcher.h"
#include "configwidget.h"
#include "../datavectors.h"

#include <qintdict.h>

class KProcess;
class KURLRequester;
class KLineEdit;
class KComboBox;

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
  ExecExternalFetcher(QObject* parent, const char* name=0);
  /**
   */
  virtual ~ExecExternalFetcher();

  virtual QString source() const;
  virtual bool isSearching() const { return m_started; }
  virtual bool canSearch(FetchKey k) const { return m_args.contains(k); }
  virtual bool canUpdate() const { return m_canUpdate; }
  virtual void search(FetchKey key, const QString& value);
  virtual void updateEntry(Data::EntryPtr entry);
  virtual void stop();
  virtual Data::EntryPtr fetchEntry(uint uid);
  virtual Type type() const { return ExecExternal; }
  virtual bool canFetch(int type) const;
  virtual void readConfigHook(KConfig* config, const QString& group);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  const QString& execPath() const { return m_path; }

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent = 0, const ExecExternalFetcher* fetcher = 0);
    ~ConfigWidget();

    void readConfig(KConfig*);
    virtual void saveConfig(KConfig*);
    virtual void removed();
    virtual QString preferredName() const;

  private:
    bool m_deleteOnRemove : 1;
    QString m_name, m_newStuffName;
    KURLRequester* m_pathEdit;
    GUI::CollectionTypeCombo* m_collCombo;
    GUI::ComboBox* m_formatCombo;
    QIntDict<QCheckBox> m_cbDict;
    QIntDict<GUI::LineEdit> m_leDict;
    QCheckBox* m_cbUpdate;
    GUI::LineEdit* m_leUpdate;
  };
  friend class ConfigWidget;

  static QString defaultName();

private slots:
  void slotData(KProcess* proc, char* buffer, int len);
  void slotError(KProcess* proc, char* buffer, int len);
  void slotProcessExited(KProcess* proc);

private:
  static QStringList parseArguments(const QString& str);

  void startSearch(const QStringList& args);

  bool m_started;
  int m_collType;
  int m_formatType;
  QString m_path;
  QMap<FetchKey, QString> m_args;
  bool m_canUpdate : 1;
  QString m_updateArgs;
  KProcess* m_process;
  QByteArray m_data;
  QMap<int, Data::EntryPtr> m_entries; // map from search result id to entry
  QStringList m_errors;
  bool m_deleteOnRemove : 1;
  QString m_newStuffName;
};

  } // end namespace
} // end namespace

#endif
