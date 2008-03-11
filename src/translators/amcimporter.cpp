/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

// The information about the AMC file format was taken from the source code for
// GCfilms, (GPL) (c) 2005 Tian
// Monotheka, (GPL) (c) 2004, 2005 Michael Dominic K.
//                      2005 Aurelien Mino

#include "amcimporter.h"
#include "../collections/videocollection.h"
#include "../imagefactory.h"
#include "../latin1literal.h"
#include "../progressmanager.h"
#include "../tellico_debug.h"

#include <kapplication.h>

#include <qfile.h>
#include <qimage.h>

#include <limits.h>

namespace {
  static const QCString AMC_FILE_ID = " AMC_X.Y Ant Movie Catalog 3.5.x   www.buypin.com    www.antp.be ";
}

using Tellico::Import::AMCImporter;

AMCImporter::AMCImporter(const KURL& url_) : DataImporter(url_), m_coll(0), m_cancelled(false) {
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
    return 0;
  }

  QIODevice* f = fileRef().file();
  m_ds.setDevice(f);
  // AMC is always little-endian? can't confirm
  m_ds.setByteOrder(QDataStream::LittleEndian);

  const uint l = AMC_FILE_ID.length();
  QMemArray<char> buffer(l+1);
  m_ds.readRawBytes(buffer.data(), l);
  QString version = QString::fromLocal8Bit(buffer, l);
  QRegExp versionRx(QString::fromLatin1(".+AMC_(\\d+)\\.(\\d+).+"));
  if(version.find(versionRx) == -1) {
    myDebug() << "AMCImporter::collection() - no file id match" << endl;
    return 0;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(f->size());
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  m_coll = new Data::VideoCollection(true);

  m_majVersion = versionRx.cap(1).toInt();
  m_minVersion = versionRx.cap(2).toInt();
//  myDebug() << m_majVersion << "::" << m_minVersion << endl;

  readString(); // name
  readString(); // email
  if(m_majVersion <= 3 && m_minVersion < 5) {
    readString(); // icq
  }
  readString(); // webpage
  readString(); // description

  const bool showProgress = options() & ImportProgress;

  while(!m_cancelled && !f->atEnd()) {
    readEntry();
    if(showProgress) {
      ProgressManager::self()->setProgress(this, f->at());
      kapp->processEvents();
    }
  }

  return m_coll;
}

bool AMCImporter::readBool() {
  Q_UINT8 b;
  m_ds >> b;
  return b;
}

Q_UINT32 AMCImporter::readInt() {
  Q_UINT32 i;
  m_ds >> i;
  if(i >= UINT_MAX) {
    i = 0;
  }
  return i;
}

QString AMCImporter::readString() {
  // The serialization format is a length specifier first, then l bytes of data
  uint l = readInt();
  if(l == 0) {
    return QString();
  }
  QMemArray<char> buffer(l+1);
  m_ds.readRawBytes(buffer.data(), l);
  QString s = QString::fromLocal8Bit(buffer, l);
//  myDebug() << "string: " << s << endl;
  return s;
}

QString AMCImporter::readImage(const QString& format_) {
  uint l = readInt();
  if(l == 0) {
    return QString();
  }
  QMemArray<char> buffer(l+1);
  m_ds.readRawBytes(buffer.data(), l);
  QByteArray bytes;
  bytes.setRawData(buffer.data(), l);
  QImage img(bytes);
  bytes.resetRawData(buffer.data(), l);
  if(img.isNull()) {
    myDebug() << "AMCImporter::readImage() - null image" << endl;
    return QString();
  }
  QString format = QString::fromLatin1("PNG");
  if(format_ == Latin1Literal(".jpg")) {
    format = QString::fromLatin1("JPEG");
  } else if(format_ == Latin1Literal(".gif")) {
    format = QString::fromLatin1("GIF");
  }
  return ImageFactory::addImage(img, format);
}

void AMCImporter::readEntry() {
  Data::EntryPtr e = new Data::Entry(m_coll);

  int id = readInt();
  if(id > 0) {
    e->setId(id);
  }
  readInt(); // add date

  int rating = readInt();
  if(m_majVersion >= 3 && m_minVersion >= 5) {
    rating /= 10;
  }
  e->setField(QString::fromLatin1("rating"), QString::number(rating));
  int year = readInt();
  if(year > 0) {
    e->setField(QString::fromLatin1("year"), QString::number(year));
  }
  int time = readInt();
  if(time > 0) {
    e->setField(QString::fromLatin1("running-time"), QString::number(time));
  }

  readInt(); // video bitrate
  readInt(); // audio bitrate
  readInt(); // number of files
  readBool(); // checked
  readString(); // media label
  e->setField(QString::fromLatin1("medium"), readString());
  readString(); // source
  readString(); // borrower
  QString s = readString(); // title
  if(!s.isEmpty()) {
    e->setField(QString::fromLatin1("title"), s);
  }
  QString s2 = readString(); // translated title
  if(s.isEmpty()) {
    e->setField(QString::fromLatin1("title"), s2);
  }

  e->setField(QString::fromLatin1("director"), readString());
  s = readString();
  QRegExp roleRx(QString::fromLatin1("(.+) \\(([^(]+)\\)"));
  roleRx.setMinimal(true);
  if(s.find(roleRx) > -1) {
    QString role = roleRx.cap(2).lower();
    if(role == Latin1Literal("story") || role == Latin1Literal("written by")) {
      e->setField(QString::fromLatin1("writer"), roleRx.cap(1));
    } else {
      e->setField(QString::fromLatin1("producer"), s);
    }
  } else {
    e->setField(QString::fromLatin1("producer"), s);
  }
  e->setField(QString::fromLatin1("nationality"), readString());
  e->setField(QString::fromLatin1("genre"), readString().replace(QString::fromLatin1(", "), QString::fromLatin1("; ")));

  e->setField(QString::fromLatin1("cast"), parseCast(readString()).join(QString::fromLatin1("; ")));

  readString(); // url
  e->setField(QString::fromLatin1("plot"), readString());
  e->setField(QString::fromLatin1("comments"), readString());
  s = readString(); // video format
  QRegExp regionRx(QString::fromLatin1("Region \\d"));
  if(s.find(regionRx) > -1) {
    e->setField(QString::fromLatin1("region"), regionRx.cap(0));
  }
  e->setField(QString::fromLatin1("audio-track"), readString()); // audio format
  readString(); // resolution
  readString(); // frame rate
  e->setField(QString::fromLatin1("language"), readString()); // audio language
  e->setField(QString::fromLatin1("subtitle"), readString()); // subtitle
  readString(); // file size
  s = readString(); // picture extension
  s = readImage(s); // picture
  if(!s.isEmpty()) {
    e->setField(QString::fromLatin1("cover"), s);
  }

  m_coll->addEntries(e);
}

QStringList AMCImporter::parseCast(const QString& text_) {
  QStringList cast;
  int nPar = 0;
  QRegExp castRx(QString::fromLatin1("[,()]"));
  QString person, role;
  int oldPos = 0;
  for(int pos = text_.find(castRx); pos > -1; pos = text_.find(castRx, pos+1)) {
    if(text_.at(pos) == ',' && nPar%2 == 0) {
      // we're done with this one
      person += text_.mid(oldPos, pos-oldPos).stripWhiteSpace();
      QString all = person;
      if(!role.isEmpty()) {
        if(role.startsWith(QString::fromLatin1("as "))) {
          role = role.mid(3);
        }
        all += "::" + role;
      }
      cast << all;
      person.truncate(0);
      role.truncate(0);
      oldPos = pos+1; // add one to go past comma
    } else if(text_.at(pos) == '(') {
      if(nPar == 0) {
        person = text_.mid(oldPos, pos-oldPos).stripWhiteSpace();
        oldPos = pos+1; // add one to go past parenthesis
      }
      ++nPar;
    } else if(text_.at(pos) == ')') {
      --nPar;
      if(nPar == 0) {
        role = text_.mid(oldPos, pos-oldPos).stripWhiteSpace();
        oldPos = pos+1; // add one to go past parenthesis
      }
    }
  }
  // grab the last one
  if(nPar%2 == 0) {
    int pos = text_.length();
    person += text_.mid(oldPos, pos-oldPos).stripWhiteSpace();
    QString all = person;
    if(!role.isEmpty()) {
      if(role.startsWith(QString::fromLatin1("as "))) {
        role = role.mid(3);
      }
      all += "::" + role;
    }
    cast << all;
  }
  return cast;
}

void AMCImporter::slotCancel() {
  m_cancelled = true;
}

#include "amcimporter.moc"
