/***************************************************************************
                                xslthandler.h
                             -------------------
    begin                : Wed Jan 22 2003
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

#ifndef XSLTHANDLER_H
#define XSLTHANDLER_H

#include <kurl.h>

#include <qstring.h>

// for xmlDocPtr
#include <libxml/tree.h>
// for xsltStyleSheetPtr
#include <libxslt/xsltInternals.h>

static const int MAX_PARAMS = 16;

/**
 * The XSLTHandler contains all the code which uses XSLT processing to generate HTML or to
 * translate to other formats.
 *
 * @author Robby Stephenson
 * @version $Id: xslthandler.h,v 1.6 2003/03/08 18:24:47 robby Exp $
 */
class XSLTHandler {

public: 
  /**
   * @param xsltFilename The name of the XSLT file
   */
  XSLTHandler(const QString& xsltFilename);
  /**
   * @param xsltURL The URL of the XSLT file
   */
  XSLTHandler(const KURL& xsltURL);
  /**
   */
  ~XSLTHandler();
  
  /**
   * Adds a param
   */
  void addParam(const QCString& name, const QCString& value);
  /**
   * Adds a string param
   */
  void addStringParam(const QCString& name, const QCString& value);
  /**
   * Processes text through the XSLT transformation.
   *
   * @param text The text to be transformed
   * @return The transformed text
   */
  QString applyStylesheet(const QString& text);
  
private:
  void readStylesheet(const QString& xsltFilename);

  xsltStylesheetPtr m_stylesheet;
  xmlDocPtr m_docIn, m_docOut;

  int m_numParams;
  const char* m_params[MAX_PARAMS + 1];
};

#endif
