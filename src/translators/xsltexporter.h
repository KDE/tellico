/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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

#include "exporter.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class XSLTExporter : public Exporter {
public:
  XSLTExporter();

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget* parent, const char* name=0);

  virtual void readOptions(KConfig* cfg);
  virtual void saveOptions(KConfig* cfg);

private:
  QWidget* m_widget;
  KURLRequester* m_URLRequester;
  QString m_xsltFile;
};

  } // end namespace
} // end namespace
#endif
