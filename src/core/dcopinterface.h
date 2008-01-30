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

class ApplicationInterface : public DCOPObject {
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
  bool exportZip(const QString& file)
    { return exportCollection(Export::TellicoZip, KURL::fromPathOrURL(file)); }
  bool exportBibtex(const QString& file)
    { return exportCollection(Export::Bibtex, KURL::fromPathOrURL(file)); }
  bool exportHTML(const QString& file)
    { return exportCollection(Export::HTML, KURL::fromPathOrURL(file)); }
  bool exportCSV(const QString& file)
    { return exportCollection(Export::CSV, KURL::fromPathOrURL(file)); }
  bool exportPilotDB(const QString& file)
    { return exportCollection(Export::PilotDB, KURL::fromPathOrURL(file)); }

  QValueList<long> selectedEntries() const;
  QValueList<long> filteredEntries() const;

  virtual void openFile(const QString& file) = 0;
  virtual void setFilter(const QString& text) = 0;
  virtual bool showEntry(long id) = 0;

protected:
  ApplicationInterface() : DCOPObject("tellico") {}
  virtual bool importFile(Import::Format format, const KURL& url, Import::Action action) = 0;
  virtual bool exportCollection(Export::Format format, const KURL& url) = 0;

private:
  Import::Action actionType(const QString& actionName);
};

class CollectionInterface : public DCOPObject {
K_DCOP
k_dcop:
  CollectionInterface() : DCOPObject("collection") {}

  long addEntry();
  bool removeEntry(long entryID);

  QStringList values(const QString& fieldName) const;
  QStringList values(long entryID, const QString& fieldName) const;
  QStringList bibtexKeys() const;
  QString bibtexKey(long entryID) const;

  bool setFieldValue(long entryID, const QString& fieldName, const QString& value);
  bool addFieldValue(long entryID, const QString& fieldName, const QString& value);
};

} // end namespace
#endif
