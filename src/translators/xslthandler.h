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

#ifndef XSLTHANDLER_H
#define XSLTHANDLER_H

#include <kurl.h>

#include <qdom.h>

// for xmlDocPtr
#include <libxml/tree.h>
// for xsltStyleSheetPtr
#include <libxslt/xsltInternals.h>

static const int MAX_PARAMS = 2*16;

namespace Bookcase {

/**
 * The XSLTHandler contains all the code which uses XSLT processing to generate HTML or to
 * translate to other formats.
 *
 * @author Robby Stephenson
 * @version $Id: xslthandler.h 744 2004-08-07 22:00:00Z robby $
 */
class XSLTHandler {

public:
  /**
   * @param xsltFile The XSLT file
   */
  XSLTHandler(const QCString& xsltFile);
  /**
   * @param xsltURL The XSLT URL
   */
  XSLTHandler(const KURL& xsltURL);
  /**
   * @param xsltDoc The XSLT DOM document
   * @param xsltFile The XSLT file, should be a url?
   */
  XSLTHandler(const QDomDocument& xsltDoc, const QCString& xsltFile);
  /**
   */
  ~XSLTHandler();

  bool isValid() const { return (m_stylesheet != NULL); }
  /**
   * Set the XSLT text
   *
   * @param dom The XSLT DOM document
   * @param xsltFile The XSLT file, should be a url?
   */
  void setXSLTDoc(const QDomDocument& dom, const QCString& xsltFile);
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
  QString applyStylesheet(const QCString& text);
  QString applyStylesheet(const QString& text, bool encodedUTF8);

private:
  void init();
  QString process();

  xsltStylesheetPtr m_stylesheet;
  xmlDocPtr m_docIn;
  xmlDocPtr m_docOut;

  int m_numParams;
  const char* m_params[MAX_PARAMS + 1];

  static int s_initCount;
};

} // end namespace
#endif
