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

#include <config.h>

#include "filelistingimporter.h"
#include "filereader.h"
#include "filereaderbook.h"
#include "filereadervideo.h"
#include "../collections/bookcollection.h"
#include "../collections/videocollection.h"
#include "../collections/filecatalog.h"
#include "../entry.h"
#include "../gui/collectiontypecombo.h"
#include "../utils/guiproxy.h"
#include "../progressmanager.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KJobWidgets>
#include <KIO/ListJob>

#include <QDate>
#include <QDir>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QFile>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QApplication>

using Tellico::Import::FileListingImporter;

FileListingImporter::FileListingImporter(const QUrl& url_) : Importer(url_), m_collType(Data::Collection::File), m_coll(nullptr)
    , m_widget(nullptr), m_collCombo(nullptr), m_recursive(nullptr), m_filePreview(nullptr), m_job(nullptr), m_useFilePreview(false), m_cancelled(false) {
}

bool FileListingImporter::canImport(int type) const {
  return type == Data::Collection::Book ||
      type == Data::Collection::Video ||
      type == Data::Collection::File;
}

Tellico::Data::CollPtr FileListingImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, i18n("Scanning files..."), true);
  item.setTotalSteps(100);
  connect(&item, &Tellico::ProgressItem::signalCancelled, this, &FileListingImporter::slotCancel);
  ProgressItem::Done done(this);

  // the importer might be running without a gui/widget
  KIO::JobFlags flags = KIO::DefaultFlags;
  if(!m_widget) flags |= KIO::HideProgressInfo;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  const bool includeHidden = false;
#else
  const auto includeHidden = KIO::ListJob::ListFlags{};
#endif
  m_job = (m_widget && m_recursive->isChecked())
          ? KIO::listRecursive(url(), flags, includeHidden)
          : KIO::listDir(url(), flags, includeHidden);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  void (KIO::ListJob::* jobEntries)(KIO::Job*, const KIO::UDSEntryList&) = &KIO::ListJob::entries;
  connect(static_cast<KIO::ListJob*>(m_job.data()), jobEntries, this, &FileListingImporter::slotEntries);

  if(!m_job->exec() || m_cancelled) {
    myDebug() << "did not run job:" << m_job->errorString();
    return Data::CollPtr();
  }

  if(m_widget) {
    m_useFilePreview = m_filePreview->isChecked();
    m_collType = m_collCombo->currentType();
  }

  const uint stepSize = qMax(1, m_files.count()/100);
  const bool showProgress = options() & ImportProgress;
  item.setTotalSteps(m_files.count());

  std::unique_ptr<AbstractFileReader> reader;
  switch(m_collType) {
    case(Data::Collection::Book):
      m_coll = new Data::BookCollection(true);
      reader.reset(new FileReaderBook(url()));
      break;

    case(Data::Collection::Video):
      m_coll = new Data::VideoCollection(true);
      reader.reset(new FileReaderVideo(url()));
      break;

    case(Data::Collection::File):
      m_coll = new Data::FileCatalog(true);
      reader.reset(new FileReaderFile(url()));
      break;
  }
  if(!reader) return Data::CollPtr();
  reader->setUseFilePreview(m_useFilePreview);

  Data::EntryList entries;
  uint j = 0;
  foreach(const KFileItem& item, m_files) {
    if(m_cancelled) {
      break;
    }

    Data::EntryPtr entry(new Data::Entry(m_coll));
    if(reader->populate(entry, item)) {
      entries += entry;
    }

    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      qApp->processEvents();
    }
    ++j;
  }
  m_coll->addEntries(entries);

  if(m_cancelled) {
    m_coll = Data::CollPtr();
    return m_coll;
  }

  return m_coll;
}

QWidget* FileListingImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("File Listing Options"), m_widget);
  auto lay = new QGridLayout(gbox);

  m_recursive = new QCheckBox(i18n("Recursive folder search"), gbox);
  m_recursive->setWhatsThis(i18n("If checked, folders are recursively searched for all files."));
  // by default, make it checked
  m_recursive->setChecked(true);

  m_filePreview = new QCheckBox(i18n("Generate file previews"), gbox);
  m_filePreview->setWhatsThis(i18n("If checked, previews of the file contents are generated, which can slow down "
                                   "the folder listing."));
  // by default, make it no previews
  m_filePreview->setChecked(false);

  QList<int> collTypes;
  collTypes << Data::Collection::Book << Data::Collection::Video << Data::Collection::File;
  m_collCombo = new GUI::CollectionTypeCombo(gbox);
  m_collCombo->setIncludedTypes(collTypes);
  m_collCombo->setCurrentData(m_collType);
  m_collCombo->setWhatsThis(i18n("Select the type of collection being imported."));
  auto lab = new QLabel(i18n("Collection &type:"), gbox);
  lab->setBuddy(m_collCombo);

  int row = 0;
  lay->addWidget(m_recursive, row++, 0, 1, -1);
  lay->addWidget(m_filePreview, row++, 0, 1, -1);
  lay->addWidget(lab, row, 0);
  lay->addWidget(m_collCombo, row++, 1);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void FileListingImporter::slotEntries(KIO::Job* job_, const KIO::UDSEntryList& list_) {
  if(m_cancelled) {
    job_->kill();
    m_job = nullptr;
    return;
  }

  for(KIO::UDSEntryList::ConstIterator it = list_.begin(); it != list_.end(); ++it) {
    KFileItem item(*it, url(), false, true);
    if(item.isFile()) {
      m_files.append(item);
    }
  }
}

void FileListingImporter::slotCancel() {
  m_cancelled = true;
  if(m_job) {
    m_job->kill();
  }
}
