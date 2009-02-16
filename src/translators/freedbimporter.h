/***************************************************************************
    copyright            : (C) 2004-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FREEDBIMPORTER_H
#define TELLICO_FREEDBIMPORTER_H

#include "importer.h"
#include "../datavectors.h"

#include <QByteArray>
#include <QList>
#include <QVector>

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
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent);
  virtual bool canImport(int type) const;

public slots:
  void slotCancel();

private slots:
  void slotClicked(int id);

private:
  typedef QVector<QString> StringVector;
  struct CDText {
    friend class FreeDBImporter;
    QString title;
    QString artist;
    QString message;
    StringVector trackTitles;
    StringVector trackArtists;
  };

  static QList<uint> offsetList(const QByteArray& drive, QList<uint>& trackLengths);
  static CDText getCDText(const QByteArray& drive);

  void readCDROM();
  void readCache();
  void readCDText(const QByteArray& drive);

  Data::CollPtr m_coll;
  QWidget* m_widget;
  QButtonGroup* m_buttonGroup;
  QRadioButton* m_radioCDROM;
  QRadioButton* m_radioCache;
  KComboBox* m_driveCombo;
  bool m_cancelled : 1;
};

  } // end namespace
} // end namespace
#endif
