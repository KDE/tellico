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

#ifndef PILOTDBEXPORTER_H
#define PILOTDBEXPORTER_H

class QCheckBox;

#include "dataexporter.h"

#include <qstringlist.h>

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: pilotdbexporter.h 862 2004-09-15 01:49:51Z robby $
 */
class PilotDBExporter : public DataExporter {
public:
  PilotDBExporter(const Data::Collection* coll);

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual QString formatString() const;
  virtual QByteArray data(bool formatFields);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig* cfg);
  virtual void saveOptions(KConfig* cfg);

  void setColumns(const QStringList& columns) { m_columns = columns; }

private:
  bool m_backup;

  QWidget* m_widget;
  QCheckBox* m_checkBackup;
  QStringList m_columns;
};

  } // end namespace
} // end namespace
#endif
