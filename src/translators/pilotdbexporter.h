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

#ifndef TELLICO_PILOTDBEXPORTER_H
#define TELLICO_PILOTDBEXPORTER_H

class QCheckBox;

#include "exporter.h"

#include <QStringList>

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class PilotDBExporter : public Exporter {
Q_OBJECT

public:
  PilotDBExporter();

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget* parent);
  virtual void readOptions(KSharedConfigPtr cfg);
  virtual void saveOptions(KSharedConfigPtr cfg);

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
