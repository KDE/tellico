/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

//  before tellicosaxmporter.h because of QT_NO_CAST_ASCII issues
#include "tellicoxmlhandler.h"
#include "tellicosaximporter.h"
#include "tellico_xml.h"
#include "../collectionfactory.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../imagefactory.h"
#include "../image.h"
#include "../isbnvalidator.h"
#include "../tellico_strings.h"
#include "../tellico_kernel.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"
#include "../progressmanager.h"

#include <klocale.h>
#include <kcodecs.h>
#include <kzip.h>
#include <kapplication.h>

#include <QBuffer>
#include <QFile>
#include <QTimer>

using Tellico::Import::TellicoSaxImporter;

TellicoSaxImporter::TellicoSaxImporter(const KUrl& url_, bool loadAllImages_) : DataImporter(url_),
    m_loadAllImages(loadAllImages_), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(0), m_zip(0), m_imgDir(0) {
}

TellicoSaxImporter::TellicoSaxImporter(const QString& text_) : DataImporter(text_),
    m_loadAllImages(true), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(0), m_zip(0), m_imgDir(0) {
}

TellicoSaxImporter::~TellicoSaxImporter() {
  if(m_zip) {
    m_zip->close();
  }
  delete m_zip;
  m_zip = 0;
  delete m_buffer;
  m_buffer = 0;
}

Tellico::Data::CollPtr TellicoSaxImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  QByteArray s; // read first 5 characters
  if(source() == URL) {
    if(!fileRef().open()) {
      return Data::CollPtr();
    }
    QIODevice* f = fileRef().file();
    for(int i = 0; i < 5; ++i) {
      char c;
      s += f->getChar(&c);
    }
    f->reset();
  } else {
    if(data().size() < 5) {
      m_format = Error;
      return Data::CollPtr();
    }
    s = QByteArray(data(), 6);
  }


  // need to decide if the data is xml text, or a zip file
  // if the first 5 characters are <?xml then treat it like text
  if(s[0] == '<' && s[1] == '?' && s[2] == 'x' && s[3] == 'm' && s[4] == 'l') {
    m_format = XML;
    loadXMLData(source() == URL ? fileRef().file()->readAll() : data(), true);
  } else {
    m_format = Zip;
    loadZipData();
  }
  return m_coll;
}

void TellicoSaxImporter::loadXMLData(const QByteArray& data_, bool loadImages_) {
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(data_.size());
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  const bool showProgress = options() & ImportProgress;

  TellicoXMLHandler handler;
  handler.setLoadImages(loadImages_);

  QXmlSimpleReader reader;
  reader.setContentHandler(&handler);

  QXmlInputSource source;
  bool success = reader.parse(&source, true);

  const int blockSize = data_.size()/100 + 1;
  int pos = 0;

  while(success && !m_cancelled && pos < data_.size()) {
    uint size = qMin(blockSize, data_.size() - pos);
    QByteArray block = QByteArray::fromRawData(data_.data() + pos, size);
    source.setData(block);
    success = reader.parseContinue();
    pos += blockSize;
    if(showProgress) {
      ProgressManager::self()->setProgress(this, pos);
      kapp->processEvents();
    }
  }

  if(!success) {
    m_format = Error;
    QString error;
    if(!url().isEmpty()) {
      error = i18n(errorLoad).arg(url().fileName()) + QChar('\n');
    }
    error += handler.errorString();
    myDebug() << error << endl;
    setStatusMessage(error);
    return;
  }

  if(!m_cancelled) {
    m_hasImages = handler.hasImages();
    m_coll = handler.collection();
  }
}

void TellicoSaxImporter::loadZipData() {
  delete m_buffer;
  delete m_zip;
  if(source() == URL) {
    m_buffer = 0;
    m_zip = new KZip(fileRef().fileName());
  } else {
    QByteArray allData = data();
    m_buffer = new QBuffer(&allData);
    m_zip = new KZip(m_buffer);
  }
  if(!m_zip->open(QIODevice::ReadOnly)) {
    setStatusMessage(i18n(errorLoad, url().fileName()));
    m_format = Error;
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const KArchiveDirectory* dir = m_zip->directory();
  if(!dir) {
    QString str = i18n(errorLoad, url().fileName()) + QChar('\n');
    str += i18n("The file is empty.");
    setStatusMessage(str);
    m_format = Error;
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  // main file was changed from bookcase.xml to tellico.xml as of version 0.13
  const KArchiveEntry* entry = dir->entry(QLatin1String("tellico.xml"));
  if(!entry) {
    entry = dir->entry(QLatin1String("bookcase.xml"));
  }
  if(!entry || !entry->isFile()) {
    QString str = i18n(errorLoad, url().fileName()) + QChar('\n');
    str += i18n("The file contains no collection data.");
    setStatusMessage(str);
    m_format = Error;
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const QByteArray xmlData = static_cast<const KArchiveFile*>(entry)->data();
  loadXMLData(xmlData, false);
  if(!m_coll) {
    m_format = Error;
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  if(m_cancelled) {
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QLatin1String("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    m_zip->close();
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }
  m_imgDir = static_cast<const KArchiveDirectory*>(imgDirEntry);
  m_images.clear();
  m_images.add(m_imgDir->entries());
  m_hasImages = !m_images.isEmpty();

  // if all the images are not to be loaded, then we're done
  if(!m_loadAllImages) {
//    myLog() << "TellicoSaxImporter::loadZipData() - delayed loading for " << m_images.count() << " images" << endl;
    return;
  }

  const QStringList images = static_cast<const KArchiveDirectory*>(imgDirEntry)->entries();
  const uint stepSize = qMax(s_stepSize, static_cast<uint>(images.count())/100);

  uint j = 0;
  for(QStringList::ConstIterator it = images.begin(); !m_cancelled && it != images.end(); ++it, ++j) {
    const KArchiveEntry* file = m_imgDir->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section('.', -1).toUpper(), (*it));
      m_images.remove(*it);
    }
    if(j%stepSize == 0) {
      kapp->processEvents();
    }
  }

  if(m_images.isEmpty()) {
    // give it some time
    QTimer::singleShot(3000, this, SLOT(deleteLater()));
  }
}

bool TellicoSaxImporter::hasImages() const {
  return m_hasImages;
}

bool TellicoSaxImporter::loadImage(const QString& id_) {
//  myLog() << "TellicoSaxImporter::loadImage() - id =  " << id_ << endl;
  if(m_format != Zip || !m_imgDir) {
    return false;
  }
  const KArchiveEntry* file = m_imgDir->entry(id_);
  if(!file || !file->isFile()) {
    return false;
  }
  QString newID = ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                                         id_.section('.', -1).toUpper(), id_);
  m_images.remove(id_);
  if(m_images.isEmpty()) {
    // give it some time
    QTimer::singleShot(3000, this, SLOT(deleteLater()));
  }
  return !newID.isEmpty();
}

void TellicoSaxImporter::slotCancel() {
  m_cancelled = true;
  m_format = Cancel;
}

#include "tellicosaximporter.moc"
