/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
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
  // "%1" is replaced in the process command, consider it a keyword
  virtual bool canSearch(FetchKey k) const { return k == Keyword; }
  virtual void search(FetchKey key, const QString& value, bool multiple);
  virtual void stop();
  virtual Data::Entry* fetchEntry(uint uid);
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
    KLineEdit* m_argsEdit;
    typedef GUI::ComboBoxProxy<int> CBProxy;
    CBProxy* m_collCombo;
  };
  friend class ConfigWidget;

private slots:
  void slotData(KProcess* proc, char* buffer, int len);
  void slotError(KProcess* proc, char* buffer, int len);
  void slotProcessExited(KProcess* proc);

private:
  static QStringList parseArguments(const QString& str);

  bool m_started;
  QString m_name;
  int m_collType;
  QString m_path;
  QString m_args;
  KProcess* m_process;
  QByteArray m_data;
  QMap<int, Data::ConstEntryPtr> m_entries; // map from search result id to entry
};

  } // end namespace
} // end namespace

#endif
