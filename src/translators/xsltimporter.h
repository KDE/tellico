/***************************************************************************
                               xsltimporter.h
                             -------------------
    begin                : Wed Sep 24 2003
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

#ifndef XSLTIMPORTER_H
#define XSLTIMPORTER_H

class KURLRequester;

class QWidget;

#include "textimporter.h"

/**
 * The XSLTImporter class takes care of transforming XML data using an XSL stylesheet.
 *
 * @author Robby Stephenson
 * @version $Id: xsltimporter.h 231 2003-10-26 16:46:37Z robby $
 */
class XSLTImporter : public TextImporter  {
Q_OBJECT

public:
  /**
   */
  XSLTImporter(const KURL& url);

  /**
   */
  virtual BCCollection* collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent, const char* name=0);

private:
  BCCollection* m_coll;

  QWidget* m_widget;
  KURLRequester* m_URLRequester;
};

#endif
