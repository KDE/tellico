/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMPORT_PDFIMPORTER_H
#define TELLICO_IMPORT_PDFIMPORTER_H

#include "importer.h"

namespace Tellico {
  namespace Import {

class PDFImporter : public Importer {
Q_OBJECT

public:
  PDFImporter(const KUrl::List& urls);

  virtual bool canImport(int type) const;

  virtual Data::CollPtr collection();

public slots:
  void slotCancel();

private:
  bool m_cancelled;
};

  }
}
#endif
