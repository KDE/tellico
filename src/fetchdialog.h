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

namespace Tellico {
  class EntryView;
  namespace Fetch {
    class Fetcher;
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
#include <ktextedit.h>

#include <qintdict.h>
#include <qguardedptr.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 * @version $Id: fetchdialog.h 988 2004-12-02 06:42:31Z robby $
 */
class FetchDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * Constructor
   */
  FetchDialog(QWidget* parent, const char* name = 0);
  ~FetchDialog();

signals:
  void signalAddEntries(const Tellico::Data::EntryList&);

private slots:
  void slotSearchClicked();
  void slotClearClicked();
  void slotAddEntry();
  void slotShowEntry(QListViewItem* item);
  void slotMoveProgress();

  void slotUpdateStatus(const QString& status);
  void slotFetchDone();
  void slotResultFound(const Tellico::Fetch::SearchResult& result);
  void slotKeyChanged(const QString& key);
  void slotSourceChanged(const QString& source);
  void slotMultipleISBN(bool toggle);
  void slotEditMultipleISBN();
  void slotInit();
  void slotLoadISBNList();

private:
  void startProgress();
  void stopProgress();

  class SearchResultItem : public KListViewItem {
    friend class FetchDialog;
    SearchResultItem(KListView* lv, const Fetch::SearchResult& r);
    const Fetch::SearchResult& m_result;
  };

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
  QGuardedPtr<KTextEdit> m_isbnTextEdit;

  bool m_started;
  int m_origCount;
  QStringList m_isbnList;
  QIntDict<Data::Entry> m_entries;
};

} //end namespace

#endif
