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

#ifndef XSLTIMPORTER_H
#define XSLTIMPORTER_H

class KURLRequester;

#include "textimporter.h"

namespace Tellico {
  namespace Import {

/**
 * The XSLTImporter class takes care of transforming XML data using an XSL stylesheet.
 *
 * @author Robby Stephenson
 * @version $Id: xsltimporter.h 862 2004-09-15 01:49:51Z robby $
 */
class XSLTImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  XSLTImporter(const KURL& url);

  /**
   */
  virtual Data::Collection* collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent, const char* name=0);
  void setXSLTURL(const KURL& url) { m_xsltURL = url; }

private:
  Data::Collection* m_coll;

  QWidget* m_widget;
  KURLRequester* m_URLRequester;
  KURL m_xsltURL;
};

  } // end namespace
} // end namespace
#endif
