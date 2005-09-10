/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef ONIXEXPORTER_H
#define ONIXEXPORTER_H

class QCheckBox;

#include "exporter.h"

namespace Tellico {
  namespace Data {
    class Collection;
  }
  class XSLTHandler;
  namespace Export {

/**
 * @author Robby Stephenson
 */
class ONIXExporter : public Exporter {
public:
  ONIXExporter();
  ONIXExporter(const Data::Collection* coll);
  ~ONIXExporter();

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget*, const char* name=0);
  virtual void readOptions(KConfig*);
  virtual void saveOptions(KConfig*);

  QString text();

private:
  XSLTHandler* m_handler;
  QString m_xsltFile;
  bool m_includeImages;

  QWidget* m_widget;
  QCheckBox* m_checkIncludeImages;
};

  } // end namespace
} // end namespace
#endif
