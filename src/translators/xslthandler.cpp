/***************************************************************************
                               xslthandler.cpp
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

#include "xslthandler.h"

//#include <kio/netaccess.h>
//#include <kio/job.h>
#include <kdebug.h>

//#include <qfile.h>

//extern int xmlLoadExtDtdDefaultValue;

#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include <libexslt/exslt.h>

/* some functions to pass to the XSLT libs */
static int writeToQString(void* context, const char* buffer, int len) {
  QString *t = static_cast<QString*>(context);
  *t += QString::fromUtf8(buffer, len);
  return len;
}

static void closeQString(void* context) {
  QString *t = static_cast<QString*>(context);;
  *t += QString::fromLatin1("\n");
}

XSLTHandler::XSLTHandler(const QString& xsltText_/*=null*/) :
    m_stylesheet(0),
    m_docIn(0),
    m_docOut(0),
    m_numParams(0) {
  init();
  if(!xsltText_.isEmpty()) {
    setXSLTText(xsltText_);
  }
}

XSLTHandler::~XSLTHandler() {
  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
  }

  if(m_docIn) {
    xmlFreeDoc(m_docIn);
  }

  if(m_docOut) {
    xmlFreeDoc(m_docOut);
  }
  
  xsltCleanupGlobals();
  xmlCleanupParser();

  for(int i = 0; i < m_numParams; ++i) {
    delete[] m_params[i];
  }
}

inline
void XSLTHandler::init() {
  xmlSubstituteEntitiesDefault(1);
  xmlLoadExtDtdDefaultValue = 0;

  // register the string library
  exsltStrRegister();
  // register the dynamic library
  exsltDynRegister();

  m_params[0] = NULL;
}

void XSLTHandler::setXSLTText(const QString& text_) {
  xmlDocPtr xsltDoc = xmlParseDoc((xmlChar *)text_.local8Bit().data());
  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
  }
  m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
}

void XSLTHandler::addParam(const QCString& name_, const QCString& value_) {
  if(m_numParams < MAX_PARAMS) {
    m_params[m_numParams]     = qstrdup(name_);
    m_params[m_numParams + 1] = qstrdup(value_);
    m_params[m_numParams + 2] = 0;
    m_numParams += 2;
//    kdDebug() << "XSLTHandler::addParam() - " << name_ << ":" << value_ << endl;
  }
}

void XSLTHandler::addStringParam(const QCString& name_, const QCString& value_) {
  addParam(name_, QCString("'") + value_ + QCString("'"));
}

//QString XSLTHandler::applyStylesheetToFile(const QString& file_) {
//  if(!m_stylesheet) {
//    kdDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer!" << endl;
//    return QString::null;
//  }
//
//  m_docIn = xmlParseFile(QFile::encodeName(file_).data());
//
//  return process();
//}

QString XSLTHandler::applyStylesheet(const QString& text_, bool encodedUTF8_) {
  if(!m_stylesheet) {
    kdDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer!" << endl;
    return QString::null;
  }

// ARGH, I don't know which to use...
  if(encodedUTF8_) {
    m_docIn = xmlParseDoc((xmlChar *)text_.utf8().data());
  } else {
    m_docIn = xmlParseDoc((xmlChar *)text_.local8Bit().data());
  }
  return process();
}

QString XSLTHandler::process() {
  if(!m_docIn) {
    kdDebug() << "XSLTHandler::applyStylesheet() - error parsing input string!" << endl;
    return QString::null;
  }

  // returns NULL on error
  m_docOut = xsltApplyStylesheet(m_stylesheet, m_docIn, m_params);
  if(!m_docOut) {
    kdDebug() << "XSLTHandler::applyStylesheet() - error applying stylesheet!" << endl;
    return QString::null;
  }

  QString result;

  xmlOutputBufferPtr outp = xmlOutputBufferCreateIO( (xmlOutputWriteCallback)writeToQString,
                                                     (xmlOutputCloseCallback)closeQString,
                                                     &result, 0);
  if(!outp) {
    kdDebug() << "XSLTHandler::applyStylesheet() - error writing output buffer!" << endl;
    return result;
  }
  
  outp->written = 0;
  
  int num_bytes = xsltSaveResultTo(outp, m_docOut, m_stylesheet);
  if(num_bytes == -1) {
    kdDebug() << "XSLTHandler::applyStylesheet() - error saving output buffer!" << endl;
    return result;
  }
  
  xmlOutputBufferFlush(outp);

  return result;
}
