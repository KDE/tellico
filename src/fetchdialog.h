/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCHDIALOG_H
#define TELLICO_FETCHDIALOG_H

#include "datavectors.h"

#include <QDialog>
#include <QPointer>
#include <QEvent>
#include <QList>
#include <QHash>

namespace Tellico {
  class EntryView;
  namespace Fetch {
    class Fetcher;
    class FetchResult;
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
class KTextEdit;

class QStatusBar;
class QPushButton;
class QLineEdit;
class QLabel;
class QProgressBar;
class QTimer;
class QCheckBox;
class QTreeWidget;

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class FetchDialog : public QDialog {
Q_OBJECT

public:
  /**
   * Constructor
   */
  FetchDialog(QWidget* parent);
  ~FetchDialog();

public Q_SLOTS:
  void slotResetCollection();

protected:
  void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
  void slotSearchClicked();
  void slotClearClicked();
  void slotAddEntry();
  void slotMoreClicked();
  void slotShowEntry();
  void slotMoveProgress();

  void slotStatus(const QString& status);
  void slotUpdateStatus();

  // for slot connection, can't use a default value for checkIsbn
  void slotFetchDone();
  void slotResultFound(Tellico::Fetch::FetchResult* result);
  void slotKeyChanged(int);
  void slotSourceChanged(const QString& source);
  void slotMultipleISBN(bool toggle);
  void slotEditMultipleISBN();
  void slotInit();
  void slotLoadISBNList();
  void slotISBNTextChanged();
  void slotUPC2ISBN();
  void columnResized(int column);

  void slotBarcodeRecognized(const QString&);
  void slotBarcodeGotImage(const QImage&);

private:
  void fetchDone(bool checkISBN);
  void startProgress();
  void stopProgress();
  void setStatus(const QString& text);

  void openBarcodePreview();
  void closeBarcodePreview();

  void customEvent(QEvent* event) override;

  class FetchResultItem;

  KComboBox* m_sourceCombo;
  GUI::ComboBox* m_keyCombo;
  QLineEdit* m_valueLineEdit;
  QPushButton* m_searchButton;
  QCheckBox* m_multipleISBN;
  QPushButton* m_editISBN;
  QTreeWidget* m_treeWidget;
  EntryView* m_entryView;
  QPushButton* m_addButton;
  QPushButton* m_moreButton;
  QStatusBar* m_statusBar;
  QLabel* m_statusLabel;
  QProgressBar* m_progress;
  QTimer* m_timer;
  QPointer<KTextEdit> m_isbnTextEdit;

  bool m_started;
  int m_resultCount;
  QString m_oldSearch;
  QStringList m_isbnList;
  QStringList m_statusMessages;
  QHash<int, Data::EntryPtr> m_entries;
  QList<Fetch::FetchResult*> m_results;
  int m_collType;
  bool m_treeWasResized;

  QLabel* m_barcodePreview;
  barcodeRecognition::barcodeRecognitionThread* m_barcodeRecognitionThread;
};

} //end namespace

#endif
