/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_REPORTDIALOG_H
#define TELLICO_REPORTDIALOG_H

#include <QDialog>

class QStackedWidget;
class QWebEngineView;
class QTemporaryFile;

namespace Tellico {
  namespace Export {
    class HTMLExporter;
  }
  namespace GUI {
    class ComboBox;
  }

/**
 * @author Robby Stephenson
 */
class ReportDialog : public QDialog {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   */
  ReportDialog(QWidget* parent);
  virtual ~ReportDialog();

public Q_SLOTS:
  /**
   * Regenerate the report.
   */
  void slotRefresh();

private Q_SLOTS:
  void slotGenerate();
  void slotPrint();
  void slotSaveAs();
  void slotUpdateSize();

private:
  void generateChart();
  void generateHtml();
  void generateAllEntries();
  void showText(const QString& text, const QUrl& url);

  QStackedWidget* m_reportView;
  QWebEngineView* m_webView;
  GUI::ComboBox* m_templateCombo;
  Export::HTMLExporter* m_exporter;
  QString m_xsltFile;
  QTemporaryFile* m_tempFile;
};

} // end namespace
#endif
