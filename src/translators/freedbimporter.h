/***************************************************************************
    copyright            : (C) 2004-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef FREEDBIMPORTER_H
#define FREEDBIMPORTER_H

#include "importer.h"
#include <config.h>

class QButtonGroup;
class QRadioButton;
class KComboBox;

namespace Tellico {
  namespace Import {

/**
 * The FreeDBImporter class takes care of importing audio files.
 *
 * @author Robby Stephenson
 */
class FreeDBImporter : public Importer {
Q_OBJECT

public:
  /**
   */
  FreeDBImporter();

  /**
   */
  virtual Data::Collection* collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual bool canImport(int type) const;

private slots:
  void slotClicked(int id);

private:
  Data::Collection* readCDROM();
  Data::Collection* readCache();

  static QValueList<uint> FreeDBImporter::offsetList(QCString drive);

  Data::Collection* m_coll;
  QWidget* m_widget;
  QButtonGroup* m_buttonGroup;
  QRadioButton* m_radioCDROM;
  QRadioButton* m_radioCache;
  KComboBox* m_driveCombo;
};

  } // end namespace
} // end namespace
#endif
