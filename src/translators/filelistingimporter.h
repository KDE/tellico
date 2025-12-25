/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_FILELISTINGIMPORTER_H
#define TELLICO_IMPORT_FILELISTINGIMPORTER_H

#include "importer.h"
#include "../datavectors.h"

#include <KFileItem>

#include <QPointer>

class QCheckBox;
namespace KIO {
  class Job;
}

namespace Tellico {
  namespace GUI {
    class CollectionTypeCombo;
  }
  namespace Import {

/**
 * @author Robby Stephenson
 */
class FileListingImporter : public Importer {
Q_OBJECT

public:
  FileListingImporter(const QUrl& url);

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection() override;
  /**
   */
  virtual QWidget* widget(QWidget* parent) override;
  virtual bool canImport(int type) const override;

  void setUseFilePreview(bool b) { m_useFilePreview = b; }
  void setCollectionType(int type_) { m_collType = type_; }

public Q_SLOTS:
  void slotCancel() override;

private Q_SLOTS:
  void slotEntries(KIO::Job* job, const KIO::UDSEntryList& list);

private:
  QString volumeName() const;

  int m_collType;
  Data::CollPtr m_coll;
  QWidget* m_widget;
  GUI::CollectionTypeCombo* m_collCombo;
  QCheckBox* m_recursive;
  QCheckBox* m_filePreview;

  QPointer<KIO::Job> m_job;
  KFileItemList m_files;
  bool m_useRecursive;
  bool m_useFilePreview;
  bool m_cancelled;
};

  } // end namespace
} // end namespace
#endif
