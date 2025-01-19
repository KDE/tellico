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

#include <QImage>
#include <QByteArray>
#include <QIODevice>
#include <QApplication>

#include <limits>
#include <algorithm>

#define AMC_FILE_ID " AMC_X.Y Ant Movie Catalog 3.5.x   www.buypin.com    www.antp.be "

namespace {
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
  Q_EMIT signalTotalSteps(this, f->size());

  const uint l = sizeof(AMC_FILE_ID)-1;
  QVector<char> buffer(l+1);
  m_ds.readRawData(buffer.data(), l);
  QString version = QString::fromLocal8Bit(buffer.data(), l);
  static const QRegularExpression versionRx(QLatin1String(".+AMC_(\\d+)\\.(\\d+).+"));
  QRegularExpressionMatch versionMatch = versionRx.match(version);
  if(!versionMatch.hasMatch()) {
    myDebug() << "no file id match";
    return Data::CollPtr();
  }

  m_coll = new Data::VideoCollection(true);

  m_majVersion = versionMatch.captured(1).toInt();
  m_minVersion = versionMatch.captured(2).toInt();
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
      Q_EMIT signalProgress(this, f->pos());
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
  if(i >= std::numeric_limits<uint>::max()) {
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
  std::copy(buffer.data(), buffer.data() + l, bytes.begin());
  QImage img = QImage::fromData(bytes);
  if(img.isNull()) {
    static uint count = 0;
    ++count;
    if(count < 5) {
      myDebug() << "AMCImporter::readImage() - null image, expected" << format_ << "from" << l << "bytes";
    } else if(count == 6) {
      myDebug() << "AMCImporter::readImage() - skipping further errors for null images";
    }
    return QString();
  }
  QString newFormat;
  if(format_ == QLatin1String(".jpg")) {
    newFormat = QStringLiteral("JPEG");
  } else if(format_ == QLatin1String(".gif")) {
    newFormat = QStringLiteral("GIF");
  } else {
    newFormat = QStringLiteral("PNG");
  }
  return ImageFactory::addImage(img, newFormat);
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
  e->setField(QStringLiteral("rating"), QString::number(rating));
  quint32 year = readInt();
  if(year > 0) {
    e->setField(QStringLiteral("year"), QString::number(year));
  }
  quint32 time = readInt();
  if(time > 0) {
    e->setField(QStringLiteral("running-time"), QString::number(time));
  }

  readInt(); // video bitrate
  readInt(); // audio bitrate
  readInt(); // number of files
  readBool(); // checked
  readString(); // media label
  e->setField(QStringLiteral("medium"), readString());
  readString(); // source
  readString(); // borrower
  QString s = readString(); // title
  if(!s.isEmpty()) {
    e->setField(QStringLiteral("title"), s);
  }
  QString s2 = readString(); // translated title
  if(s.isEmpty()) {
    e->setField(QStringLiteral("title"), s2);
  }

  e->setField(QStringLiteral("director"), readString());
  s = readString();
  static const QRegularExpression roleRx(QLatin1String("(.+?) \\(([^(]+)\\)"));
  QRegularExpressionMatch roleMatch = roleRx.match(s);
  if(roleMatch.hasMatch()) {
    QString role = roleMatch.captured(2).toLower();
    if(role == QLatin1String("story") || role == QLatin1String("written by")) {
      e->setField(QStringLiteral("writer"), roleMatch.captured(1));
    } else {
      e->setField(QStringLiteral("producer"), s);
    }
  } else {
    e->setField(QStringLiteral("producer"), s);
  }
  e->setField(QStringLiteral("nationality"), readString());
  e->setField(QStringLiteral("genre"), readString().replace(QLatin1String(", "), FieldFormat::delimiterString()));

  e->setField(QStringLiteral("cast"), parseCast(readString()).join(FieldFormat::rowDelimiterString()));

  readString(); // url
  e->setField(QStringLiteral("plot"), readString());
  e->setField(QStringLiteral("comments"), readString());
  s = readString(); // video format
  static const QRegularExpression regionRx(QLatin1String("Region \\d"));
  QRegularExpressionMatch regionMatch = regionRx.match(s);
  if(regionMatch.hasMatch()) {
    e->setField(QStringLiteral("region"), regionMatch.captured());
  }
  e->setField(QStringLiteral("audio-track"), readString()); // audio format
  readString(); // resolution
  readString(); // frame rate
  e->setField(QStringLiteral("language"), readString()); // audio language
  e->setField(QStringLiteral("subtitle"), readString()); // subtitle
  readString(); // file size
  s = readString(); // picture extension
  s = readImage(s); // picture
  if(!s.isEmpty()) {
    e->setField(QStringLiteral("cover"), s);
  }

  m_coll->addEntries(e);
}

QStringList AMCImporter::parseCast(const QString& text_) {
  QStringList cast;
  int nPar = 0;
  static const QRegularExpression castRx(QLatin1String("[,()]"));
  QRegularExpressionMatch castMatch = castRx.match(text_);
  QString person, role;
  int oldPos = 0;
  for(int pos = castMatch.capturedStart(); pos > -1; pos = castMatch.capturedStart()) {
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
    castMatch = castRx.match(text_, pos+1);
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
