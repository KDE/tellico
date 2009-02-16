/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
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
#include "../tellico_debug.h"
#include "../tellico_utils.h"

#include <kurl.h>

#include <QDomDocument>
#include <QTextCodec>
#include <QVector>

extern "C" {
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>

#include <libexslt/exslt.h>
}

// I don't want any network I/O at all
static const int xml_options = XML_PARSE_NOENT | XML_PARSE_NONET | XML_PARSE_NOCDATA;
static const int xslt_options = xml_options;

/* some functions to pass to the XSLT libs */
static int writeToQString(void* context, const char* buffer, int len) {
  QString* t = static_cast<QString*>(context);
  *t += QString::fromUtf8(buffer, len);
  return len;
}

static void closeQString(void* context) {
  QString* t = static_cast<QString*>(context);
  *t += QLatin1String("\n");
}

using Tellico::XSLTHandler;

XSLTHandler::XMLOutputBuffer::XMLOutputBuffer() : m_res(QString::null) {
  m_buf = xmlOutputBufferCreateIO((xmlOutputWriteCallback)writeToQString,
                                  (xmlOutputCloseCallback)closeQString,
                                  &m_res, 0);
  if(m_buf) {
    m_buf->written = 0;
  } else {
    myWarning() << "XMLOutputBuffer::XMLOutputBuffer() - error writing output buffer!" << endl;
  }
}

XSLTHandler::XMLOutputBuffer::~XMLOutputBuffer() {
  if(m_buf) {
    xmlOutputBufferClose(m_buf); //also flushes
    m_buf = 0;
  }
}

int XSLTHandler::s_initCount = 0;

XSLTHandler::XSLTHandler(const QByteArray& xsltFile_) :
    m_stylesheet(0),
    m_docIn(0),
    m_docOut(0) {
  init();
  QByteArray file = QUrl::toPercentEncoding(QString::fromLocal8Bit(xsltFile_));
  if(!file.isEmpty()) {
    xmlDocPtr xsltDoc = xmlReadFile(file, NULL, xslt_options);
    m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
    if(!m_stylesheet) {
      myDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer for " << xsltFile_ << endl;
    }
  } else {
    myDebug() << "XSLTHandler(QByteArray) - empty file name" << endl;
  }
}

XSLTHandler::XSLTHandler(const KUrl& xsltURL_) :
    m_stylesheet(0),
    m_docIn(0),
    m_docOut(0) {
  init();
  if(xsltURL_.isValid() && xsltURL_.isLocalFile()) {
    xmlDocPtr xsltDoc = xmlReadFile(xsltURL_.encodedPathAndQuery().toUtf8(), NULL, xslt_options);
    m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
    if(!m_stylesheet) {
      myDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer for " << xsltURL_.path() << endl;
    }
  } else {
    myDebug() << "XSLTHandler(KUrl) - invalid: " << xsltURL_ << endl;
  }
}

XSLTHandler::XSLTHandler(const QDomDocument& xsltDoc_, const QByteArray& xsltFile_, bool translate_) :
    m_stylesheet(0),
    m_docIn(0),
    m_docOut(0) {
  init();
  QByteArray file = QUrl::toPercentEncoding(QString::fromLocal8Bit(xsltFile_));
  if(!xsltDoc_.isNull() && !file.isEmpty()) {
    setXSLTDoc(xsltDoc_, file, translate_);
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
}

void XSLTHandler::init() {
  if(s_initCount == 0) {
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 0;

    // register all exslt extensions
    exsltRegisterAll();
  }
  ++s_initCount;

  m_params.clear();
}

void XSLTHandler::setXSLTDoc(const QDomDocument& dom_, const QByteArray& xsltFile_, bool translate_) {
  bool utf8 = true; // XML defaults to utf-8

  // need to find out if utf-8 or not
  const QDomNodeList childs = dom_.childNodes();
  for(int j = 0; j < childs.count(); ++j) {
    if(childs.item(j).isProcessingInstruction()) {
      QDomProcessingInstruction pi = childs.item(j).toProcessingInstruction();
      if(pi.data().toLower().contains(QLatin1String("encoding"))) {
        if(!pi.data().toLower().contains(QLatin1String("utf-8"))) {
          utf8 = false;
//        } else {
//          myDebug() << "XSLTHandler::setXSLTDoc() - PI = " << pi.data() << endl;
        }
        break;
      }
    }
  }

  QString s;
  if(translate_) {
    s = Tellico::i18nReplace(dom_.toString(0 /* indent */));
  } else {
    s = dom_.toString();
  }

  xmlDocPtr xsltDoc;
  if(utf8) {
    xsltDoc = xmlReadDoc(reinterpret_cast<xmlChar*>(s.toUtf8().data()), xsltFile_.data(), NULL, xslt_options);
  } else {
    xsltDoc = xmlReadDoc(reinterpret_cast<xmlChar*>(s.toLocal8Bit().data()), xsltFile_.data(), NULL, xslt_options);
  }

  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
  }
  m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
  if(!m_stylesheet) {
    myDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer for " << xsltFile_ << endl;
  }
//  xmlFreeDoc(xsltDoc); // this causes a crash for some reason
}

void XSLTHandler::addStringParam(const QByteArray& name_, const QByteArray& value_) {
  QByteArray value = value_;
  value.replace('\'', "&apos;");
  addParam(name_, QByteArray("'") + value + QByteArray("'"));
}

void XSLTHandler::addParam(const QByteArray& name_, const QByteArray& value_) {
  m_params.insert(name_, value_);
//  myDebug() << "XSLTHandler::addParam() - " << name_ << ":" << value_ << endl;
}

void XSLTHandler::removeParam(const QByteArray& name_) {
  m_params.remove(name_);
}

const QByteArray& XSLTHandler::param(const QByteArray& name_) {
  return m_params[name_];
}

QString XSLTHandler::applyStylesheet(const QString& text_) {
  if(!m_stylesheet) {
    myDebug() << "XSLTHandler::applyStylesheet() - null stylesheet pointer!" << endl;
    return QString::null;
  }

  m_docIn = xmlReadDoc(reinterpret_cast<xmlChar*>(text_.toUtf8().data()), NULL, NULL, xml_options);

  return process();
}

QString XSLTHandler::process() {
  if(!m_docIn) {
    myDebug() << "XSLTHandler::process() - error parsing input string!" << endl;
    return QString::null;
  }

  QVector<const char*> params(2*m_params.count() + 1);
  params[0] = NULL;
  QHash<QByteArray, QByteArray>::ConstIterator it = m_params.constBegin();
  QHash<QByteArray, QByteArray>::ConstIterator end = m_params.constEnd();
  for(int i = 0; it != end; ++it) {
    params[i  ] = qstrdup(it.key());
    params[i+1] = qstrdup(it.value());
    params[i+2] = NULL;
    i += 2;
  }
  // returns NULL on error
  m_docOut = xsltApplyStylesheet(m_stylesheet, m_docIn, params.data());
  for(int i = 0; i < 2*m_params.count(); ++i) {
    delete[] params[i];
  }
  if(!m_docOut) {
    myDebug() << "XSLTHandler::applyStylesheet() - error applying stylesheet!" << endl;
    return QString::null;
  }

  XMLOutputBuffer output;
  if(output.isValid()) {
    int num_bytes = xsltSaveResultTo(output.buffer(), m_docOut, m_stylesheet);
    if(num_bytes == -1) {
      myDebug() << "XSLTHandler::applyStylesheet() - error saving output buffer!" << endl;
    }
  }
  return output.result();
}

//static
QDomDocument& XSLTHandler::setLocaleEncoding(QDomDocument& dom_) {
  const QDomNodeList childs = dom_.documentElement().childNodes();
  for(int j = 0; j < childs.count(); ++j) {
    if(childs.item(j).isElement() && childs.item(j).nodeName() == QLatin1String("xsl:output")) {
      QDomElement e = childs.item(j).toElement();
      const QString encoding = QLatin1String(QTextCodec::codecForLocale()->name());
      e.setAttribute(QLatin1String("encoding"), encoding);
      break;
    }
  }
  return dom_;
}
