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

#ifndef ALEXANDRIAIMPORTER_H
#define ALEXANDRIAIMPORTER_H

class KComboBox;

#include "importer.h"

#include <qdir.h>

namespace Bookcase {
  namespace Import {

/**
 * An importer for importing collections used by ALexandria, the Gnome book collection manager.
 *
 * The libraries are assumed to be in $HOME/.alexandria. The file format is YAML, but instead
 * using a real YAML reader, the file is parsed line-by-line, so it's very crude. When Alexandria
 * adds new fields or types, this will have to be updated.
 *
 * @author Robby Stephenson
 * @version $Id: alexandriaimporter.h 821 2004-08-27 23:26:04Z robby $
 */
class AlexandriaImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  AlexandriaImporter() : Importer(), m_coll(0), m_widget(0) {}
  /**
   */
  virtual ~AlexandriaImporter() {}

  /**
   */
  virtual Data::Collection* collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual bool canImport(Data::Collection::Type type) { return (type == Data::Collection::Book); }

private:
  Data::Collection* m_coll;
  QWidget* m_widget;
  KComboBox* m_library;

  QDir m_libraryDir;
};

  } // end namespace
} // end namespace
#endif
