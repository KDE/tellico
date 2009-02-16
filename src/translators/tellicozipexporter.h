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

#ifndef TELLICO_ZIPEXPORTER_H
#define TELLICO_ZIPEXPORTER_H

#include "exporter.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class TellicoZipExporter : public Exporter {
Q_OBJECT

public:
  TellicoZipExporter() : Exporter(), m_includeImages(true), m_cancelled(false) {}

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  // no options
  virtual QWidget* widget(QWidget*) { return 0; }

  void setIncludeImages(bool b) { m_includeImages = b; }

public slots:
  void slotCancel();

private:
  bool m_includeImages : 1;
  bool m_cancelled : 1;
};

  } // end namespace
} // end namespace
#endif
