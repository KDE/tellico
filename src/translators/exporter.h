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

#ifndef TELLICO_EXPORTER_H
#define TELLICO_EXPORTER_H

class KConfig;

class QWidget;
class QString;

#include "../entry.h"
#include "../datavectors.h"

#include <kurl.h>

#include <qobject.h>

namespace Tellico {
  namespace Export {
    enum Options {
      ExportFormatted   = 1 << 0,   // format entries when exported
      ExportUTF8        = 1 << 1,   // valid for some text files, export as utf-8
      ExportImages      = 1 << 2,   // should the images be included?
      ExportForce       = 1 << 3,   // force the export, no confirmation of overwriting
      ExportComplete    = 1 << 4,   // export complete document, including loans, etc.
      ExportProgress    = 1 << 5,   // show progress bar
      ExportClean       = 1 << 6,   // specifically for bibliographies, remove latex commands
      ExportVerifyImages= 1 << 7    // don't put in an image link that's not in the cache
    };

/**
 * @author Robby Stephenson
 */
class Exporter : public QObject {
Q_OBJECT

public:
  Exporter();
  Exporter(Data::CollPtr coll);
  virtual ~Exporter();

  Data::CollPtr collection() const;

  void setURL(const KURL& url_) { m_url = url_; }
  void setEntries(const Data::EntryVec& entries) { m_entries = entries; }
  void setOptions(long options) { m_options = options; reset(); }

  virtual QString formatString() const = 0;
  virtual QString fileFilter() const = 0;
  const KURL& url() const { return m_url; }
  const Data::EntryVec& entries() const { return m_entries; }
  long options() const { return m_options; }

  /**
   * Do the export
   */
  virtual bool exec() = 0;
  /**
   * If changing options in the exporter should cause member variables to reset, implement
   * that here
   */
  virtual void reset() {}

  virtual QWidget* widget(QWidget* parent, const char* name=0) = 0;
  virtual void readOptions(KConfig* config) = 0;
  virtual void saveOptions(KConfig* config) = 0;

private:
  long m_options;
  Data::CollPtr m_coll;
  Data::EntryVec m_entries;
  KURL m_url;
};

  } // end namespace
} // end namespace
#endif
