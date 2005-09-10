/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef EXPORTER_H
#define EXPORTER_H

class KConfig;

class QWidget;

#include "../entry.h"
#include "../datavectors.h"

#include <kurl.h>

#include <qstring.h>

namespace Tellico {
  namespace Data {
    class Collection;
  }
  namespace Export {
    enum Options {
      ExportFormatted   = 1 << 0,   // format entries when exported
      ExportUTF8        = 1 << 1,   // valid for some text files, export as utf-8
      ExportImages      = 1 << 2,   // should the images be included?
      ExportForce       = 1 << 3,   // force the export, no confirmation of overwriting
      ExportComplete    = 1 << 4    // export complete document, including loans, etc.
    };

/**
 * @author Robby Stephenson
 */
class Exporter {
public:
  Exporter() : m_options(Export::ExportUTF8 | Export::ExportComplete), m_coll(0) {}
  Exporter(const Data::Collection* coll) : m_options(Export::ExportUTF8), m_coll(coll) {}
  virtual ~Exporter() {}

  const Data::Collection* const collection() const;

  void setURL(const KURL& url_) { m_url = url_; }
  void setEntries(const Data::EntryVec& entries) { m_entries = entries; }
  void setOptions(int options) { m_options = options; }

  virtual QString formatString() const = 0;
  virtual QString fileFilter() const = 0;
  const KURL& url() const { return m_url; }
  const Data::EntryVec& entries() const { return m_entries; }
  int options() const { return m_options; }

  /**
   * Do the export
   */
  virtual bool exec() = 0;

  virtual QWidget* widget(QWidget* parent, const char* name=0) = 0;
  virtual void readOptions(KConfig* config) = 0;
  virtual void saveOptions(KConfig* config) = 0;

private:
  int m_options;
  const Data::Collection* const m_coll;
  Data::EntryVec m_entries;
  KURL m_url;
};

  } // end namespace
} // end namespace
#endif
