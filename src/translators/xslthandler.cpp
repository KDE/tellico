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

#include "xslthandler.h"

#include <kdebug.h>

extern "C" {
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>

#include <libexslt/exslt.h>
}

#if LIBXML_VERSION >= 20600
// I don't want any network I/O at all
static int xml_options = XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOCDATA;
static int xslt_options = xml_options;
#endif

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

using Bookcase::XSLTHandler;

int XSLTHandler::s_initCount = 0;

XSLTHandler::XSLTHandler(const QCString& xsltFile_) :
    m_stylesheet(0),
    m_docIn(0),
    m_docOut(0),
    m_numParams(0) {
  init();
  if(!xsltFile_.isNull()) {
#if LIBXML_VERSION >= 20600
    xmlDocPtr xsltDoc = xmlReadFile(xsltFile_, NULL, xslt_options);
#else
    xmlDocPtr xsltDoc = xmlParseFile(xsltFile_);
#endif
    m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
    if(!m_stylesheet) {
      kdDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer for " << xsltFile_ << endl;
    }
  }
}

XSLTHandler::XSLTHandler(const KURL& xsltURL_) :
    m_stylesheet(0),
    m_docIn(0),
    m_docOut(0),
    m_numParams(0) {
  init();
  if(xsltURL_.isValid() && xsltURL_.isLocalFile()) {
#if LIBXML_VERSION >= 20600
    xmlDocPtr xsltDoc = xmlReadFile(xsltURL_.path().utf8(), NULL, xslt_options);
#else
    xmlDocPtr xsltDoc = xmlParseFile(xsltURL_.path().utf8());
#endif
    m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
    if(!m_stylesheet) {
      kdDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer for " << xsltURL_.path() << endl;
    }
  }
}

XSLTHandler::XSLTHandler(const QDomDocument& xsltDoc_, const QCString& xsltFile_) :
    m_stylesheet(0),
    m_docIn(0),
    m_docOut(0),
    m_numParams(0) {
  init();
  if(!xsltDoc_.isNull()) {
    setXSLTDoc(xsltDoc_, xsltFile_);
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

  --s_initCount;
  if(s_initCount == 0) {
    xsltUnregisterExtModule(EXSLT_STRINGS_NAMESPACE);
    xsltUnregisterExtModule(EXSLT_DYNAMIC_NAMESPACE);
    xsltCleanupGlobals();
    xmlCleanupParser();
  }

  for(int i = 0; i < m_numParams; ++i) {
    delete[] m_params[i];
  }
}

void XSLTHandler::init() {
  if(s_initCount == 0) {
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 0;

    // register all exslt extensions
    exsltRegisterAll();
  }
  ++s_initCount;

  m_params[0] = NULL;
}

void XSLTHandler::setXSLTDoc(const QDomDocument& dom_, const QCString& xsltFile_) {
  bool utf8 = true; // XML defaults to utf-8

  // need to find out if utf-8 or not
  const QDomNodeList childs = dom_.childNodes();
  for(unsigned j = 0; j < childs.count(); ++j) {
    if(childs.item(j).isProcessingInstruction()) {
      QDomProcessingInstruction pi = childs.item(j).toProcessingInstruction();
      if(pi.data().lower().contains(QString::fromLatin1("encoding"))) {
        if(!pi.data().lower().contains(QString::fromLatin1("utf-8"))) {
          utf8 = false;
//        } else {
//          kdDebug() << "XSLTHandler::setXSLTDoc() - PI = " << pi.data() << endl;
        }
        break;
      }
    }
  }

  xmlDocPtr xsltDoc;
  if(utf8) {
#if LIBXML_VERSION >= 20600
    xsltDoc = xmlReadDoc((xmlChar *)dom_.toString().utf8().data(), xsltFile_.data(), NULL, xslt_options);
#else
    xsltDoc = xmlParseDoc((xmlChar *)dom_.toString().utf8().data());
    xsltDoc->URL = (xmlChar *)qstrdup(xsltFile_.data()); // needed in case of xslt includes
#endif
  } else {
#if LIBXML_VERSION >= 20600
    xsltDoc = xmlReadDoc((xmlChar *)dom_.toString().local8Bit().data(), xsltFile_.data(), NULL, xslt_options);
#else
    xsltDoc = xmlParseDoc((xmlChar *)dom_.toString().local8Bit().data());
    xsltDoc->URL = (xmlChar *)qstrdup(xsltFile_.data()); // needed in case of xslt includes
#endif
  }

  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
  }
  m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
  if(!m_stylesheet) {
    kdDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer for " << xsltFile_ << endl;
  }
//  xmlFreeDoc(xsltDoc); // this causes a crash for some reason
}

void XSLTHandler::addParam(const QCString& name_, const QCString& value_) {
  if(m_numParams < MAX_PARAMS) {
    m_params[m_numParams]     = qstrdup(name_);
    m_params[m_numParams + 1] = qstrdup(value_);
    m_params[m_numParams + 2] = 0;
    m_numParams += 2;
//    kdDebug() << "XSLTHandler::addParam() - " << name_ << ":" << value_ << endl;
  } else {
    kdWarning() << "XSLTHandler::addParam() - too many params to add " << name_ << ":" << value_ << endl;
  }
}

void XSLTHandler::addStringParam(const QCString& name_, const QCString& value_) {
  QCString value = value_;
//  value.replace('&', "&amp;");
//  value.replace('<', "&lt;");
//  value.replace('>', "&gt;");
//  value.replace('"', "&quot;");
  value.replace('\'', "&apos;");
  addParam(name_, QCString("'") + value + QCString("'"));
}

QString XSLTHandler::applyStylesheet(const QCString& text_) {
  if(!m_stylesheet) {
    kdDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer!" << endl;
    return QString::null;
  }

#if LIBXML_VERSION >= 20600
  m_docIn = xmlReadDoc((xmlChar *)text_.data(), NULL, NULL, xml_options);
#else
  m_docIn = xmlParseDoc((xmlChar *)text_.data());
#endif

  return process();
}

QString XSLTHandler::applyStylesheet(const QString& text_, bool encodedUTF8_) {
  if(!m_stylesheet) {
    kdDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer!" << endl;
    return QString::null;
  }

  if(encodedUTF8_) {
#if LIBXML_VERSION >= 20600
    m_docIn = xmlReadDoc((xmlChar *)text_.utf8().data(), NULL, NULL, xml_options);
#else
    m_docIn = xmlParseDoc((xmlChar *)text_.utf8().data());
#endif
  } else {
#if LIBXML_VERSION >= 20600
    m_docIn = xmlReadDoc((xmlChar *)text_.local8Bit().data(), NULL, NULL, xml_options);
#else
    m_docIn = xmlParseDoc((xmlChar *)text_.local8Bit().data());
#endif
  }

  return process();
}

QString XSLTHandler::process() {
  if(!m_docIn) {
    kdDebug() << "XSLTHandler::process() - error parsing input string!" << endl;
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

  xmlOutputBufferClose(outp); //also flushes

//  kdDebug() << "RESULT: " << result << endl;
  return result;
}
