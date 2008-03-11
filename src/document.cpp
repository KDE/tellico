/***************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "document.h"
#include "mainwindow.h" // needed for calling fileSave()
#include "collectionfactory.h"
#include "translators/tellicoimporter.h"
#include "translators/tellicozipexporter.h"
#include "translators/tellicoxmlexporter.h"
#include "collection.h"
#include "filehandler.h"
#include "controller.h"
#include "borrower.h"
#include "tellico_kernel.h"
#include "latin1literal.h"
#include "tellico_debug.h"
#include "imagefactory.h"
#include "image.h"
#include "stringset.h"
#include "progressmanager.h"
#include "core/tellico_config.h"

#include <kmessagebox.h>
#include <klocale.h>
#include <kglobal.h>
#include <kapplication.h>

#include <qregexp.h>
#include <qtimer.h>

// use a vector so we can use a sort functor
#include <vector>
#include <algorithm>

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

void Document::setURL(const KURL& url_) {
  m_url = url_;
  if(m_url.fileName() != i18n("Untitled")) {
    ImageFactory::setLocalDirectory(m_url);
  }
}

void Document::slotSetModified(bool m_/*=true*/) {
  m_isModified = m_;
  emit signalModified(m_isModified);
}

void Document::slotDocumentRestored() {
  slotSetModified(false);
}

bool Document::newDocument(int type_) {
//  kdDebug() << "Document::newDocument()" << endl;
  delete m_importer;
  m_importer = 0;
  deleteContents();

  m_coll = CollectionFactory::collection(static_cast<Collection::Type>(type_), true);
  m_coll->setTrackGroups(true);

  Kernel::self()->resetHistory();
  Controller::self()->slotCollectionAdded(m_coll);

  slotSetModified(false);
  KURL url;
  url.setFileName(i18n("Untitled"));
  setURL(url);
  m_validFile = false;
  m_fileFormat = Import::TellicoImporter::Unknown;

  return true;
}

bool Document::openDocument(const KURL& url_) {
  myLog() << "Document::openDocument() - " << url_.prettyURL() << endl;

  m_loadAllImages = false;
  // delayed image loading only works for local files
  if(!url_.isLocalFile()) {
    m_loadAllImages = true;
  }

  delete m_importer;
  m_importer = new Import::TellicoImporter(url_, m_loadAllImages);

  CollPtr coll = m_importer->collection();
  // delayed image loading only works for zip files
  // format is only known AFTER collection() is called

  m_fileFormat = m_importer->format();
  m_allImagesOnDisk = !m_importer->hasImages();
  if(!m_importer->hasImages() || m_fileFormat != Import::TellicoImporter::Zip) {
    m_loadAllImages = true;
  }

  if(!coll) {
//    myDebug() << "Document::openDocument() - returning false" << endl;
    Kernel::self()->sorry(m_importer->statusMessage());
    m_validFile = false;
    return false;
  }
  deleteContents();
  m_coll = coll;
  m_coll->setTrackGroups(true);
  setURL(url_);
  m_validFile = true;

  Kernel::self()->resetHistory();
  Controller::self()->slotCollectionAdded(m_coll);

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
  }
  return true;
}

bool Document::saveModified() {
  bool completed = true;

  if(m_isModified) {
    MainWindow* app = static_cast<MainWindow*>(Kernel::self()->widget());
    QString str = i18n("The current file has been modified.\n"
                       "Do you want to save it?");
    int want_save = KMessageBox::warningYesNoCancel(Kernel::self()->widget(), str, i18n("Unsaved Changes"),
                                                    KStdGuiItem::save(), KStdGuiItem::discard());
    switch(want_save) {
      case KMessageBox::Yes:
        completed = app->fileSave();
        break;

      case KMessageBox::No:
        slotSetModified(false);
        completed = true;
        break;

      case KMessageBox::Cancel:
      default:
        completed = false;
        break;
    }
  }

  return completed;
}

bool Document::saveDocument(const KURL& url_) {
  if(!FileHandler::queryExists(url_)) {
    return false;
  }
//  DEBUG_BLOCK;

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
    exporter = new Export::TellicoXMLExporter();
    static_cast<Export::TellicoXMLExporter*>(exporter)->setIncludeImages(includeImages);
  } else {
    exporter = new Export::TellicoZipExporter();
    static_cast<Export::TellicoZipExporter*>(exporter)->setIncludeImages(includeImages);
  }
  item.setProgress(int(0.8*totalSteps));
  exporter->setEntries(m_coll->entries());
  exporter->setURL(url_);
  // since we already asked about overwriting the file, force the save
  long opt = exporter->options() | Export::ExportForce | Export::ExportProgress;
  // only write the image sizes if they're known already
  opt &= ~Export::ExportImageSize;
  exporter->setOptions(opt);
  bool success = exporter->exec();
  item.setProgress(int(0.9*totalSteps));

  if(success) {
    Kernel::self()->resetHistory();
    setURL(url_);
    // if successful, doc is no longer modified
    slotSetModified(false);
  } else {
    myDebug() << "Document::saveDocument() - not successful saving to " << url_.prettyURL() << endl;
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
    Controller::self()->slotCollectionDeleted(m_coll);
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

void Document::appendCollection(CollPtr coll_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);
  Data::FieldVec fields = coll_->fields();
  for(FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
    m_coll->mergeField(field);
  }

  EntryVec entries = coll_->entries();
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    Data::EntryPtr newEntry = new Data::Entry(*entry);
    newEntry->setCollection(m_coll);
  }
  m_coll->addEntries(entries);
  // TODO: merge filters and loans
  m_coll->blockSignals(false);
}

Tellico::Data::MergePair Document::mergeCollection(CollPtr coll_) {
  MergePair pair;
  if(!coll_) {
    return pair;
  }

  m_coll->blockSignals(true);
  Data::FieldVec fields = coll_->fields();
  for(FieldVec::Iterator field = fields.begin(); field != fields.end(); ++field) {
    m_coll->mergeField(field);
  }

  EntryVec currEntries = m_coll->entries();
  EntryVec newEntries = coll_->entries();
  for(EntryVec::Iterator newIt = newEntries.begin(); newIt != newEntries.end(); ++newIt) {
    int bestMatch = 0;
    Data::EntryPtr matchEntry;
    for(EntryVec::Iterator currIt = currEntries.begin(); currIt != currEntries.end(); ++currIt) {
      int match = m_coll->sameEntry(&*currIt, &*newIt);
      if(match >= Collection::ENTRY_PERFECT_MATCH) {
        matchEntry = currIt;
        break;
      } else if(match >= Collection::ENTRY_GOOD_MATCH && match > bestMatch) {
        bestMatch = match;
        matchEntry = currIt;
        // don't break, keep looking for better one
      }
    }
    if(matchEntry) {
      m_coll->mergeEntry(matchEntry, &*newIt, false /*overwrite*/);
    } else {
      Data::EntryPtr e = new Data::Entry(*newIt);
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

void Document::replaceCollection(CollPtr coll_) {
  if(!coll_) {
    return;
  }

//  kdDebug() << "Document::replaceCollection()" << endl;

  KURL url;
  url.setFileName(i18n("Untitled"));
  setURL(url);
  m_validFile = false;

  // the collection gets cleared by the CollectionCommand that called this function
  // no need to do it here

  m_coll = coll_;
  m_coll->setTrackGroups(true);
  m_cancelImageWriting = true;
  // CollectionCommand takes care of calling Controller signals
}

void Document::unAppendCollection(CollPtr coll_, FieldVec origFields_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);

  StringSet origFieldNames;
  for(FieldVec::Iterator field = origFields_.begin(); field != origFields_.end(); ++field) {
    m_coll->modifyField(field);
    origFieldNames.add(field->name());
  }

  EntryVec entries = coll_->entries();
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    // probably don't need to do this, but on the safe side...
    entry->setCollection(coll_);
  }
  m_coll->removeEntries(entries);

  // since Collection::removeField() iterates over all entries to reset the value of the field
  // don't removeField() until after removeEntry() is done
  FieldVec currFields = m_coll->fields();
  for(FieldVec::Iterator field = currFields.begin(); field != currFields.end(); ++field) {
    if(!origFieldNames.has(field->name())) {
      m_coll->removeField(field);
    }
  }
  m_coll->blockSignals(false);
}

void Document::unMergeCollection(CollPtr coll_, FieldVec origFields_, MergePair entryPair_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);

  QStringList origFieldNames;
  for(FieldVec::Iterator field = origFields_.begin(); field != origFields_.end(); ++field) {
    m_coll->modifyField(field);
    origFieldNames << field->name();
  }

  // first item in pair are the entries added by the operation, remove them
  EntryVec entries = entryPair_.first;
  m_coll->removeEntries(entries);

  // second item in pair are the entries which got modified by the original merge command
  const QString track = QString::fromLatin1("track");
  PairVector trackChanges = entryPair_.second;
  // need to go through them in reverse since one entry may have been modified multiple times
  // first item in the pair is the entry pointer
  // second item is the old value of the track field
  for(int i = trackChanges.count()-1; i >= 0; --i) {
    trackChanges[i].first->setField(track, trackChanges[i].second);
  }

  // since Collection::removeField() iterates over all entries to reset the value of the field
  // don't removeField() until after removeEntry() is done
  FieldVec currFields = m_coll->fields();
  for(FieldVec::Iterator field = currFields.begin(); field != currFields.end(); ++field) {
    if(origFieldNames.findIndex(field->name()) == -1) {
      m_coll->removeField(field);
    }
  }
  m_coll->blockSignals(false);
}

bool Document::isEmpty() const {
  //an empty doc may contain a collection, but no entries
  return (!m_coll || m_coll->entries().isEmpty());
}

bool Document::loadImage(const QString& id_) {
//  myLog() << "Document::loadImage() - id = " << id_ << endl;
  if(!m_coll) {
    return false;
  }

  bool b = !m_loadAllImages && m_validFile && m_importer && m_importer->loadImage(id_);
  if(b) {
    m_allImagesOnDisk = false;
  }
  return b;
}

bool Document::loadAllImagesNow() const {
//  myLog() << "Document::loadAllImagesNow()" << endl;
  if(!m_coll || !m_validFile) {
    return false;
  }
  if(m_loadAllImages) {
    myDebug() << "Document::loadAllImagesNow() - all valid images should already be loaded!" << endl;
    return false;
  }
  return Import::TellicoImporter::loadAllImages(m_url);
}

Tellico::Data::EntryVec Document::filteredEntries(Filter::Ptr filter_) const {
  Data::EntryVec matches;
  Data::EntryVec entries = m_coll->entries();
  for(EntryVec::Iterator it = entries.begin(); it != entries.end(); ++it) {
    if(filter_->matches(it.data())) {
      matches.append(it);
    }
  }
  return matches;
}

void Document::checkOutEntry(Data::EntryPtr entry_) {
  if(!entry_) {
    return;
  }

  const QString loaned = QString::fromLatin1("loaned");
  if(!m_coll->hasField(loaned)) {
    FieldPtr f = new Field(loaned, i18n("Loaned"), Field::Bool);
    f->setFlags(Field::AllowGrouped);
    f->setCategory(i18n("Personal"));
    m_coll->addField(f);
  }
  entry_->setField(loaned, QString::fromLatin1("true"));
  EntryVec vec;
  vec.append(entry_);
  m_coll->updateDicts(vec);
}

void Document::checkInEntry(Data::EntryPtr entry_) {
  if(!entry_) {
    return;
  }

  const QString loaned = QString::fromLatin1("loaned");
  if(!m_coll->hasField(loaned)) {
    return;
  }
  entry_->setField(loaned, QString::null);
  m_coll->updateDicts(EntryVec(entry_));
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
  Data::EntryVec entries = m_coll->entries();
  Data::FieldVec imageFields = m_coll->imageFields();
  for(Data::EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    for(Data::FieldVec::Iterator field = imageFields.begin(); field != imageFields.end() && !m_cancelImageWriting; ++field) {
      id = entry->field(field);
      if(id.isEmpty() || images.has(id)) {
        continue;
      }
      // this is the early loading, so just by calling imageById()
      // the image gets sucked from the zip file and written to disk
      //by ImageFactory::imageById()
      if(ImageFactory::imageById(id).isNull()) {
        myDebug() << "Document::slotLoadAllImages() - entry title: " << entry->title() << endl;
      }
      images.add(id);
    }
    if(m_cancelImageWriting) {
      break;
    }
    // stay responsive, do this in the background
    kapp->processEvents();
  }

  if(m_cancelImageWriting) {
    myLog() << "Document::slotLoadAllImages() - cancel image writing" << endl;
  } else {
    emit signalCollectionImagesLoaded(m_coll);
  }

  m_cancelImageWriting = false;
}

void Document::writeAllImages(int cacheDir_, const KURL& localDir_) {
  // images get 80 steps in saveDocument()
  const uint stepSize = 1 + QMAX(1, m_coll->entryCount()/80); // add 1 since it could round off
  uint j = 1;

  QString oldLocalDir = ImageFactory::localDir();
  ImageFactory::setLocalDirectory(localDir_);

  ImageFactory::CacheDir cacheDir = static_cast<ImageFactory::CacheDir>(cacheDir_);

  QString id;
  StringSet images;
  EntryVec entries = m_coll->entries();
  FieldVec imageFields = m_coll->imageFields();
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    for(FieldVec::Iterator field = imageFields.begin(); field != imageFields.end() && !m_cancelImageWriting; ++field) {
      id = entry->field(field);
      if(id.isEmpty() || images.has(id)) {
        continue;
      }
      images.add(id);
      if(ImageFactory::imageInfo(id).linkOnly) {
        continue;
      }
      if(!ImageFactory::writeCachedImage(id, cacheDir)) {
        myDebug() << "Document::writeAllImages() - did not write image for entry title: " << entry->title() << endl;
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
    myDebug() << "Document::writeAllImages() - cancel image writing" << endl;
  }

  m_cancelImageWriting = false;
  ImageFactory::setLocalDirectory(oldLocalDir);
}

bool Document::pruneImages() {
  bool found = false;
  QString id;
  StringSet images;
  Data::EntryVec entries = m_coll->entries();
  Data::FieldVec imageFields = m_coll->imageFields();
  for(Data::EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    for(Data::FieldVec::Iterator field = imageFields.begin(); field != imageFields.end(); ++field) {
      id = entry->field(field);
      if(id.isEmpty() || images.has(id)) {
        continue;
      }
      const Data::Image& img = ImageFactory::imageById(id);
      if(img.isNull()) {
        entry->setField(field, QString::null);
        found = true;
        myDebug() << "Document::pruneImages() - removing null image for " << entry->title() << ": " << id << endl;
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
  FieldVec fields = m_coll->imageFields();
  EntryVec entries = m_coll->entries();
  for(FieldVecIt f = fields.begin(); f != fields.end(); ++f) {
    for(EntryVecIt e = entries.begin(); e != entries.end(); ++e) {
      images.add(e->field(f->name()));
    }
  }
  return images.count();
}

Tellico::Data::EntryVec Document::sortEntries(EntryVec entries_) const {
  std::vector<EntryPtr> vec;
  for(EntryVecIt e = entries_.begin(); e != entries_.end(); ++e) {
    vec.push_back(e);
  }

  QStringList titles = Controller::self()->sortTitles();
  // have to go in reverse for sorting
  for(int i = titles.count()-1; i >= 0; --i) {
    if(titles[i].isEmpty()) {
      continue;
    }
    QString field = m_coll->fieldNameByTitle(titles[i]);
    std::sort(vec.begin(), vec.end(), EntryCmp(field));
  }

  Data::EntryVec sorted;
  for(std::vector<EntryPtr>::iterator it = vec.begin(); it != vec.end(); ++it) {
    sorted.append(*it);
  }
  return sorted;
}

void Document::removeImagesNotInCollection(EntryVec entries_, EntryVec entriesToKeep_) {
  // first get list of all images in collection
  StringSet images;
  FieldVec fields = m_coll->imageFields();
  EntryVec allEntries = m_coll->entries();
  for(FieldVecIt f = fields.begin(); f != fields.end(); ++f) {
    for(EntryVecIt e = allEntries.begin(); e != allEntries.end(); ++e) {
      images.add(e->field(f->name()));
    }
    for(EntryVecIt e = entriesToKeep_.begin(); e != entriesToKeep_.end(); ++e) {
      images.add(e->field(f->name()));
    }
  }

  // now for all images not in the cache, we can clear them
  StringSet imagesToCheck = ImageFactory::imagesNotInCache();

  // if entries_ is not empty, that means we want to limit the images removed
  // to those that are referenced in those entries
  StringSet imagesToRemove;
  for(FieldVecIt f = fields.begin(); f != fields.end(); ++f) {
    for(EntryVecIt e = entries_.begin(); e != entries_.end(); ++e) {
      QString id = e->field(f->name());
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

#include "document.moc"
