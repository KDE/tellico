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

static const int MAX_PARAMS = 2*16;

/**
 * The XSLTHandler contains all the code which uses XSLT processing to generate HTML or to
 * translate to other formats.
 *
 * @author Robby Stephenson
 * @version $Id: xslthandler.h 288 2003-11-10 06:11:56Z robby $
 */
class XSLTHandler {

public: 
  /**
   * @param xsltText The XSLT text
   */
  XSLTHandler(const QString& xsltText=QString::null);
  /**
   */
  ~XSLTHandler();

  /**
   * Set the XSLT text
   *
   * @param text The XSLT text
   */
  void setXSLTText(const QString& text);
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
   * @param encodedUTF8 Whether the text is encoded in utf-8 or not
   * @return The transformed text
   */
  QString applyStylesheet(const QString& text, bool encodedUTF8);
//  QString applyStylesheetToFile(const QString& file_);

private:
  void init();
  QString process();

  xsltStylesheetPtr m_stylesheet;
  xmlDocPtr m_docIn, m_docOut;

  int m_numParams;
  const char* m_params[MAX_PARAMS + 1];
};

#endif
