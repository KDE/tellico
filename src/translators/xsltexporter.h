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

#ifndef TELLICO_XSLTEXPORTER_H
#define TELLICO_XSLTEXPORTER_H

class KUrlRequester;

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

  virtual QWidget* widget(QWidget* parent);

  virtual void readOptions(KSharedConfigPtr config);
  virtual void saveOptions(KSharedConfigPtr config);

private:
  QWidget* m_widget;
  KUrlRequester* m_URLRequester;
  KUrl m_xsltFile;
};

  } // end namespace
} // end namespace
#endif
