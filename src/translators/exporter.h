/***************************************************************************
                                 exporter.h
                             -------------------
    begin                : Sat Aug 2 2003
    copyright            : (C) 2003 by Robby Stephenson
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

#include "../bcunit.h"

#include <qstring.h>

/**
 * @author Robby Stephenson
 * @version $Id: exporter.h 216 2003-10-24 00:58:22Z robby $
 */
class Exporter {
public: 
  Exporter(const BCCollection* coll, const BCUnitList& list) : m_coll(coll), m_unitList(list) {}
  virtual ~Exporter() {}

  virtual QWidget* widget(QWidget* parent, const char* name=0) = 0;
  virtual QString formatString() const = 0;
  virtual QString text(bool formatAttributes, bool encodeUTF8) = 0;
  virtual QString fileFilter() const = 0;
  virtual void readOptions(KConfig* config) = 0;
  virtual void saveOptions(KConfig* config) = 0;

protected:
  const BCCollection* collection() const { return m_coll; }
  const BCUnitList& unitList() const { return m_unitList; }

private:
  const BCCollection* m_coll;
  const BCUnitList m_unitList;
};

#endif
