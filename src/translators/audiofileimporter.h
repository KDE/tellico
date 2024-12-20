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

#ifndef TELLICO_AUDIOFILEIMPORTER_H
#define TELLICO_AUDIOFILEIMPORTER_H

class QCheckBox;

#include "importer.h"
#include "../datavectors.h"

namespace TagLib {
  class FileRef;
}

namespace Tellico {
  namespace Import {

/**
 * The AudioFileImporter class takes care of importing audio files.
 *
 * @author Robby Stephenson
 */
class AudioFileImporter : public Importer {
Q_OBJECT

enum AudioFileImporterOptions {
  Recursive    = 1 << 0,
  AddFilePath  = 1 << 1,
  AddBitrate   = 1 << 2
};

public:
  /**
   */
  AudioFileImporter(const QUrl& url);

  /**
   */
  virtual Data::CollPtr collection() override;
  /**
   */
  virtual QWidget* widget(QWidget* parent) override;
  virtual bool canImport(int type) const override;

  void setRecursive(bool recursive);
  void setAddFilePath(bool addFilePath);
  void setAddBitrate(bool addBitrate);

public Q_SLOTS:
  void slotCancel() override;
  void slotAddFileToggled(bool on);

private:
  static QString insertValue(const QString& str, const QString& value, int pos);

  int discNumber(const TagLib::FileRef& file) const;

  Data::CollPtr m_coll;
  QWidget* m_widget;
  QCheckBox* m_recursive;
  QCheckBox* m_addFilePath;
  QCheckBox* m_addBitrate;
  bool m_cancelled;
  int m_audioOptions;
};

  } // end namespace
} // end namespace
#endif
