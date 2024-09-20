/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#include "xslthandler.h"
#include "../tellico_debug.h"
#include "../utils/string_utils.h"

#include <QUrl>

#include <QDomDocument>
#include <QVector>

extern "C" {
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>

#include <libexslt/exslt.h>
}

// I don't want any network I/O at all
static const int xml_options = XML_PARSE_NONET | XML_PARSE_NOCDATA;
static const int xslt_options = xml_options;

/* some functions to pass to the XSLT libs */
static int writeToQString(void* context, const char* buffer, int len) {
  QString* t = static_cast<QString*>(context);
  *t += QString::fromUtf8(buffer, len);
  return len;
}

static int closeQString(void* context) {
  QString* t = static_cast<QString*>(context);
  *t += QLatin1String("\n");
  return 0;
}

using Tellico::XSLTHandler;

XSLTHandler::XMLOutputBuffer::XMLOutputBuffer() {
  m_buf = xmlOutputBufferCreateIO((xmlOutputWriteCallback)writeToQString,
                                  (xmlOutputCloseCallback)closeQString,
                                  &m_res, nullptr);
  if(m_buf) {
    m_buf->written = 0;
  } else {
    myWarning() << "error writing output buffer!";
  }
}

XSLTHandler::XMLOutputBuffer::~XMLOutputBuffer() {
  if(m_buf) {
    xmlOutputBufferClose(m_buf); //also flushes
    m_buf = nullptr;
  }
}

int XSLTHandler::s_initCount = 0;

XSLTHandler::XSLTHandler(const QByteArray& xsltFile_) :
    m_stylesheet(nullptr) {
  init();
  if(!xsltFile_.isEmpty()) {
    xmlDocPtr xsltDoc = xmlReadFile(xsltFile_.constData(), nullptr, xslt_options);
    m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
    if(!m_stylesheet) {
      myDebug() << "null stylesheet pointer for " << xsltFile_;
    }
  } else {
    myDebug() << "XSLTHandler(QByteArray) - empty file name";
  }
}

XSLTHandler::XSLTHandler(const QUrl& xsltURL_) :
    m_stylesheet(nullptr) {
  init();
  if(xsltURL_.isValid() && xsltURL_.isLocalFile()) {
    xmlDocPtr xsltDoc = xmlReadFile(xsltURL_.toLocalFile().toUtf8().constData(), nullptr, xslt_options);
    m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
    if(!m_stylesheet) {
      myDebug() << "null stylesheet pointer for " << xsltURL_.path();
    }
  } else {
    myDebug() << "XSLTHandler(QUrl) - invalid: " << xsltURL_;
  }
}

XSLTHandler::XSLTHandler(const QDomDocument& xsltDoc_, const QByteArray& xsltFile_, bool translate_) :
    m_stylesheet(nullptr) {
  init();
  if(!xsltDoc_.isNull() && !xsltFile_.isEmpty()) {
    setXSLTDoc(xsltDoc_, xsltFile_, translate_);
  }
}

XSLTHandler::~XSLTHandler() {
  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
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
    xmlLoadExtDtdDefaultValue = 0;

    // register all exslt extensions
    exsltRegisterAll();
  }
  ++s_initCount;

  m_params.clear();
}

bool XSLTHandler::isValid() const {
  return (m_stylesheet != nullptr);
}

void XSLTHandler::setXSLTDoc(const QDomDocument& dom_, const QByteArray& xsltFile_, bool translate_) {
  bool utf8 = true; // XML defaults to utf-8

  // need to find out if utf-8 or not
  const QDomNodeList children = dom_.childNodes();
  for(int j = 0; j < children.count(); ++j) {
    if(children.item(j).isProcessingInstruction()) {
      QDomProcessingInstruction pi = children.item(j).toProcessingInstruction();
      if(pi.data().toLower().contains(QLatin1String("encoding"))) {
        if(!pi.data().toLower().contains(QLatin1String("utf-8"))) {
          utf8 = false;
//        } else {
//          myDebug() << "PI = " << pi.data();
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
    xsltDoc = xmlReadDoc(reinterpret_cast<xmlChar*>(s.toUtf8().data()), xsltFile_.data(), nullptr, xslt_options);
  } else {
    xsltDoc = xmlReadDoc(reinterpret_cast<xmlChar*>(s.toLocal8Bit().data()), xsltFile_.data(), nullptr, xslt_options);
  }

  if(m_stylesheet) {
    xsltFreeStylesheet(m_stylesheet);
  }
  m_stylesheet = xsltParseStylesheetDoc(xsltDoc);
  if(!m_stylesheet) {
    myDebug() << "null stylesheet pointer for " << xsltFile_;
  }
//  xmlFreeDoc(xsltDoc); // this causes a crash for some reason
}

void XSLTHandler::addStringParam(const QByteArray& name_, const QByteArray& value_) {
  QByteArray value = value_;
  if(value.contains('\'')) {
    if(value.contains('"')) {
      myWarning() << "String param contains both ' and \"" << value_;
      value.replace('"', "'");
    }
    addParam(name_, QByteArray("\"") + value + QByteArray("\""));
  } else {
    addParam(name_, QByteArray("'") + value + QByteArray("'"));
  }
}

void XSLTHandler::addParam(const QByteArray& name_, const QByteArray& value_) {
  m_params.insert(name_, value_);
//  myDebug() << name_ << ":" << value_;
}

void XSLTHandler::removeParam(const QByteArray& name_) {
  m_params.remove(name_);
}

const QByteArray& XSLTHandler::param(const QByteArray& name_) {
  return m_params[name_];
}

QString XSLTHandler::applyStylesheet(const QString& text_) {
  if(!m_stylesheet) {
    myDebug() << "null stylesheet pointer!";
    return QString();
  }
  if(text_.isEmpty()) {
    myDebug() << "XSLTHandler::applyStylesheet() - empty input";
    return QString();
  }

  xmlDocPtr docIn;
  docIn = xmlReadDoc(reinterpret_cast<xmlChar*>(text_.toUtf8().data()), nullptr, nullptr, xml_options);

  return process(docIn);
}

QString XSLTHandler::process(xmlDocPtr docIn) {
  if(!docIn) {
    myDebug() << "XSLTHandler::applyStylesheet() - error parsing input string!";
    return QString();
  }

  QVector<const char*> params(2*m_params.count() + 1);
  params[0] = nullptr;
  QHash<QByteArray, QByteArray>::ConstIterator it = m_params.constBegin();
  QHash<QByteArray, QByteArray>::ConstIterator end = m_params.constEnd();
  for(int i = 0; it != end; ++it) {
    params[i  ] = qstrdup(it.key().constData());
    params[i+1] = qstrdup(it.value().constData());
    params[i+2] = nullptr;
    i += 2;
  }
  // returns NULL on error
  xmlDocPtr docOut;
  docOut = xsltApplyStylesheet(m_stylesheet, docIn, params.data());
  for(int i = 0; i < 2*m_params.count(); ++i) {
    delete[] params[i];
  }

  xmlFreeDoc(docIn);
  docIn = nullptr;

  if(!docOut) {
    myDebug() << "error applying stylesheet!";
    return QString();
  }

  XMLOutputBuffer output;
  if(output.isValid()) {
    int num_bytes = xsltSaveResultTo(output.buffer(), docOut, m_stylesheet);
    if(num_bytes == -1) {
      myDebug() << "error saving output buffer!";
    }
  }

  xmlFreeDoc(docOut);
  docOut = nullptr;

  return output.result();
}

//static
QDomDocument& XSLTHandler::setLocaleEncoding(QDomDocument& dom_) {
  const QDomNodeList children = dom_.documentElement().childNodes();
  for(int j = 0; j < children.count(); ++j) {
    if(children.item(j).isElement() && children.item(j).nodeName() == QLatin1String("xsl:output")) {
      QDomElement e = children.item(j).toElement();
      e.setAttribute(QStringLiteral("encoding"), QLatin1String(Tellico::localeEncodingName()));
      break;
    }
  }
  return dom_;
}
