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

#ifndef ENTRYVIEW_H
#define ENTRYVIEW_H

class KRun;

#include <khtml_part.h>

#include <qguardedptr.h>

namespace Bookcase {
  class XSLTHandler;
  namespace Data {
    class Entry;
  }

/**
 * @author Robby Stephenson
 * @version $Id: entryview.h 386 2004-01-24 05:12:28Z robby $
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
   * Clear the widget and set Entry pointer to NULL
   */
  void clear();
  /**
   * Uses the xslt handler to convert an entry to html, and then writes that html to the view
   *
   * @param entry The entry to show
   */
  void showEntry(const Data::Entry* const entry);
  /**
   * Helper function to refresh view.
   */
  void refresh();
  /**
   * Sets the XSLT file used to show the entry. The file name is passed, not the path, and the
   * standard directories are searched.
   *
   * @param file The XSLT file name
   */
  void setXSLTFile(const QString& file);
private slots:
  /**
   * Open a URL.
   *
   * @param url The URL to open
   */
  void slotOpenURL(const KURL& url);

private:
  const Data::Entry* m_entry;
  XSLTHandler* m_xsltHandler;
  // need to keep track of xslt file so images used in view can be found
  QString m_xsltFile;
  // keep track if the temp directory has been set in the xslt handler
  bool m_tempDirSet;

  // to run any clicked processes
  QGuardedPtr<KRun> m_run;
};

} //end namespace
#endif
