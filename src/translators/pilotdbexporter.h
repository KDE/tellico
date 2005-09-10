/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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

#include "exporter.h"

#include <qstringlist.h>

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class PilotDBExporter : public Exporter {
public:
  PilotDBExporter();

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget* parent, const char* name=0);
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
