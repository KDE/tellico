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

namespace Bookcase {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: exporter.h 386 2004-01-24 05:12:28Z robby $
 */
class Exporter {
public: 
  Exporter(const Data::Collection* coll, const Data::EntryList& list) : m_coll(coll), m_entryList(list) {}
  virtual ~Exporter() {}

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

private:
  const Data::Collection* m_coll;
  const Data::EntryList m_entryList;
};

  } // end namespace
} // end namespace
#endif
