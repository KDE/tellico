/***************************************************************************
    copyright            : (C) 2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef AUDIOFILEIMPORTER_H
#define AUDIOFILEIMPORTER_H

class QCheckBox;

#include "importer.h"

namespace Tellico {
  namespace Import {

/**
 * The AudioFileImporter class takes care of importing audio files.
 *
 * @author Robby Stephenson
 * @version $Id: audiofileimporter.h 862 2004-09-15 01:49:51Z robby $
 */
class AudioFileImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  AudioFileImporter(const KURL& url);

  /**
   */
  virtual Data::Collection* collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual bool canImport(Data::Collection::Type type) { return (type == Data::Collection::Album); }

private:
  static QString insertValue(const QString& str, const QString& value, uint pos);
  static QStringList findAllSubDirs(const QString& dir);

  Data::Collection* m_coll;
  QWidget* m_widget;
  QCheckBox* m_recursive;
};

  } // end namespace
} // end namespace
#endif
