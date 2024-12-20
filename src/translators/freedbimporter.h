/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
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
  virtual Data::CollPtr collection() override;
  /**
   */
  virtual QWidget* widget(QWidget* parent) override;
  virtual bool canImport(int type) const override;

public Q_SLOTS:
  void slotCancel() override;

private Q_SLOTS:
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
