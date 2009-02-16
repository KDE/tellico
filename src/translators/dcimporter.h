/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMPORT_DCIMPORTER_H
#define TELLICO_IMPORT_DCIMPORTER_H

#include "xmlimporter.h"

namespace Tellico {
  namespace Import {

class DCImporter : public XMLImporter {
public:
  DCImporter(const KUrl& url);
  DCImporter(const QString& text);
  DCImporter(const QDomDocument& dom);
  ~DCImporter() {}

  virtual Data::CollPtr collection();
};

  }
}
#endif
