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

#ifndef TELLICO_ENTRYVIEW_H
#define TELLICO_ENTRYVIEW_H

#include "datavectors.h"

#include <khtml_part.h>
#include <khtmlview.h>

#include <QPointer>

class KRun;
class KTemporaryFile;

namespace Tellico {
  class XSLTHandler;
  class ImageFactory;
  class StyleOptions;

/**
 * @author Robby Stephenson
 */
class EntryView : public KHTMLPart {
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

signals:
  void signalAction(const KUrl& url);

public slots:
  /**
   * Helper function to refresh view.
   */
  void slotRefresh();

private slots:
  /**
   * Open a URL.
   *
   * @param url The URL to open
   */
  void slotOpenURL(const KUrl& url);
  void slotReloadEntry();
  void slotResetColors();

private:
  void resetColors();

  Data::EntryPtr m_entry;
  XSLTHandler* m_handler;
  QString m_xsltFile;
  QString m_textToShow;

  // to run any clicked processes
  QPointer<KRun> m_run;
  KTemporaryFile* m_tempFile;
  bool m_useGradientImages : 1;
  bool m_checkCommonFile : 1;
};

// stupid naming on my part, I need to subclass the view to
// add a slot. EntryView is really a part though
class EntryViewWidget : public KHTMLView {
Q_OBJECT
public:
  EntryViewWidget(KHTMLPart* part, QWidget* parent);
public slots:
  void copy();
};

} //end namespace
#endif
