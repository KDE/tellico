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

#ifndef TELLICO_IMPORT_AMCIMPORTER_H
#define TELLICO_IMPORT_AMCIMPORTER_H

#include "dataimporter.h"

namespace Tellico {
  namespace Import {

/**
 @author Robby Stephenson
 */
class AMCImporter : public DataImporter {
Q_OBJECT
public:
  AMCImporter(const KUrl& url);
  virtual ~AMCImporter();

  virtual Data::CollPtr collection();
  bool canImport(int type) const;

public slots:
  void slotCancel();

private:
  bool readBool();
  quint32 readInt();
  QString readString();
  QString readImage(const QString& format);
  void readEntry();
  QStringList parseCast(const QString& text);

  Data::CollPtr m_coll;
  bool m_cancelled : 1;
  QDataStream m_ds;
  int m_majVersion;
  int m_minVersion;
};

  } // end namespace
} // end namespace

#endif
