/***************************************************************************
    Copyright (C) 2008-2020 Robby Stephenson <robby@periapsis.org>
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
#include "tellicoxmlreader.h"
#include "tellico_xml.h"
#include "../images/imagefactory.h"
#include "../core/tellico_strings.h"
#include "../utils/guiproxy.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KZip>

#include <QBuffer>
#include <QFile>
#include <QTimer>
#include <QApplication>
#include <QPointer>

namespace {
  static const int MIN_BLOCK_SIZE = 100*1024; // minimum read size of 100 kB
}

using Tellico::Import::TellicoImporter;

TellicoImporter::TellicoImporter(const QUrl& url_, bool loadAllImages_) : DataImporter(url_),
    m_loadAllImages(loadAllImages_), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(nullptr), m_zip(nullptr), m_imgDir(nullptr) {
}

TellicoImporter::TellicoImporter(const QString& text_) : DataImporter(text_),
    m_loadAllImages(true), m_format(Unknown), m_modified(false),
    m_cancelled(false), m_hasImages(false), m_buffer(nullptr), m_zip(nullptr), m_imgDir(nullptr) {
}

TellicoImporter::~TellicoImporter() = default;

Tellico::Data::CollPtr TellicoImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  auto getFirstData = [this] {
    if(source() == URL && fileRef().open()) {
      return fileRef().file()->peek(5);
    } else {
      return QByteArray(data().constData(), 6);
    }
  };
  const auto s = getFirstData(); // read first 5 characters
  if(s.size() < 5) {
    m_format = Error;
    return Data::CollPtr();
  }

  // hack for processEvents and deletion
  QPointer<TellicoImporter> thisPtr(this);

  // need to decide if the data is xml text, or a zip file
  // if the first 5 characters are <?xml then treat it like text
  if(s[0] == '<' && s[1] == '?' && s[2] == 'x' && s[3] == 'm' && s[4] == 'l') {
    m_format = XML;
    loadXMLData(source() == URL ? fileRef().file()->readAll() : data(), true);
  } else {
    m_format = Zip;
    loadZipData();
  }
  return thisPtr ? m_coll : Data::CollPtr();
}

void TellicoImporter::loadXMLData(const QByteArray& data_, bool loadImages_) {
  const bool showProgress = options() & ImportProgress;

  TellicoXmlReader reader(m_baseUrl);
  reader.setLoadImages(loadImages_);
  reader.setShowImageLoadErrors(options() & ImportShowImageErrors);
  reader.setImagePathsAsLinks(options() & ImportImagesAsLinks);
  bool success = true;

  const int blockSize = qMax(data_.size()/100 + 1, MIN_BLOCK_SIZE);
  int pos = 0;
  Q_EMIT signalTotalSteps(this, data_.size());

  // hack to allow processEvents
  QPointer<TellicoImporter> thisPtr(this);
  while(thisPtr && success && !m_cancelled && pos < data_.size()) {
    const uint size = qMin(blockSize, data_.size() - pos);
    const QByteArray block = QByteArray::fromRawData(data_.data() + pos, size);
    success = reader.readNext(block);
    if(!success && reader.isNotWellFormed()) {
      // could be bug 418067 where version of Tellico < 3.3 could use invalid XML names
      // try to recover. If it's not a bad field name, this should be a pretty quick check
      myDebug() << "XML parsing failed. Attempting to recover.";
      QByteArray newData = XML::recoverFromBadXMLName(data_);
      if(newData.length() == data_.length()) {
        // might be bug 443845 with invalid XML control characters
        newData = XML::removeInvalidXml(data_);
      }
      if(newData.length() < data_.length()) {
        myDebug() << "Reloading the XML data.";
        loadXMLData(newData, loadImages_);
        return;
      }
    }
    pos += blockSize;
    if(thisPtr && showProgress) {
      Q_EMIT signalProgress(this, pos);
      qApp->processEvents();
    }
  }
  if(!thisPtr) {
    return;
  }

  if(!success) {
    m_format = Error;
    QString error;
    if(!url().isEmpty()) {
      error = TC_I18N2(errorLoad, url().fileName());
    }
    const QString errorString = reader.errorString();
    if(!errorString.isEmpty()) {
      error += QStringLiteral("\n") + errorString;
    }
    myDebug() << error;
    setStatusMessage(error);
    return;
  }

  if(!m_cancelled) {
    m_hasImages = reader.hasImages();
    m_coll = reader.collection();
  }
}

void TellicoImporter::loadZipData() {
  std::unique_ptr<KZip> zip;
  std::unique_ptr<QBuffer> buffer;
  if(source() == URL) {
    buffer.reset();
    zip.reset(new KZip(fileRef().fileName()));
  } else {
    myDebug() << "Attempting to read text as a zip file";
    return;
  }
  if(!zip->open(QIODevice::ReadOnly)) {
    setStatusMessage(TC_I18N2(errorLoad, url().fileName()));
    m_format = Error;
    return;
  }

  const KArchiveDirectory* dir = zip->directory();
  if(!dir) {
    QString str = TC_I18N2(errorLoad, url().fileName()) + QLatin1Char('\n');
    str += i18n("The file is empty.");
    setStatusMessage(str);
    m_format = Error;
    return;
  }

  // main file was changed from bookcase.xml to tellico.xml as of version 0.13
  const KArchiveEntry* entry = dir->entry(QStringLiteral("tellico.xml"));
  if(!entry) {
    entry = dir->entry(QStringLiteral("bookcase.xml"));
  }
  if(!entry || !entry->isFile()) {
    QString str = TC_I18N2(errorLoad, url().fileName()) + QLatin1Char('\n');
    str += i18n("The file contains no collection data.");
    setStatusMessage(str);
    m_format = Error;
    return;
  }

  const QByteArray xmlData = static_cast<const KArchiveFile*>(entry)->data();
  // hack to account for processEvents and deletion
  QPointer<TellicoImporter> thisPtr(this);
  loadXMLData(xmlData, false);
  if(!thisPtr) {
    return;
  }
  if(!m_coll) {
    m_format = Error;
    return;
  }

  if(m_cancelled) {
    return;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QStringLiteral("images"));
  if(!imgDirEntry || !imgDirEntry->isDirectory()) {
    return;
  }

  // past the point of dropping errors, so retain ownership of the objects
  m_zip = std::move(zip);
  m_buffer = std::move(buffer);

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
      qApp->processEvents();
    }
  }

  if(thisPtr && m_images.isEmpty()) {
    // give it some time
    QTimer::singleShot(3000, this, &QObject::deleteLater);
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
    QTimer::singleShot(3000, this, &QObject::deleteLater);
  }
  return !newID.isEmpty();
}

std::unique_ptr<KZip> TellicoImporter::takeImages() {
  return std::move(m_zip);
}

void TellicoImporter::slotCancel() {
  m_cancelled = true;
  m_format = Cancel;
}

// static
bool TellicoImporter::loadAllImages(const QUrl& url_) {
  // only local files are allowed
  if(url_.isEmpty() || !url_.isValid() || !url_.isLocalFile()) {
//    myDebug() << "returning";
    return false;
  }

  // keep track of url for error reporting
  static QUrl u;

  KZip zip(url_.path());
  if(!zip.open(QIODevice::ReadOnly)) {
    if(u != url_) {
      GUI::Proxy::sorry(TC_I18N2(errorImageLoad, url_.fileName()));
    }
    u = url_;
    return false;
  }

  const KArchiveDirectory* dir = zip.directory();
  if(!dir) {
    if(u != url_) {
      GUI::Proxy::sorry(TC_I18N2(errorImageLoad, url_.fileName()));
    }
    u = url_;
    return false;
  }

  const KArchiveEntry* imgDirEntry = dir->entry(QStringLiteral("images"));
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
