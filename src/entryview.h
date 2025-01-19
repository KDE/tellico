/***************************************************************************
    Copyright (C) 2003-2020 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ENTRYVIEW_H
#define TELLICO_ENTRYVIEW_H

#include "datavectors.h"

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QPrinter>

class QTemporaryFile;

namespace Tellico {
  class XSLTHandler;
  class ImageFactory;
  class StyleOptions;

/**
 * @author Robby Stephenson
 */
class EntryView : public QWebEngineView {

Q_OBJECT

public:
  /**
   * The EntryView shows a HTML representation of the data in the entry.
   *
   * @param parent QWidget parent
   */
  EntryView(QWidget* parent);
  /**
   */
  virtual ~EntryView();

  /**
   * Uses the xslt handler to convert an entry to html, and then writes that html to the view
   *
   * @param entry The entry to show
   */
  void showEntry(Data::EntryPtr entry);
  void showText(const QString& text);

  /**
   * Clear the widget and set Entry pointer to NULL
   */
  void clear();
  /**
   * Sets the XSLT file. If the file name does not start with a back-slash, then the
   * standard directories are searched.
   *
   * @param file The XSLT file name
   */
  void setXSLTFile(const QString& file);
  void addXSLTStringParam(const QByteArray& name, const QByteArray& value);
  void setXSLTOptions(const StyleOptions& options);
  void setUseGradientImages(bool b) { m_useGradientImages = b; }
  void resetView();

Q_SIGNALS:
  void signalTellicoAction(const QUrl& url);

public Q_SLOTS:
  void copy();
  /**
   * Helper function to refresh view.
   */
  void slotRefresh();
  void showEntries(Tellico::Data::EntryList entries);

private Q_SLOTS:
  void slotReloadEntry();

protected:
  void changeEvent(QEvent* event) override;

private Q_SLOTS:
  void slotPrint();

private:
  void resetColors();
  void contextMenuEvent(QContextMenuEvent* event) override;

  Data::EntryPtr m_entry;
  XSLTHandler* m_handler;
  QString m_xsltFile;
  QString m_textToShow;

  QTemporaryFile* m_tempFile;
  bool m_useGradientImages;
  bool m_checkCommonFile;
  QPrinter m_printer;
};

class EntryViewPage : public QWebEnginePage {
Q_OBJECT
public:
  EntryViewPage(QWidget* parent);

Q_SIGNALS:
  void signalTellicoAction(const QUrl& url);

protected:
  virtual bool acceptNavigationRequest(const QUrl& url, QWebEnginePage::NavigationType type, bool isMainFrame) override;
  virtual QWebEnginePage* createWindow(QWebEnginePage::WebWindowType type) override;

private:
  void openExternalLink(const QUrl& url);
};

} //end namespace
#endif
