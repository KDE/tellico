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
    m_fileFormat(Import::TellicoImporter::Unknown), m_allImagesOnDisk(!Config::writeImagesInFile()) {
  newDocument(Collection::Book);
}

Document::~Document() {
  delete m_importer;
  m_importer = 0;
}

Tellico::Data::CollPtr Document::collection() const {
  return m_coll;
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
  if(!m_importer->hasImages()) {
    m_allImagesOnDisk = true;
  }
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
    QTimer::singleShot(500, this, SLOT(slotWriteAllImages()));
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

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, i18n("Saving file..."), false);
  item.setTotalSteps(100);
  ProgressItem::Done done(this);

  // will always save as zip file, no matter if has images or not
  bool includeImages = Config::writeImagesInFile();
  // write all images to disk cache if needed
  // have to do this before executing exporter in case
  // the user changed the imageInFile setting from Yes to No, in which
  // case saving will over write the old file that has the images in it!
  if(!includeImages) {
    m_cancelImageWriting = false;
    slotWriteAllImages(ImageFactory::DataDir);
  }
  ProgressManager::self()->setProgress(this, 80);
  Export::Exporter* exporter;
  if(m_fileFormat == Import::TellicoImporter::XML) {
    exporter = new Export::TellicoXMLExporter();
    static_cast<Export::TellicoXMLExporter*>(exporter)->setIncludeImages(includeImages);
  } else {
    exporter = new Export::TellicoZipExporter();
    static_cast<Export::TellicoZipExporter*>(exporter)->setIncludeImages(includeImages);
  }
  exporter->setEntries(m_coll->entries());
  exporter->setURL(url_);
  // since we already asked about overwriting the file, force the save
  exporter->setOptions(exporter->options() | Export::ExportForce | Export::ExportProgress);
  bool success = exporter->exec();
  ProgressManager::self()->setProgress(this, 90);

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
    m_coll->addEntry(newEntry);
  }
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

  // FIXME: find a faster way than one-to-one comparison
  // music collection get a special case because of the audio file metadata might
  // be read on a per-track basis, allow merging of entries with identical titles and artists
  bool isMusic = (m_coll->type() == Collection::Album);
  const QString sep = QString::fromLatin1("; ");
  const QString sep2 = QString::fromLatin1("::");
  const QString artist = QString::fromLatin1("artist");
  const QString track = QString::fromLatin1("track");

  bool matches = false;
  EntryVec currEntries = m_coll->entries();
  EntryVec::Iterator currIt = currEntries.begin();
  EntryVec entries = coll_->entries();
  for(EntryVec::Iterator newIt = entries.begin(); newIt != entries.end(); ++newIt) {
    matches = false;
    for(currIt = currEntries.begin(); currIt != currEntries.end(); ++currIt) {
      if(isMusic && !newIt->title().isEmpty() && !newIt->field(artist).isEmpty()
          && newIt->title() == currIt->title() && newIt->field(artist) == currIt->field(artist)) {
        matches = true;
        // trackList 1 is collection to be merged, trackList2 is current collection
        QStringList newTracks = Data::Field::split(newIt->field(track), true);
        QStringList currTracks = Data::Field::split(currIt->field(track), true);
        for(uint idx = 0; idx < newTracks.count(); ++idx) {
          // only add track if current track id is empty or shorter
          if(newTracks[idx].isEmpty()) {
            continue;
          }
          while(currTracks.count() <= idx) {
            currTracks += QString();
          }
          if(currTracks[idx].isEmpty()) {
            currTracks[idx] = newTracks[idx];
          } else {
            // we could add artist or length
            QStringList currParts = QStringList::split(sep2, currTracks[idx], true);
            QStringList newParts = QStringList::split(sep2, newTracks[idx], true);
            bool addedPart = false;
            while(currParts.count() < newParts.count()) {
              currParts += QString();
            }
            for(uint i = 1; i < newParts.count(); ++i) {
              if(currParts[i].isEmpty()) {
                currParts[i] = newParts[i];
                addedPart = true;
              }
            }
            if(addedPart) {
              currTracks[idx] = currParts.join(sep2);
            }
          }
        }
        // the merge item is the entry pointer and the old track value
        pair.second.append(qMakePair(EntryPtr(currIt), currIt->field(track)));
        currIt->setField(track, currTracks.join(sep));
      } else if(*currIt == *newIt) {
        matches = true;
        break;
      }
    }
    if(!matches) {
      Data::EntryPtr e = new Data::Entry(*newIt);
      e->setCollection(m_coll);
      m_coll->addEntry(e);
      // keep track of which entries got added
      pair.first.append(e);
    }
  }
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

  // because of the shared ptr mix, have to clear collection
  if(m_coll) {
    m_coll->clear();
  }
  m_coll = coll_;
  m_cancelImageWriting = true;
  // CollectionCommand takes care of calling Controller signals
}

void Document::unAppendCollection(CollPtr coll_, FieldVec origFields_) {
  if(!coll_) {
    return;
  }

  m_coll->blockSignals(true);

  QStringList origFieldNames;
  for(FieldVec::Iterator field = origFields_.begin(); field != origFields_.end(); ++field) {
    m_coll->modifyField(field);
    origFieldNames << field->name();
  }

  EntryVec entries = coll_->entries();
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    // probably don't need to do this, but on the safe side...
    entry->setCollection(coll_);
    m_coll->removeEntry(entry);
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
  for(EntryVec::Iterator entry = entries.begin(); entry != entries.end(); ++entry) {
    m_coll->removeEntry(entry);
  }

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
//  kdDebug() << "Document::loadImage() - id = " << id_ << endl;
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
  EntryVec vec;
  vec.append(entry_);
  m_coll->updateDicts(vec);
}

void Document::renameCollection(const QString& newTitle_) {
  m_coll->setTitle(newTitle_);
}

void Document::slotWriteAllImages(int cacheDir_) {
  // images get 80 steps
  const uint stepSize = 1 + QMAX(1, m_coll->entryCount()/80); // add 1 since it could round off
  uint j = 1;

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
      if(ImageFactory::writeCachedImage(id, static_cast<ImageFactory::CacheDir>(cacheDir_))) {
        images.add(id);
      } else {
        myDebug() << "Document::slotWriteAllImages() - entry title: " << entry->title() << endl;
      }
    }
    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j/stepSize);
      kapp->processEvents();
    }
    ++j;
  }

  m_cancelImageWriting = false;
}

bool Document::pruneImages() {
  DEBUG_BLOCK;
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

void Document::removeImagesNotInCollection(Data::EntryVec entries_) {
  // first get list of all images in collection
  StringSet images;
  FieldVec fields = m_coll->imageFields();
  EntryVec allEntries = m_coll->entries();
  for(FieldVecIt f = fields.begin(); f != fields.end(); ++f) {
    for(EntryVecIt e = allEntries.begin(); e != allEntries.end(); ++e) {
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
      if(!id.isEmpty() && imagesToCheck.has(id)) {
        imagesToRemove.add(id);
      }
    }
  }

  const QStringList realImagesToRemove = imagesToRemove.toList();
  for(QStringList::ConstIterator it = realImagesToRemove.begin(); it != realImagesToRemove.end(); ++it) {
    if(!images.has(*it)) {
      ImageFactory::removeImage(*it, false); // doesn't delete, just remove link
    }
  }
}

#include "document.moc"
