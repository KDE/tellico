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

#ifndef TELLICO_XSLTIMPORTER_H
#define TELLICO_XSLTIMPORTER_H

class KUrlRequester;

#include "textimporter.h"
#include "../datavectors.h"

namespace Tellico {
  class XSLTHandler;

  namespace Import {

/**
 * The XSLTImporter class takes care of transforming XML data using an XSL stylesheet.
 *
 * @author Robby Stephenson
 */
class XSLTImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  XSLTImporter(const QUrl& url);
  XSLTImporter(const QString& text);

  /**
   */
  virtual Data::CollPtr collection() override;
  /**
   * The XSLTImporter can import any type known to Tellico.
   */
  virtual bool canImport(int type) const override { Q_UNUSED(type); return true; }
  /**
   */
  virtual QWidget* widget(QWidget* parent) override;
  virtual void beginXSLTHandler(XSLTHandler*) {}
  void setXSLTURL(const QUrl& url) { m_xsltURL = url; }

public Q_SLOTS:
  void slotCancel() override;

private:
  Data::CollPtr m_coll;

  QWidget* m_widget;
  KUrlRequester* m_URLRequester;
  QUrl m_xsltURL;
};

  } // end namespace
} // end namespace
#endif
