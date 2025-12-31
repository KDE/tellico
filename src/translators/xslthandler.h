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

#ifndef TELLICO_XSLTHANDLER_H
#define TELLICO_XSLTHANDLER_H

#include <QByteArray>
#include <QHash>
#include <QString>

// for xmlDocPtr
#include <libxml/tree.h>

extern "C" {
// for xsltStyleSheetPtr
#include <libxslt/xsltInternals.h>
}

class QUrl;
class QDomDocument;

namespace Tellico {

/**
 * The XSLTHandler contains all the code which uses XSLT processing to generate HTML or to
 * translate to other formats.
 *
 * @author Robby Stephenson
 */
class XSLTHandler {

public:
  class XMLOutputBuffer {
  public:
    XMLOutputBuffer();
    ~XMLOutputBuffer();
    bool isValid() const { return (m_buf != nullptr); }
    xmlOutputBuffer* buffer() const { return m_buf; }
    QString result() const { return m_res; }
  private:
    Q_DISABLE_COPY(XMLOutputBuffer)
    xmlOutputBuffer* m_buf;
    QString m_res;
  };

  /**
   * @param xsltFile The XSLT file
   */
  XSLTHandler(const QByteArray& xsltFile);
  /**
   * @param xsltURL The XSLT URL
   */
  XSLTHandler(const QUrl& xsltURL);
  /**
   * @param xsltDoc The XSLT DOM document
   * @param xsltFile The XSLT file, should be a url?
   */
  XSLTHandler(const QDomDocument& xsltDoc, const QByteArray& xsltFile, bool translate=false);
  /**
   */
  ~XSLTHandler();

  bool isValid() const;

  /**
   * Set the XSLT text
   *
   * @param dom The XSLT DOM document
   * @param xsltFile The XSLT file, should be a url?
   */
  void setXSLTDoc(const QDomDocument& dom, const QByteArray& xsltFile, bool translate=false);
  /**
   * Adds a param
   */
  void addParam(const QByteArray& name, const QByteArray& value);
  /**
   * Adds a string param
   */
  void addStringParam(const QByteArray& name, const QByteArray& value);
  void removeParam(const QByteArray& name);
  const QByteArray& param(const QByteArray& name);
  /**
   * Processes text through the XSLT transformation.
   *
   * @param text The text to be transformed
   * @return The transformed text
   */
  QString applyStylesheet(const QString& text);

  static QDomDocument& setLocaleEncoding(QDomDocument& dom);

private:
  Q_DISABLE_COPY(XSLTHandler)

  void init();
  QString process(xmlDocPtr docIn);

  xsltStylesheetPtr m_stylesheet;

  QHash<QByteArray, QByteArray> m_params;

  static int s_initCount;
};

} // end namespace
#endif
