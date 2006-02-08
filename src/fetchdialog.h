/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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

#include "datavectors.h"

#include <kdialogbase.h>
#include <klistview.h>
#include <ktextedit.h>

#include <qvaluestack.h>
#include <qguardedptr.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FetchDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * Constructor
   */
  FetchDialog(QWidget* parent, const char* name = 0);
  ~FetchDialog();

private slots:
  void slotSearchClicked();
  void slotClearClicked();
  void slotAddEntry();
  void slotShowEntry(QListViewItem* item);
  void slotMoveProgress();

  void slotStatus(const QString& status);
  void slotUpdateStatus();

  void slotFetchDone();
  void slotResultFound(Tellico::Fetch::SearchResult* result);
  void slotKeyChanged(const QString& key);
  void slotSourceChanged(const QString& source);
  void slotMultipleISBN(bool toggle);
  void slotEditMultipleISBN();
  void slotInit();
  void slotLoadISBNList();
  void slotUPC2ISBN();

private:
  void startProgress();
  void stopProgress();
  void setStatus(const QString& text);

  class SearchResultItem : public KListViewItem {
    friend class FetchDialog;
    SearchResultItem(KListView* lv, Fetch::SearchResult* r);
    Fetch::SearchResult* m_result;
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
  int m_resultCount;
  QString m_oldSearch;
  QStringList m_isbnList;
  QStringList m_statusMessages;
  QMap<int, Data::EntryPtr> m_entries;
  QPtrList<Fetch::SearchResult> m_results;
};

} //end namespace

#endif
