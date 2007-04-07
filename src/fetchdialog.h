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
  namespace GUI {
    class ComboBox;
    class ListView;
  }
}

class KComboBox;
class KLineEdit;
class KPushButton;
class KStatusBar;
class KTextEdit;
class QProgressBar;
class QTimer;
class QCheckBox;

#include "datavectors.h"

#include <kdialogbase.h>

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
  void slotMoreClicked();
  void slotShowEntry();
  void slotMoveProgress();

  void slotStatus(const QString& status);
  void slotUpdateStatus();

  void slotFetchDone(bool checkISBN = true);
  void slotResultFound(Tellico::Fetch::SearchResult* result);
  void slotKeyChanged(int);
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

  class SearchResultItem;

  KComboBox* m_sourceCombo;
  GUI::ComboBox* m_keyCombo;
  KLineEdit* m_valueLineEdit;
  KPushButton* m_searchButton;
  QCheckBox* m_multipleISBN;
  KPushButton* m_editISBN;
  GUI::ListView* m_listView;
  EntryView* m_entryView;
  KPushButton* m_addButton;
  KPushButton* m_moreButton;
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
