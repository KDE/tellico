/***************************************************************************
                               xsltexporter.h
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

#ifndef XSLTEXPORTER_H
#define XSLTEXPORTER_H

class KURLRequester;

#include "exporter.h"

/**
 * @author Robby Stephenson
 * @version $Id: xsltexporter.h 216 2003-10-24 00:58:22Z robby $
 */
class XSLTExporter : public Exporter {
public: 
  XSLTExporter(const BCCollection* coll, BCUnitList list);

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*) {}
  virtual void saveOptions(KConfig*) {}

private:
  QWidget* m_widget;
  KURLRequester* m_URLRequester;
};

#endif
