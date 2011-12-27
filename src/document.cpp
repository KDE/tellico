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
#include "filehandler.h"
#include "borrower.h"
#include "fieldformat.h"
#include "tellico_strings.h"
#include "images/imagefactory.h"
#include "images/image.h"
#include "images/imageinfo.h"
#include "utils/stringset.h"
#include "progressmanager.h"
#include "core/tellico_config.h"
#include "entrycomparison.h"
#include "gui/guiproxy.h"
#include "tellico_debug.h"

#include <kmessagebox.h>
#include <klocale.h>
#include <kglobal.h>
#include <kapplication.h>

#include <QRegExp>
#include <QTimer>

#include <unistd.h>

using namespace Tellico;
using Tellico::Data::Document;
Document* Document::s_self = 0;

Document::Document() : QObject(), m_coll(0), m_isModified(false),
    m_loadAllImages(false), m_validFile(false), m_importer(0), m_cancelImageWriting(false),
    m_fileFormat(Import::TellicoImporter::Unknown) {
  m_allImagesOnDisk = Config::imageLocation() != Config::ImagesInFile;
  newDocument(Collection::Book);
}

Document::~Document() {
  delete m_importer;
  m_importer = 0;
}

Tellico::Data::CollPtr Document::collection() const {
  return m_coll;
}

void Document::setURL(const KUrl& url_) {
  m_url = url_;
  if(m_url.fileName() != i18n(Tellico::untitledFilename)) {
    ImageFactory::setLocalDirectory(m_url);
    EntryComparison::setDocumentUrl(m_url);
  }
}

void Document::slotSetModified(bool modified_/*=true*/) {
  if(modified_ != m_isModified) {
    m_isModified = modified_;
    emit signalModified(m_isModified);
  }
}

/**
 * Since QUndoStack emits cleanChanged(), the behavior is opposite
 * the document modified flag
 */
void Document::slotSetClean(bool clean_) {
   slotSetModified(!clean_);
}

bool Document::newDocument(int type_) {
  delete m_importer;
  m_importer = 0;
  deleteContents();

  m_coll = CollectionFactory::collection(type_, true);
  m_coll->setTrackGroups(true);

  emit signalCollectionAdded(m_coll);

  slotSetModified(false);
  KUrl url;
  url.setFileName(i18n(Tellico::untitledFilename));
  setURL(url);
  m_validFile = false;
  m_fileFormat = Import::TellicoImporter::Unknown;

  return true;
}

bool Document::openDocument(const KUrl& url_) {
  MARK;
  m_loadAllImages = false;
  // delayed image loading only works for local files
  if(!url_.isLocalFile()) {
    m_loadAllImages = true;
  }

  delete m_importer;
  m_importer = new Import::TellicoImporter(url_, m_loadAllImages);

  ProgressItem& item = ProgressManager::self()->newProgressItem(m_importer, m_importer->progressLabel(), true);
  connect(m_importer, SIGNAL(signalTotalSteps(QObject*, qulonglong)),
          ProgressManager::self(), SLOT(setTotalSteps(QObject*, qulonglong)));
  connect(m_importer, SIGNAL(signalProgress(QObject*, qulonglong)),
          ProgressManager::self(), SLOT(setProgress(QObject*, qulonglong)));
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), m_importer, SLOT(slotCancel()));
  ProgressItem::Done done(m_importer);

  CollPtr coll = m_importer->collection();
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
  slotSetModified(m_importer && m_importer->modifiedOriginal());
//  if(pruneImages()) {
//    slotSetModified(true);
//  }
  if(m_importer->hasImages()) {
    m_cancelImageWriting = false;
    QTimer::singleShot(500, this, SLOT(slotLoadAllImages()));
  } else {
    emit signalCollectionImagesLoaded(m_coll);
    m_importer->deleteLater();
    m_importer = 0;
  }
  return true;
}

bool Document::saveDocument(const KUrl& url_) {
  // FileHandler::queryExists calls FileHandler::writeBackupFile
  // so the only reason to check queryExists() is if the url to write to is different than the current one
  if(url_ == m_url) {
    if(!FileHandler::writeBackupFile(url_)) {
      return false;
    }
  } else {
    if(!FileHandler::queryExists(url_)) {
      return false;
    }
  }

  // in case we're still loading images, give that a chance to cancel
  m_cancelImageWriting = true;
  kapp->processEvents();

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, i18n("Saving file..."), false);
  ProgressItem::Done done(this);

  // will always save as zip file, no matter if has images or not
  int imageLocation = Config::imageLocation();
  bool includeImages = imageLocation == Config::ImagesInFile;
  int totalSteps;
  // write all images to disk cache if needed
  // have to do this before executing exporter in case
  // the user changed the imageInFile setting from Yes to No, in which
  // case saving will over write the old file that has the images in it!
  if(includeImages) {
    totalSteps = 10;
    item.setTotalSteps(10);
    // since TellicoZipExporter uses 100 steps, then it will get 100/110 of the total progress
  } else {
    totalSteps = 100;
    item.setTotalSteps(100);
    m_cancelImageWriting = false;
    writeAllImages(imageLocation == Config::ImagesInAppDir ? ImageFactory::DataDir : ImageFactory::LocalDir, url_);
  }
  Export::Exporter* exporter;
  if(m_fileFormat == Import::TellicoImporter::XML) {
    exporter = new Export::TellicoXMLExporter(m_coll);
    static_cast<Export::TellicoXMLExporter*>(exporter)->setIncludeImages(includeImages);
  } else {
    exporter = new Export::TellicoZipExporter(m_coll);
    static_cast<Export::TellicoZipExporter*>(exporter)->setIncludeImages(includeImages);
  }
  item.setProgress(int(0.8*totalSteps));
  exporter->setEntries(m_coll->entries());
  exporter->setURL(url_);
  // since we already asked about overwriting the file, force the save
  long opt = exporter->options() | Export::ExportForce | Export::ExportComplete | Export::ExportProgress;
  // only write the image sizes if they're known already
  opt &= ~Export::ExportImageSize;
  exporter->setOptions(opt);
  bool success = exporter->exec();
  item.setProgress(int(0.9*totalSteps));

  if(success) {
    setURL(url_);
    // if successful, doc is no longer modified
    slotSetModified(false);
  } else {
    myDebug() << "not successful saving to " << url_;
  }
  delete exporter;
  return success;
}

bool Document::closeDocument() {
  delete m_importer;
  m_importer = 0;
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
  m_coll = 0; // old collection gets deleted as a KSharedPtr
  m_cancelImageWriting = true;
}

void Document::appendCollection(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);

  foreach(FieldPtr field, coll_->fields()) {
    m_coll->mergeField(field);
  }

  foreach(EntryPtr entry, coll_->entries()) {
    Data::EntryPtr newEntry(new Data::Entry(*entry));
    newEntry->setCollection(m_coll);
  }
  m_coll->addEntries(coll_->entries());
  // TODO: merge filters and loans
  m_coll->blockSignals(false);
}

Tellico::Data::MergePair Document::mergeCollection(Tellico::Data::CollPtr coll_) {
  MergePair pair;
  if(!coll_) {
    return pair;
  }

  m_coll->blockSignals(true);
  Data::FieldList fields = coll_->fields();
  foreach(FieldPtr field, fields) {
    m_coll->mergeField(field);
  }

  EntryList currEntries = m_coll->entries();
  EntryList newEntries = coll_->entries();
  foreach(EntryPtr newEntry, newEntries) {
    int bestMatch = 0;
    Data::EntryPtr matchEntry;
    foreach(EntryPtr currEntry, currEntries) {
      int match = m_coll->sameEntry(currEntry, newEntry);
      if(match >= EntryComparison::ENTRY_PERFECT_MATCH) {
        matchEntry = currEntry;
        break;
      } else if(match >= EntryComparison::ENTRY_GOOD_MATCH && match > bestMatch) {
        bestMatch = match;
        matchEntry = currEntry;
        // don't break, keep looking for better one
      }
    }
    if(matchEntry) {
      mergeEntry(matchEntry, newEntry);
    } else {
      Data::EntryPtr e(new Data::Entry(*newEntry));
      e->setCollection(m_coll);
      // keep track of which entries got added
      pair.first.append(e);
    }
  }
  m_coll->addEntries(pair.first);
  // TODO: merge filters and loans
  m_coll->blockSignals(false);
  return pair;
}

void Document::replaceCollection(Tellico::Data::CollPtr coll_) {
  if(!coll_) {
    return;
  }

  KUrl url;
  url.setFileName(i18n(Tellico::untitledFilename));
  setURL(url);
  m_validFile = false;

  // the collection gets cleared by the CollectionCommand that called this function
  // no need to do it here

  m_coll = coll_;
  m_coll->setTrackGroups(true);
  m_cancelImageWriting = true;
  // CollectionCommand takes care of calling Controller signals
}

void Document::unAppendCollection(Tellico::Data::CollPtr coll_, Tellico::Data::FieldList origFields_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);

  StringSet origFieldNames;
  foreach(FieldPtr field, origFields_) {
    m_coll->modifyField(field);
    origFieldNames.add(field->name());
  }

  EntryList entries = coll_->entries();
  foreach(EntryPtr entry, entries) {
    // probably don't need to do this, but on the safe side...
    entry->setCollection(coll_);
  }
  m_coll->removeEntries(entries);

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

void Document::unMergeCollection(Tellico::Data::CollPtr coll_, Tellico::Data::FieldList origFields_, Tellico::Data::MergePair entryPair_) {
  if(!coll_) {
    return;
  }

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
  const QString track = QLatin1String("track");
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
    myDebug() << "all valid images should already be loaded!";
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

  const QString loaned = QLatin1String("loaned");
  if(!m_coll->hasField(loaned)) {
    FieldPtr f(new Field(loaned, i18n("Loaned"), Field::Bool));
    f->setFlags(Field::AllowGrouped);
    f->setCategory(i18n("Personal"));
    m_coll->addField(f);
  }
  entry_->setField(loaned, QLatin1String("true"));
  EntryList vec;
  vec.append(entry_);
  m_coll->updateDicts(vec, QStringList() << loaned);
}

void Document::checkInEntry(Tellico::Data::EntryPtr entry_) {
  if(!entry_) {
    return;
  }

  const QString loaned = QLatin1String("loaned");
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
// copied to disk. then the zip file can be closed and not retained in memory
void Document::slotLoadAllImages() {
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
      // this is the early loading, so just by calling imageById()
      // the image gets sucked from the zip file and written to disk
      //by ImageFactory::imageById()
      if(ImageFactory::imageById(id).isNull()) {
        myDebug() << "entry title:" << entry->title();
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
    kapp->processEvents();
  }

  if(m_cancelImageWriting) {
    myLog() << "cancel image writing";
  } else {
    emit signalCollectionImagesLoaded(m_coll);
  }

  m_cancelImageWriting = false;
}

void Document::writeAllImages(int cacheDir_, const KUrl& localDir_) {
  // images get 80 steps in saveDocument()
  const uint stepSize = 1 + qMax(1, m_coll->entryCount()/80); // add 1 since it could round off
  uint j = 1;

  QString oldLocalDir = ImageFactory::localDir();
  ImageFactory::setLocalDirectory(localDir_);

  ImageFactory::CacheDir cacheDir = static_cast<ImageFactory::CacheDir>(cacheDir_);

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
      if(!ImageFactory::writeCachedImage(id, cacheDir)) {
        myDebug() << "did not write image for entry title:" << entry->title();
      }
      if(m_cancelImageWriting) {
        break;
      }
    }
    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j/stepSize);
      kapp->processEvents();
    }
    ++j;
    if(m_cancelImageWriting) {
      break;
    }
  }

  if(m_cancelImageWriting) {
    myDebug() << "cancel image writing";
  }

  m_cancelImageWriting = false;
  ImageFactory::setLocalDirectory(oldLocalDir);
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
      images.add(entry->field(field->name()));
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
      images.add(entry->field(field->name()));
    }
    foreach(EntryPtr entry, entriesToKeep_) {
      images.add(entry->field(field->name()));
    }
  }

  // now for all images not in the cache, we can clear them
  StringSet imagesToCheck = ImageFactory::imagesNotInCache();

  // if entries_ is not empty, that means we want to limit the images removed
  // to those that are referenced in those entries
  StringSet imagesToRemove;
  foreach(FieldPtr field, fields) {
    foreach(EntryPtr entry, entries_) {
      QString id = entry->field(field->name());
      if(!id.isEmpty() && imagesToCheck.has(id) && !images.has(id)) {
        imagesToRemove.add(id);
      }
    }
  }

  const QStringList realImagesToRemove = imagesToRemove.toList();
  for(QStringList::ConstIterator it = realImagesToRemove.begin(); it != realImagesToRemove.end(); ++it) {
    ImageFactory::removeImage(*it, false); // doesn't delete, just remove link
  }
}

bool Document::mergeEntry(Data::EntryPtr e1, Data::EntryPtr e2, MergeConflictResolver* resolver_) {
  if(!e1 || !e2) {
    myDebug() << "bad entry pointer";
    return false;
  }
  bool ret = true;
  foreach(FieldPtr field, e1->collection()->fields()) {
    if(e2->field(field).isEmpty()) {
      continue;
    }
    // never try to merge entry id, creation date or mod date. Those are unique to each entry
    if(field->name() == QLatin1String("id") ||
       field->name() == QLatin1String("cdate") ||
       field->name() == QLatin1String("mdate")) {
      continue;
    }
//    myLog() << "reading field: " << field->name();
    if(e1->field(field) == e2->field(field)) {
      continue;
    } else if(e1->field(field).isEmpty()) {
//      myLog() << e1->title() << ": updating field(" << field->name() << ") to " << e2->field(field);
      e1->setField(field, e2->field(field));
      ret = true;
    } else if(field->type() == Data::Field::Table) {
      // if field F is a table-type field (album tracks, files, etc.), merge rows (keep their position)
      // if e1's F val in [row i, column j] empty, replace with e2's val at same position
      // if different (non-empty) vals at same position, CONFLICT!
      QStringList vals1 = FieldFormat::splitTable(e1->field(field));
      QStringList vals2 = FieldFormat::splitTable(e2->field(field));
      while(vals1.count() < vals2.count()) {
        vals1 += QString();
      }
      for(int i = 0; i < vals2.count(); ++i) {
        if(vals2[i].isEmpty()) {
          continue;
        }
        if(vals1[i].isEmpty()) {
          vals1[i] = vals2[i];
          ret = true;
        } else {
          QStringList parts1 = FieldFormat::splitRow(vals1[i]);
          QStringList parts2 = FieldFormat::splitRow(vals2[i]);
          bool changedPart = false;
          while(parts1.count() < parts2.count()) {
            parts1 += QString();
          }
          for(int j = 0; j < parts2.count(); ++j) {
            if(parts2[j].isEmpty()) {
              continue;
            }
            if(parts1[j].isEmpty()) {
              parts1[j] = parts2[j];
              changedPart = true;
            } else if(resolver_ && parts1[j] != parts2[j]) {
              int resolverResponse = resolver_->resolve(e1, e2, field, parts1[j], parts2[j]);
              if(resolverResponse == MergeConflictResolver::CancelMerge) {
                ret = false;
                return false; // cancel all the merge right now
              } else if(resolverResponse == MergeConflictResolver::KeepSecond) {
                parts1[j] = parts2[j];
                changedPart = true;
              }
            }
          }
          if(changedPart) {
            vals1[i] = parts1.join(FieldFormat::columnDelimiterString());
            ret = true;
          }
        }
      }
      if(ret) {
        e1->setField(field, vals1.join(FieldFormat::rowDelimiterString()));
      }
// remove the merging due to user comments
// maybe in the future have a more intelligent way
#if 0
    } else if(field->hasFlag(Data::Field::AllowMultiple)) {
      // if field F allows multiple values and not a Table (see above case),
      // e1's F values = (e1's F values) U (e2's F values) (union)
      // replace e1's field with union of e1's and e2's values for this field
      QStringList items1 = e1->fields(field, false);
      QStringList items2 = e2->fields(field, false);
      foreach(const QString& item2, items2) {
        // possible to have one value formatted and the other one not...
        if(!items1.contains(item2) && !items1.contains(Field::format(item2, field->formatType()))) {
          items1.append(item2);
        }
      }
// not sure if I think it should be sorted or not
//      items1.sort();
      e1->setField(field, items1.join(FieldFormat::delimiterString()));
      ret = true;
#endif
    } else if(resolver_) {
      int resolverResponse = resolver_->resolve(e1, e2, field);
      if(resolverResponse == MergeConflictResolver::CancelMerge) {
        ret = false; // we got cancelled
        return false; // cancel all the merge right now
      } else if(resolverResponse == MergeConflictResolver::KeepSecond) {
        e1->setField(field, e2->field(field));
      }
    } else {
      myDebug() << "Doing nothing for" << field->name();
    }
  }
  return ret;
}

#include "document.moc"
