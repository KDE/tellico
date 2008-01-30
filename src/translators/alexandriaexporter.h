/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef ALEXANDRIAEXPORTER_H
#define ALEXANDRIAEXPORTER_H

class QDir;

#include "exporter.h"

namespace Tellico {
  namespace Data {
    class Entry;
  }
  namespace Export {

/**
 * @author Robby Stephenson
 */
class AlexandriaExporter : public Exporter {
Q_OBJECT

public:
  AlexandriaExporter() : Exporter() {}

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const { return QString::null; } // no need for this

  // no config options
  virtual QWidget* widget(QWidget*, const char*) { return 0; }

private:
  static QString& escapeText(QString& str);

  bool writeFile(const QDir& dir, Data::ConstEntryPtr entry);
};

  } // end namespace
} // end namespace
#endif
