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

#ifndef TELLICO_BIBTEXMLEXPORTER_H
#define TELLICO_BIBTEXMLEXPORTER_H

#include "exporter.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class BibtexmlExporter : public Exporter {
Q_OBJECT

public:
  BibtexmlExporter() : Exporter() {}

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  // no options
  virtual QWidget* widget(QWidget*) { return 0; }
};

  } // end namespace
} // end namespace
#endif
