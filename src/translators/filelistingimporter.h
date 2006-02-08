/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMPORT_FILELISTINGIMPORTER_H
#define TELLICO_IMPORT_FILELISTINGIMPORTER_H

#include "importer.h"
#include "../datavectors.h"

#include <kio/global.h>
#include <kfileitem.h>

#include <qguardedptr.h>

class QCheckBox;
namespace KIO {
  class Job;
}

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
 */
class FileListingImporter : public Importer {
Q_OBJECT

public:
  FileListingImporter(const KURL& url);

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*, const char*);
  virtual bool canImport(int type) const;

public slots:
  void slotCancel();

private slots:
  void slotEntries(KIO::Job* job, const KIO::UDSEntryList& list);
  void slotResult(KIO::Job* job);
  void slotPreview(const KFileItem* item, const QPixmap& pix);

private:
  QString volumeName() const;
  void enter_loop();

  Data::CollPtr m_coll;
  QWidget* m_widget;
  QCheckBox* m_recursive;
  QCheckBox* m_filePreview;
  QGuardedPtr<KIO::Job> m_job;
  bool m_jobOK;
  KFileItemList m_files;
  QPixmap m_pixmap;
  bool m_cancelled : 1;
};

  } // end namespace
} // end namespace
#endif
