/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef FETCHDIALOG_H
#define FETCHDIALOG_H

namespace Bookcase {
  class EntryView;
  namespace Fetch {
    class Fetcher;
    class Manager;
    class SearchResult;
  }
}

class KComboBox;
class KLineEdit;
class KPushButton;
class KStatusBar;
class QProgressBar;
class QTimer;
class QCheckBox;

#include "entry.h" //needed for EntryList

#include <kdialogbase.h>
#include <klistview.h>

namespace Bookcase {

/**
 * @author Robby Stephenson
 * @version $Id: fetchdialog.h 766 2004-08-18 01:40:39Z robby $
 */
class FetchDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * Constructor
   */
  FetchDialog(Data::Collection* coll, QWidget* parent, const char* name = 0);
  ~FetchDialog();

signals:
  void signalAddEntries(const Bookcase::Data::EntryList&);

private slots:
  void slotSearchClicked();
  void slotClearClicked();
  void slotAddEntry();
  void slotShowEntry(QListViewItem* item);
  void slotMoveProgress();

  void slotUpdateStatus(const QString& status);
  void slotFetchDone();
  void slotResultFound(const Bookcase::Fetch::SearchResult& result);
  void slotKeyChanged(const QString& key);
  void slotMultipleISBN(bool toggle);
  void slotEditMultipleISBN();

private:
  void startProgress();
  void stopProgress();

  class SearchResultItem : public QListViewItem {
    friend class FetchDialog;
    SearchResultItem(QListView* lv, const Fetch::SearchResult& r);
    const Fetch::SearchResult& m_result;
  };

  Data::Collection* m_coll;
  KComboBox* m_sourceCombo;
  KComboBox* m_keyCombo;
  KLineEdit* m_valueLineEdit;
  KPushButton* m_searchButton;
  QCheckBox* m_multipleISBN;
  KPushButton* m_editISBN;
  KListView* m_listView;
  EntryView* m_entryView;
  KPushButton* m_addButton;
  KStatusBar* m_statusBar;
  QProgressBar* m_progress;
  QTimer* m_timer;

  Fetch::Manager* m_fetchManager;
  bool m_started;
  QStringList m_isbnList;
};

} //end namespace

#endif
