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

#ifndef ENTRYVIEW_H
#define ENTRYVIEW_H

class KRun;
class KTempFile;

#include "datavectors.h"

#include <khtml_part.h>

#include <qguardedptr.h>

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
   * @param name QObject name
   */
  EntryView(QWidget* parent, const char* name=0);
  /**
   */
  virtual ~EntryView();

  /**
   * Uses the xslt handler to convert an entry to html, and then writes that html to the view
   *
   * @param entry The entry to show
   */
  void showEntry(Data::EntryPtr entry);

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
  void setXSLTOptions(const StyleOptions& options);
  void setUseGradientImages(bool b) { m_useGradientImages = b; }

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
  void slotOpenURL(const KURL& url);
  void slotReloadEntry();
  void slotResetColors();

private:
  void resetColors();

  Data::EntryPtr m_entry;
  XSLTHandler* m_handler;
  QString m_xsltFile;

  // to run any clicked processes
  QGuardedPtr<KRun> m_run;
  KTempFile* m_tempFile;
  bool m_useGradientImages : 1;
  bool m_checkCommonFile : 1;
};

} //end namespace
#endif
