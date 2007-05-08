/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DCOPINTERFACE_H
#define TELLICO_DCOPINTERFACE_H

#include "../translators/translators.h"

#include <dcopobject.h>
#include <kurl.h>

#include <qstringlist.h> // used in generated dcopinterface_skel.cpp

namespace Tellico {

class DCOPInterface : public DCOPObject {
K_DCOP
k_dcop:
  bool importTellico(const QString& file, const QString& action)
    { return importFile(Import::TellicoXML, KURL::fromPathOrURL(file), actionType(action)); }
  bool importBibtex(const QString& file, const QString& action)
    { return importFile(Import::Bibtex, KURL::fromPathOrURL(file), actionType(action)); }
  bool importMODS(const QString& file, const QString& action)
    { return importFile(Import::MODS, KURL::fromPathOrURL(file), actionType(action)); }
  bool importRIS(const QString& file, const QString& action)
    { return importFile(Import::RIS, KURL::fromPathOrURL(file), actionType(action)); }

  bool exportXML(const QString& file)
    { return exportCollection(Export::TellicoXML, KURL::fromPathOrURL(file)); }
  bool exportBibtex(const QString& file)
    { return exportCollection(Export::Bibtex, KURL::fromPathOrURL(file)); }
  bool exportHTML(const QString& file)
    { return exportCollection(Export::HTML, KURL::fromPathOrURL(file)); }
  bool exportCSV(const QString& file)
    { return exportCollection(Export::CSV, KURL::fromPathOrURL(file)); }
  bool exportPilotDB(const QString& file)
    { return exportCollection(Export::PilotDB, KURL::fromPathOrURL(file)); }

  virtual QStringList bibtexKeys() const = 0;

protected:
  DCOPInterface() : DCOPObject("tellico") {}
  virtual bool importFile(Import::Format format, const KURL& url, Import::Action action) = 0;
  virtual bool exportCollection(Export::Format format, const KURL& url) = 0;

private:
  Import::Action actionType(const QString& actionName);
};

} // end namespace
#endif
