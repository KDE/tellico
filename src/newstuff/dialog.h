/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_NEWSTUFF_DIALOG_H
#define TELLICO_NEWSTUFF_DIALOG_H

#include "manager.h"

#include <kdialogbase.h>

class KPushButton;
class KStatusBar;
namespace KIO {
  class Job;
}
namespace KNS {
  class Entry;
  class Provider;
}

class QProgressBar;
class QSplitter;
class QLabel;
class QTextEdit;

namespace Tellico {
  namespace GUI {
    class ListView;
    class CursorSaver;
  }

  namespace NewStuff {

class Dialog : public KDialogBase {
Q_OBJECT

public:
  Dialog(DataType type, QWidget* parent);
  virtual ~Dialog();

  QPtrList<DataSourceInfo> dataSourceInfo() const { return m_manager->dataSourceInfo(); }

private slots:
  void slotProviders(QPtrList<KNS::Provider>* list);
  void slotData(KIO::Job* job, const QByteArray& data);
  void slotResult(KIO::Job* job);
  void slotPreviewResult(KIO::Job* job);

  void slotShowPercent(KIO::Job* job, unsigned long percent);

  void slotSelected(QListViewItem* item);
  void slotInstall();
  void slotDoneInstall(KNS::Entry* entry);

  void slotProviderError();
  void slotMoveProgress();

private:
  class Item;

  void setStatus(const QString& status);
  void addEntry(KNS::Entry* entry);

  Manager* const m_manager;
  DataType m_type;
  QString m_lang;
  QString m_typeName;

  QSplitter* m_split;
  GUI::ListView* m_listView;
  QLabel* m_iconLabel;
  QLabel* m_nameLabel;
  QLabel* m_infoLabel;
  QTextEdit* m_descLabel;
  KPushButton* m_install;
  KStatusBar* m_statusBar;
  QProgressBar* m_progress;
  QTimer* m_timer;
  GUI::CursorSaver* m_cursorSaver;
  KTempFile* m_tempPreviewImage;

  QMap<KIO::Job*, KNS::Provider*> m_jobs;
  QMap<KIO::Job*, QByteArray> m_data;

  QMap<QListViewItem*, KNS::Entry*> m_entryMap;
  QListViewItem* m_lastPreviewItem;
};

  }
}
#endif
