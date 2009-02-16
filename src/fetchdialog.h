/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCHDIALOG_H
#define TELLICO_FETCHDIALOG_H

#include "datavectors.h"

#include <kdialog.h>

#include <QPointer>
#include <QEvent>
#include <QCustomEvent>
#include <QList>
#include <QHash>

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

namespace barcodeRecognition {
  class barcodeRecognitionThread;
}

class KComboBox;
class KLineEdit;
class KPushButton;
class KStatusBar;
class KTextEdit;

class QLabel;
class QProgressBar;
class QTimer;
class QCheckBox;
class QTreeWidget;

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FetchDialog : public KDialog {
Q_OBJECT

public:
  /**
   * Constructor
   */
  FetchDialog(QWidget* parent);
  ~FetchDialog();

public slots:
  void slotResetCollection();

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

  void slotBarcodeRecognized(const QString&);
  void slotBarcodeGotImage(const QImage&);
private:
  void startProgress();
  void stopProgress();
  void setStatus(const QString& text);

  void customEvent(QEvent* event);

  class SearchResultItem;

  KComboBox* m_sourceCombo;
  GUI::ComboBox* m_keyCombo;
  KLineEdit* m_valueLineEdit;
  KPushButton* m_searchButton;
  QCheckBox* m_multipleISBN;
  KPushButton* m_editISBN;
  QTreeWidget* m_treeWidget;
  EntryView* m_entryView;
  KPushButton* m_addButton;
  KPushButton* m_moreButton;
  KStatusBar* m_statusBar;
  QLabel* m_statusLabel;
  QProgressBar* m_progress;
  QTimer* m_timer;
  QPointer<KTextEdit> m_isbnTextEdit;
  QLabel* m_barcodePreview;

  bool m_started;
  int m_resultCount;
  QString m_oldSearch;
  QStringList m_isbnList;
  QStringList m_statusMessages;
  QHash<int, Data::EntryPtr> m_entries;
  QList<Fetch::SearchResult*> m_results;
  int m_collType;

  barcodeRecognition::barcodeRecognitionThread* m_barcodeRecognitionThread;
};

} //end namespace

#endif
