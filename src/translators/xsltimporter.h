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

#ifndef TELLICO_XSLTIMPORTER_H
#define TELLICO_XSLTIMPORTER_H

class KUrlRequester;

#include "textimporter.h"
#include "../datavectors.h"

namespace Tellico {
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
  XSLTImporter(const KUrl& url);

  /**
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent);
  void setXSLTURL(const KUrl& url) { m_xsltURL = url; }

private:
  Data::CollPtr m_coll;

  QWidget* m_widget;
  KUrlRequester* m_URLRequester;
  KUrl m_xsltURL;
};

  } // end namespace
} // end namespace
#endif
