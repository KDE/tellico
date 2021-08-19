/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#include "document.h"
#include "collectionfactory.h"
#include "translators/tellicoimporter.h"
#include "translators/tellicozipexporter.h"
#include "translators/tellicoxmlexporter.h"
#include "collection.h"
#include "core/filehandler.h"
#include "borrower.h"
#include "fieldformat.h"
#include "core/tellico_strings.h"
#include "images/imagefactory.h"
#include "images/imagedirectory.h"
#include "images/image.h"
#include "images/imageinfo.h"
#include "utils/stringset.h"
#include "utils/mergeconflictresolver.h"
#include "progressmanager.h"
#include "config/tellico_config.h"
#include "entrycomparison.h"
#include "utils/guiproxy.h"
#include "tellico_debug.h"

#include <KMessageBox>
#include <KLocalizedString>

#include <QApplication>

using namespace Tellico;
using Tellico::Data::Document;
Document* Document::s_self = nullptr;

Document::Document() : QObject(), m_coll(nullptr), m_isModified(false),
    m_loadAllImages(false), m_validFile(false), m_importer(nullptr), m_cancelImageWriting(true),
    m_fileFormat(Import::TellicoImporter::Unknown), m_loadImagesTimer(this) {
  m_allImagesOnDisk = Config::imageLocation() != Config::ImagesInFile;
  m_loadImagesTimer.setSingleShot(true);
  m_loadImagesTimer.setInterval(500);
  connect(&m_loadImagesTimer, &QTimer::timeout, this, &Document::slotLoadAllImages);
  newDocument(Collection::Book);
}

Document::~Document() {
  delete m_importer;
  m_importer = nullptr;
}

Tellico::Data::CollPtr Document::collection() const {
  return m_coll;
}

void Document::setURL(const QUrl& url_) {
  m_url = url_;
  if(m_url.fileName() != i18n(Tellico::untitledFilename)) {
    ImageFactory::setLocalDirectory(m_url);
    EntryComparison::setDocumentUrl(m_url);
  }
}

void Document::setModified(bool modified_) {
  if(modified_ != m_isModified) {
    m_isModified = modified_;
    emit signalModified(m_isModified);
  }
}

void Document::slotSetModified() {
  setModified(true);
}

/**
 * Since QUndoStack emits cleanChanged(), the behavior is opposite
 * the document modified flag
 */
void Document::slotSetClean(bool clean_) {
  setModified(!clean_);
}

bool Document::newDocument(int type_) {
  if(m_importer) {
    m_importer->deleteLater();
    m_importer = nullptr;
  }
  deleteContents();

  m_coll = CollectionFactory::collection(type_, true);
  m_coll->setTrackGroups(true);

  emit signalCollectionAdded(m_coll);
  emit signalCollectionImagesLoaded(m_coll);

  setModified(false);
  QUrl url = QUrl::fromLocalFile(i18n(Tellico::untitledFilename));
  setURL(url);
  m_validFile = false;
  m_fileFormat = Import::TellicoImporter::Unknown;

  return true;
}

bool Document::openDocument(const QUrl& url_) {
  MARK;
  // delayed image loading only works for local files
  m_loadAllImages = !url_.isLocalFile();
  m_loadImagesTimer.stop(); // avoid potential race condition

  if(m_importer) {
    m_importer->deleteLater();
  }
  m_importer = new Import::TellicoImporter(url_, m_loadAllImages);

  ProgressItem& item = ProgressManager::self()->newProgressItem(m_importer, m_importer->progressLabel(), true);
  connect(m_importer, &Import::Importer::signalTotalSteps,
          ProgressManager::self(), &ProgressManager::setTotalSteps);
  connect(m_importer, &Import::Importer::signalProgress,
          ProgressManager::self(), &ProgressManager::setProgress);
  connect(&item, &ProgressItem::signalCancelled, m_importer, &Import::Importer::slotCancel);
  ProgressItem::Done done(m_importer);

  CollPtr coll = m_importer->collection();
  if(!m_importer) {
    myDebug() << "The importer was deleted out from under us";
    return false;
  }
  // delayed image loading only works for zip files
  // format is only known AFTER collection() is called

  m_fileFormat = m_importer->format();
  m_allImagesOnDisk = !m_importer->hasImages();
  if(!m_importer->hasImages() || m_fileFormat != Import::TellicoImporter::Zip) {
    m_loadAllImages = true;
  }
  ImageFactory::setZipArchive(m_importer->takeImages());

  if(!coll) {
//    myDebug() << "returning false";
    GUI::Proxy::sorry(m_importer->statusMessage());
    m_validFile = false;
    return false;
  }
  deleteContents();
  m_coll = coll;
  m_coll->setTrackGroups(true);
  setURL(url_);
  m_validFile = true;

  emit signalCollectionAdded(m_coll);

  // m_importer might have been deleted?
  setModified(m_importer && m_importer->modifiedOriginal());
//  if(pruneImages()) {
//    slotSetModified(true);
//  }
  if(m_importer && m_importer->hasImages()) {
    m_cancelImageWriting = false;
    m_loadImagesTimer.start();
  } else {
    emit signalCollectionImagesLoaded(m_coll);
    if(m_importer) {
      m_importer->deleteLater();
      m_importer = nullptr;
    }
  }
  return true;
}

bool Document::saveDocument(const QUrl& url_, bool force_) {
  // FileHandler::queryExists calls FileHandler::writeBackupFile
  // so the only reason to check queryExists() is if the url to write to is different than the current one
  if(url_ == m_url) {
    if(!FileHandler::writeBackupFile(url_)) {
      return false;
    }
  } else {
    if(!force_ && !FileHandler::queryExists(url_)) {
      return false;
    }
  }

  // in case we're still loading images, give that a chance to cancel
  m_cancelImageWriting = true;
  qApp->processEvents();

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, i18n("Saving file..."), false);
  ProgressItem::Done done(this);

  // will always save as zip file, no matter if has images or not
  int imageLocation = Config::imageLocation();
  bool includeImages = imageLocation == Config::ImagesInFile;
  int totalSteps;
  // write all images to disk cache if needed
  // have to do this before executing exporter in case
  // the user changed the imageInFile setting from Yes to No, in which
  // case saving will overwrite the old file that has the images in it!
  if(includeImages) {
    totalSteps = 10;
    item.setTotalSteps(totalSteps);
    // since TellicoZipExporter uses 100 steps, then it will get 100/110 of the total progress
  } else {
    totalSteps = 100;
    item.setTotalSteps(totalSteps);
    m_cancelImageWriting = false;
    writeAllImages(imageLocation == Config::ImagesInAppDir ? ImageFactory::DataDir : ImageFactory::LocalDir, url_);
  }
  QScopedPointer<Export::Exporter> exporter;
  if(m_fileFormat == Import::TellicoImporter::XML) {
    exporter.reset(new Export::TellicoXMLExporter(m_coll));
    static_cast<Export::TellicoXMLExporter*>(exporter.data())->setIncludeImages(includeImages);
  } else {
    exporter.reset(new Export::TellicoZipExporter(m_coll));
    static_cast<Export::TellicoZipExporter*>(exporter.data())->setIncludeImages(includeImages);
  }
  item.setProgress(int(0.8*totalSteps));
  exporter->setEntries(m_coll->entries());
  exporter->setURL(url_);
  // since we already asked about overwriting the file, force the save
  long opt = exporter->options() | Export::ExportForce | Export::ExportComplete | Export::ExportProgress;
  // only write the image sizes if they're known already
  opt &= ~Export::ExportImageSize;
  exporter->setOptions(opt);
  const bool success = exporter->exec();
  item.setProgress(int(0.9*totalSteps));

  if(success) {
    setURL(url_);
    // if successful, doc is no longer modified
    setModified(false);
  } else {
    myDebug() << "Document::saveDocument() - not successful saving to" << url_.url();
  }
  return success;
}

bool Document::closeDocument() {
  if(m_importer) {
    m_importer->deleteLater();
    m_importer = nullptr;
  }
  deleteContents();
  return true;
}

void Document::deleteContents() {
  if(m_coll) {
    emit signalCollectionDeleted(m_coll);
  }
  // don't delete the m_importer here, bad things will happen

  // since the collection holds a pointer to each entry and each entry
  // hold a pointer to the collection, and they're both sharedptrs,
  // neither will ever get deleted, unless the entries are removed from the collection
  if(m_coll) {
    m_coll->clear();
  }
  m_coll = nullptr; // old collection gets deleted as refcount goes to 0
  m_cancelImageWriting = true;
}

void Document::appendCollection(Tellico::Data::CollPtr coll_) {
  appendCollection(m_coll, coll_);
}

void Document::appendCollection(Tellico::Data::CollPtr coll1_, Tellico::Data::CollPtr coll2_) {
  if(!coll1_ || !coll2_) {
    return;
  }

  coll1_->blockSignals(true);

  foreach(FieldPtr field, coll2_->fields()) {
    coll1_->mergeField(field);
  }

  Data::EntryList newEntries;
  foreach(EntryPtr entry, coll2_->entries()) {
    Data::EntryPtr newEntry(new Data::Entry(*entry));
    newEntry->setCollection(coll1_);
    newEntries << newEntry;
  }
  coll1_->addEntries(newEntries);
  // TODO: merge filters and loans
  coll1_->blockSignals(false);
}

Tellico::Data::MergePair Document::mergeCollection(Tellico::Data::CollPtr coll_) {
  return mergeCollection(m_coll, coll_);
}

Tellico::Data::MergePair Document::mergeCollection(Tellico::Data::CollPtr coll1_, Tellico::Data::CollPtr coll2_) {
  MergePair pair;
  if(!coll1_ || !coll2_) {
    return pair;
  }

  coll1_->blockSignals(true);
  Data::FieldList fields = coll2_->fields();
  foreach(FieldPtr field, fields) {
    coll1_->mergeField(field);
  }

  EntryList currEntries = coll1_->entries();
  EntryList newEntries = coll2_->entries();
  std::sort(currEntries.begin(), currEntries.end(), Data::EntryCmp(QStringLiteral("title")));
  std::sort(newEntries.begin(), newEntries.end(), Data::EntryCmp(QStringLiteral("title")));

  const int currTotal = currEntries.count();
  int lastMatchId = 0;
  bool checkSameId = false; // if the matching entries have the same id, then check that first for later comparisons
  foreach(EntryPtr newEntry, newEntries) {
    int bestMatch = 0;
    Data::EntryPtr matchEntry, currEntry;
    // first, if we're checking against same ID
    if(checkSameId) {
      currEntry = coll1_->entryById(newEntry->id());
      if(currEntry && coll1_->sameEntry(currEntry, newEntry) >= EntryComparison::ENTRY_PERFECT_MATCH) {
        // only have to compare against perfect match
        matchEntry = currEntry;
      }
    }
    if(!matchEntry) {
      // alternative is to loop over them all
      for(int i = 0; i < currTotal; ++i) {
        // since we're sorted by title, track the index of the previous match and start comparison there
        currEntry = currEntries.at((i+lastMatchId) % currTotal);
        const int match = coll1_->sameEntry(currEntry, newEntry);
        if(match >= EntryComparison::ENTRY_PERFECT_MATCH) {
          matchEntry = currEntry;
          lastMatchId = (i+lastMatchId) % currTotal;
          break;
        } else if(match >= EntryComparison::ENTRY_GOOD_MATCH && match > bestMatch) {
          bestMatch = match;
          matchEntry = currEntry;
          lastMatchId = (i+lastMatchId) % currTotal;
          // don't break, keep looking for better one
        }
      }
    }
    if(matchEntry) {
      checkSameId = checkSameId || (matchEntry->id() == newEntry->id());
      Merge::mergeEntry(matchEntry, newEntry);
    } else {
      Data::EntryPtr e(new Data::Entry(*newEntry));
      e->setCollection(coll1_);
      // keep track of which entries got added
      pair.first.append(e);
    }
  }
  coll1_->addEntries(pair.first);
  // TODO: merge filters and loans
  coll1_->blockSignals(false);
  return pair;
}

void Document::replaceCollection(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    return;
  }

  QUrl url = QUrl::fromLocalFile(i18n(Tellico::untitledFilename));
  setURL(url);
  m_validFile = false;

  // the collection gets cleared by the CollectionCommand that called this function
  // no need to do it here

  m_coll = coll_;
  m_coll->setTrackGroups(true);
  m_cancelImageWriting = true;
  // CollectionCommand takes care of calling Controller signals
}

void Document::unAppendCollection(Tellico::Data::FieldList origFields_, QList<int> addedEntries_) {
  m_coll->blockSignals(true);

  StringSet origFieldNames;
  foreach(FieldPtr field, origFields_) {
    m_coll->modifyField(field);
    origFieldNames.add(field->name());
  }

  EntryList entriesToRemove;
  foreach(int id, addedEntries_) {
    auto e = m_coll->entryById(id);
    if(e) entriesToRemove << e;
  }
  m_coll->removeEntries(entriesToRemove);

  // since Collection::removeField() iterates over all entries to reset the value of the field
  // don't removeField() until after removeEntry() is done
  FieldList currFields = m_coll->fields();
  foreach(FieldPtr field, currFields) {
    if(!origFieldNames.has(field->name())) {
      m_coll->removeField(field);
    }
  }
  m_coll->blockSignals(false);
}

void Document::unMergeCollection(Tellico::Data::FieldList origFields_, Tellico::Data::MergePair entryPair_) {
  m_coll->blockSignals(true);

  QStringList origFieldNames;
  foreach(FieldPtr field, origFields_) {
    m_coll->modifyField(field);
    origFieldNames << field->name();
  }

  // first item in pair are the entries added by the operation, remove them
  EntryList entries = entryPair_.first;
  m_coll->removeEntries(entries);

  // second item in pair are the entries which got modified by the original merge command
  const QString track = QStringLiteral("track");
  PairVector trackChanges = entryPair_.second;
  // need to go through them in reverse since one entry may have been modified multiple times
  // first item in the pair is the entry pointer
  // second item is the old value of the track field
  for(int i = trackChanges.count()-1; i >= 0; --i) {
    trackChanges[i].first->setField(track, trackChanges[i].second);
  }

  // since Collection::removeField() iterates over all entries to reset the value of the field
  // don't removeField() until after removeEntry() is done
  FieldList currFields = m_coll->fields();
  foreach(FieldPtr field, currFields) {
    if(origFieldNames.indexOf(field->name()) == -1) {
      m_coll->removeField(field);
    }
  }
  m_coll->blockSignals(false);
}

bool Document::isEmpty() const {
  //an empty doc may contain a collection, but no entries
  return (!m_coll || m_coll->entries().isEmpty());
}

bool Document::loadAllImagesNow() const {
//  DEBUG_LINE;
  if(!m_coll || !m_validFile) {
    return false;
  }
  if(m_loadAllImages) {
    myDebug() << "Document::loadAllImagesNow() - all valid images should already be loaded!";
    return false;
  }
  return Import::TellicoImporter::loadAllImages(m_url);
}

Tellico::Data::EntryList Document::filteredEntries(Tellico::FilterPtr filter_) const {
  Data::EntryList matches;
  Data::EntryList entries = m_coll->entries();
  foreach(EntryPtr entry, entries) {
    if(filter_->matches(entry)) {
      matches.append(entry);
    }
  }
  return matches;
}

void Document::checkOutEntry(Tellico::Data::EntryPtr entry_) {
  if(!entry_) {
    return;
  }

  const QString loaned = QStringLiteral("loaned");
  if(!m_coll->hasField(loaned)) {
    FieldPtr f(new Field(loaned, i18n("Loaned"), Field::Bool));
    f->setFlags(Field::AllowGrouped);
    f->setCategory(i18n("Personal"));
    m_coll->addField(f);
  }
  entry_->setField(loaned, QStringLiteral("true"));
  EntryList vec;
  vec.append(entry_);
  m_coll->updateDicts(vec, QStringList() << loaned);
}

void Document::checkInEntry(Tellico::Data::EntryPtr entry_) {
  if(!entry_) {
    return;
  }

  const QString loaned = QStringLiteral("loaned");
  if(!m_coll->hasField(loaned)) {
    return;
  }
  entry_->setField(loaned, QString());
  m_coll->updateDicts(EntryList() << entry_, QStringList() << loaned);
}

void Document::renameCollection(const QString& newTitle_) {
  m_coll->setTitle(newTitle_);
}

// this only gets called when a zip file with images is opened
// by loading every image, it gets pulled out of the zip file and
// copied to disk. Then the zip file can be closed and not retained in memory
void Document::slotLoadAllImages() {
  QString id;
  StringSet images;
  foreach(EntryPtr entry, m_coll->entries()) {
    foreach(FieldPtr field, m_coll->imageFields()) {
      id = entry->field(field);
      if(id.isEmpty() || images.has(id)) {
        continue;
      }
      // this is the early loading, so just by calling imageById()
      // the image gets sucked from the zip file and written to disk
      // by ImageFactory::imageById()
      // TODO:: does this need to check against images with link only?
      if(ImageFactory::imageById(id).isNull()) {
        myDebug() << "Null image for entry:" << entry->title() << id;
      }
      images.add(id);
      if(m_cancelImageWriting) {
        break;
      }
    }
    if(m_cancelImageWriting) {
      break;
    }
    // stay responsive, do this in the background
    qApp->processEvents();
  }

  if(m_cancelImageWriting) {
    myLog() << "slotLoadAllImages() - cancel image writing";
  } else {
    emit signalCollectionImagesLoaded(m_coll);
  }

  m_cancelImageWriting = false;
  if(m_importer) {
    m_importer->deleteLater();
    m_importer = nullptr;
  }
}

// cacheDir_ is the location dir to write the images
// localDir_ provide the new file location which is only needed if cacheDir == LocalDir
void Document::writeAllImages(int cacheDir_, const QUrl& localDir_) {
  // images get 80 steps in saveDocument()
  const uint stepSize = 1 + qMax(1, m_coll->entryCount()/80); // add 1 since it could round off
  uint j = 1;

  ImageFactory::CacheDir cacheDir = static_cast<ImageFactory::CacheDir>(cacheDir_);
  QScopedPointer<ImageDirectory> imgDir;
  if(cacheDir == ImageFactory::LocalDir) {
    imgDir.reset(new ImageDirectory(ImageFactory::localDirectory(localDir_)));
  }

  QString id;
  StringSet images;
  EntryList entries = m_coll->entries();
  FieldList imageFields = m_coll->imageFields();
  foreach(EntryPtr entry, entries) {
    foreach(FieldPtr field, imageFields) {
      id = entry->field(field);
      if(id.isEmpty() || images.has(id)) {
        continue;
      }
      images.add(id);
      if(ImageFactory::imageInfo(id).linkOnly) {
        continue;
      }
      // careful here, if we're writing to LocalDir, need to read from the old LocalDir and write to new
      bool success;
      if(cacheDir == ImageFactory::LocalDir) {
        success = ImageFactory::writeCachedImage(id, imgDir.data());
      } else {
        success = ImageFactory::writeCachedImage(id, cacheDir);
      }
      if(!success) {
        myDebug() << "did not write image for entry title:" << entry->title();
      }
      if(m_cancelImageWriting) {
        break;
      }
    }
    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j/stepSize);
    }
    ++j;
    if(m_cancelImageWriting) {
      break;
    }
  }

  if(m_cancelImageWriting) {
    myDebug() << "Document::writeAllImages() - cancel image writing";
  }

  m_cancelImageWriting = false;
}

bool Document::pruneImages() {
  bool found = false;
  QString id;
  StringSet images;
  Data::EntryList entries = m_coll->entries();
  Data::FieldList imageFields = m_coll->imageFields();
  foreach(EntryPtr entry, entries) {
    foreach(FieldPtr field, imageFields) {
      id = entry->field(field);
      if(id.isEmpty() || images.has(id)) {
        continue;
      }
      const Data::Image& img = ImageFactory::imageById(id);
      if(img.isNull()) {
        entry->setField(field, QString());
        found = true;
        myDebug() << "removing null image for" << entry->title() << ":" << id;
      } else {
        images.add(id);
      }
    }
  }
  return found;
}

int Document::imageCount() const {
  if(!m_coll) {
    return 0;
  }
  StringSet images;
  FieldList fields = m_coll->imageFields();
  EntryList entries = m_coll->entries();
  foreach(FieldPtr field, fields) {
    foreach(EntryPtr entry, entries) {
      images.add(entry->field(field));
    }
  }
  return images.count();
}

void Document::removeImagesNotInCollection(Tellico::Data::EntryList entries_, Tellico::Data::EntryList entriesToKeep_) {
  // first get list of all images in collection
  StringSet images;
  FieldList fields = m_coll->imageFields();
  EntryList allEntries = m_coll->entries();
  foreach(FieldPtr field, fields) {
    foreach(EntryPtr entry, allEntries) {
      images.add(entry->field(field));
    }
    foreach(EntryPtr entry, entriesToKeep_) {
      images.add(entry->field(field));
    }
  }

  // now for all images not in the cache, we can clear them
  StringSet imagesToCheck = ImageFactory::imagesNotInCache();

  // if entries_ is not empty, that means we want to limit the images removed
  // to those that are referenced in those entries
  StringSet imagesToRemove;
  foreach(FieldPtr field, fields) {
    foreach(EntryPtr entry, entries_) {
      QString id = entry->field(field);
      if(!id.isEmpty() && imagesToCheck.has(id) && !images.has(id)) {
        imagesToRemove.add(id);
      }
    }
  }

  const QStringList realImagesToRemove = imagesToRemove.values();
  for(QStringList::ConstIterator it = realImagesToRemove.begin(); it != realImagesToRemove.end(); ++it) {
    ImageFactory::removeImage(*it, false); // doesn't delete, just remove link
  }
}
