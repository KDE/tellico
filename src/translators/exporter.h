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

#ifndef EXPORTER_H
#define EXPORTER_H

class KConfig;

class QWidget;

#include "../entry.h"

#include <qstring.h>

#include <kurl.h>

namespace Bookcase {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: exporter.h 580 2004-03-27 03:34:33Z robby $
 */
class Exporter {
public:
  Exporter(const Data::Collection* coll, const Data::EntryList& list) : m_coll(coll), m_entryList(list) {}
  virtual ~Exporter() {}

  void setURL(const KURL& url_) { m_url = url_; }

  virtual QWidget* widget(QWidget* parent, const char* name=0) = 0;
  virtual QString formatString() const = 0;
  virtual QString text(bool formatFields, bool encodeUTF8) = 0;
  virtual QByteArray data(bool formatFields) = 0;
  virtual QString fileFilter() const = 0;
  virtual void readOptions(KConfig* config) = 0;
  virtual void saveOptions(KConfig* config) = 0;
  virtual bool isText() const = 0;

protected:
  const Data::Collection* collection() const { return m_coll; }
  const Data::EntryList& entryList() const { return m_entryList; }
  const KURL& url() const { return m_url; }

private:
  const Data::Collection* m_coll;
  const Data::EntryList m_entryList;
  KURL m_url;
};

  } // end namespace
} // end namespace
#endif
