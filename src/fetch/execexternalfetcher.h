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
    template <class T> class ComboBoxProxy;
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
  virtual void readConfig(KConfig* config, const QString& group);
  virtual Fetch::ConfigWidget* configWidget(QWidget* parent) const;

  class ConfigWidget : public Fetch::ConfigWidget {
  public:
    ConfigWidget(QWidget* parent_, const ExecExternalFetcher* fetcher = 0);
    ~ConfigWidget();

    virtual void saveConfig(KConfig*);

  private:
    KURLRequester* m_pathEdit;
    typedef GUI::ComboBoxProxy<int> CBProxy;
    CBProxy* m_collCombo;
    CBProxy* m_formatCombo;
    QIntDict<QCheckBox> m_cbDict;
    QIntDict<KLineEdit> m_leDict;
    QCheckBox* m_cbUpdate;
    KLineEdit* m_leUpdate;
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
  QString m_name;
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
};

  } // end namespace
} // end namespace

#endif
