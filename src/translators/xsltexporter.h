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

#ifndef XSLTEXPORTER_H
#define XSLTEXPORTER_H

class KURLRequester;

#include "textexporter.h"

namespace Bookcase {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: xsltexporter.h 759 2004-08-11 01:28:25Z robby $
 */
class XSLTExporter : public TextExporter {
public:
  XSLTExporter(const Data::Collection* coll);

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*) {}
  virtual void saveOptions(KConfig*) {}

private:
  QWidget* m_widget;
  KURLRequester* m_URLRequester;
};

  } // end namespace
} // end namespace
#endif
