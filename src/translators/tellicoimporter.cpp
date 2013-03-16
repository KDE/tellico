/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellicoimporter.h"
#include "tellicoxmlhandler.h"
#include "tellico_xml.h"
#include "../collectionfactory.h"
#include "../entry.h"
#include "../field.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../utils/isbnvalidator.h"
#include "../core/tellico_strings.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kzip.h>
#include <kapplication.h>

#include <QBuffer>
#include <QFile>
#include <QTimer>

using Tellico::Import::TellicoImporter;

TellicoImporter::TellicoImporter(const KUrl& url_, bool loadAllImages_) : DataImporter(url_),
    m_loadAllImages(loadAllImages_), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(0), m_zip(0), m_imgDir(0) {
}

TellicoImporter::TellicoImporter(const QString& text_) : DataImporter(text_),
    m_loadAllImages(true), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(0), m_zip(0), m_imgDir(0) {
}

TellicoImporter::~TellicoImporter() {
  delete m_zip;
  m_zip = 0;
  delete m_buffer;
  m_buffer = 0;
}

Tellico::Data::CollPtr TellicoImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  QByteArray s; // read first 5 characters
  if(source() == URL) {
    if(!fileRef().open()) {
      return Data::CollPtr();
    }
    QIODevice* f = fileRef().file();
    char c;
    for(int i = 0; i < 5; ++i) {
      if(f->getChar(&c)) {
        s += c;
      }
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

void TellicoImporter::loadXMLData(const QByteArray& data_, bool loadImages_) {
  const bool showProgress = options() & ImportProgress;

  TellicoXMLHandler handler;
  handler.setLoadImages(loadImages_);
  handler.setShowImageLoadErrors(options() & ImportShowImageErrors);

  QXmlSimpleReader reader;
  reader.setContentHandler(&handler);

  QXmlInputSource source;
  source.setData(QByteArray()); // necessary
  bool success = reader.parse(&source, true);

  const int blockSize = data_.size()/100 + 1;
  int pos = 0;
  emit signalTotalSteps(this, data_.size());

  while(success && !m_cancelled && pos < data_.size()) {
    uint size = qMin(blockSize, data_.size() - pos);
    QByteArray block = QByteArray::fromRawData(data_.data() + pos, size);
    source.setData(block);
    success = reader.parseContinue();
    pos += blockSize;
    if(showProgress) {
      emit signalProgress(this, pos);
      kapp->processEvents();
    }
  }
  qDebug() << "done reading xml";

  if(!success) {
    m_format = Error;
    QString error;
    if(!url().isEmpty()) {
      error = i18n(errorLoad).arg(url().fileName()) + QLatin1Char('\n');
    }
    error += handler.errorString();
    myDebug() << error;
    setStatusMessage(error);
    return;
  }

  qDebug() << "cancelled?" << m_cancelled;
  if(!m_cancelled) {
    m_hasImages = handler.hasImages();
    qDebug() << "Grabbing handler collection";
    m_coll = handler.collection();
  }
  qDebug() << "done with xml";
}

void TellicoImporter::loadZipData() {
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
    QString str = i18n(errorLoad, url().fileName()) + QLatin1Char('\n');
    str += i18n("The file is empty.");
    setStatusMessage(str);
    m_format = Error;
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
    QString str = i18n(errorLoad, url().fileName()) + QLatin1Char('\n');
    str += i18n("The file contains no collection data.");
    setStatusMessage(str);
    m_format = Error;
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
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  if(m_cancelled) {
    delete m_zip;
    m_zip = 0;
    delete m_buffer;
    m_buffer = 0;
    return;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QLatin1String("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
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
//    myLog() << "delayed loading for " << m_images.count() << " images";
    return;
  }

  const QStringList images = m_imgDir->entries();
  const uint stepSize = qMax(s_stepSize, static_cast<uint>(images.count())/100);

  uint j = 0;
  for(QStringList::ConstIterator it = images.begin(); !m_cancelled && it != images.end(); ++it, ++j) {
    const KArchiveEntry* file = m_imgDir->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section(QLatin1Char('.'), -1).toUpper(), (*it));
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

bool TellicoImporter::hasImages() const {
  return m_hasImages;
}

bool TellicoImporter::loadImage(const QString& id_) {
//  myLog() << "id =  " << id_;
  if(m_format != Zip || !m_imgDir) {
    return false;
  }
  const KArchiveEntry* file = m_imgDir->entry(id_);
  if(!file || !file->isFile()) {
    return false;
  }
  QString newID = ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                                         id_.section(QLatin1Char('.'), -1).toUpper(), id_);
  m_images.remove(id_);
  if(m_images.isEmpty()) {
    // give it some time
    QTimer::singleShot(3000, this, SLOT(deleteLater()));
  }
  return !newID.isEmpty();
}

KZip* TellicoImporter::takeImages() {
  KZip* zip = m_zip;
  m_zip = 0;
  return zip;
}

void TellicoImporter::slotCancel() {
  m_cancelled = true;
  m_format = Cancel;
}

// static
bool TellicoImporter::loadAllImages(const KUrl& url_) {
  // only local files are allowed
  if(url_.isEmpty() || !url_.isValid() || !url_.isLocalFile()) {
//    myDebug() << "returning";
    return false;
  }

  // keep track of url for error reporting
  static KUrl u;

  KZip zip(url_.path());
  if(!zip.open(QIODevice::ReadOnly)) {
    if(u != url_) {
      GUI::Proxy::sorry(i18n(errorImageLoad, url_.fileName()));
    }
    u = url_;
    return false;
  }

  const KArchiveDirectory* dir = zip.directory();
  if(!dir) {
    if(u != url_) {
      GUI::Proxy::sorry(i18n(errorImageLoad, url_.fileName()));
    }
    u = url_;
    return false;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QLatin1String("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    return false;
  }
  const QStringList images = static_cast<const KArchiveDirectory*>(imgDirEntry)->entries();
  for(QStringList::ConstIterator it = images.begin(); it != images.end(); ++it) {
    const KArchiveEntry* file = static_cast<const KArchiveDirectory*>(imgDirEntry)->entry(*it);
    if(file && file->isFile()) {
      ImageFactory::addImage(static_cast<const KArchiveFile*>(file)->data(),
                             (*it).section(QLatin1Char('.'), -1).toUpper(), (*it));
    }
  }
  return true;
}

#include "tellicoimporter.moc"
