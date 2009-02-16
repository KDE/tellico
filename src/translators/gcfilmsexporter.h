/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_EXPORT_GCFILMSEXPORTER_H
#define TELLICO_EXPORT_GCFILMSEXPORTER_H

#include "exporter.h"

class QTextStream;

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class GCfilmsExporter : public Exporter {
Q_OBJECT

public:
  GCfilmsExporter();

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  // no options
  virtual QWidget* widget(QWidget*) { return 0; }

private:
  void push(QTextStream& ts, const QByteArray& fieldName, Data::EntryPtr entry, bool format);
};

  } // end namespace
} // end namespace
#endif
