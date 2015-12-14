/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

// The information about the AMC file format was taken from the source code for
// GCfilms, (GPL) (c) 2005 Tian
// Monotheka, (GPL) (c) 2004, 2005 Michael Dominic K.
//                      2005 Aurelien Mino

#include "amcimporter.h"
#include "../fieldformat.h"
#include "../collections/videocollection.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <QFile>
#include <QImage>
#include <QByteArray>
#include <QApplication>

#include <limits.h>

namespace {
  static const QByteArray AMC_FILE_ID = " AMC_X.Y Ant Movie Catalog 3.5.x   www.buypin.com    www.antp.be ";
  static const quint32 AMC_MAX_STRING_SIZE = 128 * 1024;
}

using Tellico::Import::AMCImporter;

AMCImporter::AMCImporter(const QUrl& url_) : DataImporter(url_), m_cancelled(false), m_failed(false), m_majVersion(0), m_minVersion(0) {
}

AMCImporter::~AMCImporter() {
}

bool AMCImporter::canImport(int type) const {
  return type == Data::Collection::Video;
}

Tellico::Data::CollPtr AMCImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  if(!fileRef().open()) {
    return Data::CollPtr();
  }

  QIODevice* f = fileRef().file();
  m_ds.setDevice(f);
  // AMC is always little-endian? can't confirm
  m_ds.setByteOrder(QDataStream::LittleEndian);
  emit signalTotalSteps(this, f->size());

  const uint l = AMC_FILE_ID.length();
  QVector<char> buffer(l+1);
  m_ds.readRawData(buffer.data(), l);
  QString version = QString::fromLocal8Bit(buffer.data(), l);
  QRegExp versionRx(QLatin1String(".+AMC_(\\d+)\\.(\\d+).+"));
  if(versionRx.indexIn(version) == -1) {
    myDebug() << "no file id match";
    return Data::CollPtr();
  }

  m_coll = new Data::VideoCollection(true);

  m_majVersion = versionRx.cap(1).toInt();
  m_minVersion = versionRx.cap(2).toInt();
//  myDebug() << m_majVersion << "-" << m_minVersion;

  readString(); // name
  readString(); // email
  if(m_majVersion <= 3 && m_minVersion < 5) {
    readString(); // icq
  }
  readString(); // webpage
  readString(); // description

  const bool showProgress = options() & ImportProgress;

  while(!m_cancelled && !m_failed && !f->atEnd()) {
    readEntry();
    if(showProgress) {
      emit signalProgress(this, f->pos());
      qApp->processEvents();
    }
  }

  return m_coll;
}

bool AMCImporter::readBool() {
  quint8 b;
  m_ds >> b;
  return b;
}

quint32 AMCImporter::readInt() {
  if(m_failed) {
    return 0;
  }
  quint32 i;
  m_ds >> i;
  if(i >= UINT_MAX) {
    i = 0;
  }
  return i;
}

QString AMCImporter::readString() {
  if(m_failed) {
    return QString();
  }
  // The serialization format is a length specifier first, then l bytes of data
  quint32 l = readInt();
  if(l == 0) {
    return QString();
  }
  if(l > AMC_MAX_STRING_SIZE) {
    myDebug() << "string is too long:" << l;
    m_failed = true;
    return QString();
  }
  QVector<char> buffer(l+1);
  m_ds.readRawData(buffer.data(), l);
  QString s = QString::fromLocal8Bit(buffer.data(), l);
//  myDebug() << "string: " << s;
  return s;
}

QString AMCImporter::readImage(const QString& format_) {
  if(m_failed) {
    return QString();
  }
  quint32 l = readInt();
  if(l == 0) {
    return QString();
  }
  if(l > AMC_MAX_STRING_SIZE) {
    myDebug() << "string is too long:" << l;
    m_failed = true;
    return QString();
  }
  QVector<char> buffer(l+1);
  m_ds.readRawData(buffer.data(), l);
  QByteArray bytes;
  bytes.reserve(l);
  qCopy(buffer.data(), buffer.data() + l, bytes.begin());
  QImage img = QImage::fromData(bytes);
  if(img.isNull()) {
    myDebug() << "null image";
    return QString();
  }
  QString format = QLatin1String("PNG");
  if(format_ == QLatin1String(".jpg")) {
    format = QLatin1String("JPEG");
  } else if(format_ == QLatin1String(".gif")) {
    format = QLatin1String("GIF");
  }
  return ImageFactory::addImage(img, format);
}

void AMCImporter::readEntry() {
  Data::EntryPtr e(new Data::Entry(m_coll));

  quint32 id = readInt();
  if(id > 0) {
    e->setId(id);
  }
  readInt(); // add date

  quint32 rating = readInt();
  if(m_majVersion >= 3 && m_minVersion >= 5) {
    rating /= 10;
  }
  e->setField(QLatin1String("rating"), QString::number(rating));
  quint32 year = readInt();
  if(year > 0) {
    e->setField(QLatin1String("year"), QString::number(year));
  }
  quint32 time = readInt();
  if(time > 0) {
    e->setField(QLatin1String("running-time"), QString::number(time));
  }

  readInt(); // video bitrate
  readInt(); // audio bitrate
  readInt(); // number of files
  readBool(); // checked
  readString(); // media label
  e->setField(QLatin1String("medium"), readString());
  readString(); // source
  readString(); // borrower
  QString s = readString(); // title
  if(!s.isEmpty()) {
    e->setField(QLatin1String("title"), s);
  }
  QString s2 = readString(); // translated title
  if(s.isEmpty()) {
    e->setField(QLatin1String("title"), s2);
  }

  e->setField(QLatin1String("director"), readString());
  s = readString();
  QRegExp roleRx(QLatin1String("(.+) \\(([^(]+)\\)"));
  roleRx.setMinimal(true);
  if(roleRx.indexIn(s) > -1) {
    QString role = roleRx.cap(2).toLower();
    if(role == QLatin1String("story") || role == QLatin1String("written by")) {
      e->setField(QLatin1String("writer"), roleRx.cap(1));
    } else {
      e->setField(QLatin1String("producer"), s);
    }
  } else {
    e->setField(QLatin1String("producer"), s);
  }
  e->setField(QLatin1String("nationality"), readString());
  e->setField(QLatin1String("genre"), readString().replace(QLatin1String(", "), FieldFormat::delimiterString()));

  e->setField(QLatin1String("cast"), parseCast(readString()).join(FieldFormat::rowDelimiterString()));

  readString(); // url
  e->setField(QLatin1String("plot"), readString());
  e->setField(QLatin1String("comments"), readString());
  s = readString(); // video format
  QRegExp regionRx(QLatin1String("Region \\d"));
  if(regionRx.indexIn(s) > -1) {
    e->setField(QLatin1String("region"), regionRx.cap(0));
  }
  e->setField(QLatin1String("audio-track"), readString()); // audio format
  readString(); // resolution
  readString(); // frame rate
  e->setField(QLatin1String("language"), readString()); // audio language
  e->setField(QLatin1String("subtitle"), readString()); // subtitle
  readString(); // file size
  s = readString(); // picture extension
  s = readImage(s); // picture
  if(!s.isEmpty()) {
    e->setField(QLatin1String("cover"), s);
  }

  m_coll->addEntries(e);
}

QStringList AMCImporter::parseCast(const QString& text_) {
  QStringList cast;
  int nPar = 0;
  QRegExp castRx(QLatin1String("[,()]"));
  QString person, role;
  int oldPos = 0;
  for(int pos = castRx.indexIn(text_); pos > -1; pos = castRx.indexIn(text_, pos+1)) {
    if(text_.at(pos) == QLatin1Char(',') && nPar%2 == 0) {
      // we're done with this one
      person += text_.mid(oldPos, pos-oldPos).trimmed();
      QString all = person;
      if(!role.isEmpty()) {
        if(role.startsWith(QLatin1String("as "))) {
          role = role.mid(3);
        }
        all += FieldFormat::columnDelimiterString() + role;
      }
      cast << all;
      person.clear();
      role.clear();
      oldPos = pos+1; // add one to go past comma
    } else if(text_.at(pos) == QLatin1Char('(')) {
      if(nPar == 0) {
        person = text_.mid(oldPos, pos-oldPos).trimmed();
        oldPos = pos+1; // add one to go past parenthesis
      }
      ++nPar;
    } else if(text_.at(pos) == QLatin1Char(')')) {
      --nPar;
      if(nPar == 0) {
        role = text_.mid(oldPos, pos-oldPos).trimmed();
        oldPos = pos+1; // add one to go past parenthesis
      }
    }
  }
  // grab the last one
  if(nPar%2 == 0) {
    int pos = text_.length();
    person += text_.mid(oldPos, pos-oldPos).trimmed();
    QString all = person;
    if(!role.isEmpty()) {
      if(role.startsWith(QLatin1String("as "))) {
        role = role.mid(3);
      }
      all += FieldFormat::columnDelimiterString() + role;
    }
    cast << all;
  }
  return cast;
}

void AMCImporter::slotCancel() {
  m_cancelled = true;
}
