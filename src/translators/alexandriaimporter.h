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

#ifndef ALEXANDRIAIMPORTER_H
#define ALEXANDRIAIMPORTER_H

class KComboBox;

#include "importer.h"
#include "../datavectors.h"

#include <QDir>

namespace Tellico {
  namespace Import {

/**
 * An importer for importing collections used by Alexandria, the Gnome book collection manager.
 *
 * The libraries are assumed to be in $HOME/.alexandria. The file format is YAML, but instead
 * using a real YAML reader, the file is parsed line-by-line, so it's very crude. When Alexandria
 * adds new fields or types, this will have to be updated.
 *
 * @author Robby Stephenson
 */
class AlexandriaImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  AlexandriaImporter() : Importer(), m_widget(0), m_cancelled(false) {}
  /**
   */
  virtual ~AlexandriaImporter() {}

  /**
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent);
  virtual bool canImport(int type) const;

public slots:
  void slotCancel();

private:
  static QString& cleanLine(QString& str);
  static QString& clean(QString& str);

  Data::CollPtr m_coll;
  QWidget* m_widget;
  KComboBox* m_library;

  QDir m_libraryDir;
  bool m_cancelled : 1;
};

  } // end namespace
} // end namespace
#endif
