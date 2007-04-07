/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellico_utils.h"
#include "tellico_kernel.h"
#include "latin1literal.h"
#include "tellico_debug.h"

#include <kapplication.h>
#include <klocale.h>
#include <kglobal.h>
#include <klibloader.h>
#include <kstandarddirs.h>
#include <kcharsets.h>

#include <qregexp.h>
#include <qdir.h>
#include <qcursor.h>
#include <qscrollview.h>

namespace {
  static const int STRING_STORE_SIZE = 997; // too big, too small?
}

QColor Tellico::contrastColor;

QString Tellico::decodeHTML(QString text) {
  QRegExp rx(QString::fromLatin1("&(.+);"));
  rx.setMinimal(true);
  int pos = rx.search(text);
  while(pos > -1) {
    QChar c = KCharsets::fromEntity(rx.cap(1));
    if(!c.isNull()) {
      text.replace(pos, rx.matchedLength(), c);
    }
    pos = rx.search(text, pos+1);
  }
  return text;
}

QString Tellico::uid(int l, bool prefix) {
  QString uid;
  if(prefix) {
    uid = QString::fromLatin1("Tellico");
  }
  uid.append(kapp->randomString(QMAX(l - uid.length(), 0)));
  return uid;
}

uint Tellico::toUInt(const QString& s, bool* ok) {
  if(s.isEmpty()) {
    if(ok) {
      *ok = false;
    }
    return 0;
  }

  uint idx = 0;
  while(s[idx].isDigit()) {
    ++idx;
  }
  if(idx == 0) {
    if(ok) {
      *ok = false;
    }
    return 0;
  }
  return s.left(idx).toUInt(ok);
}

QString Tellico::i18nReplace(QString text) {
  // Because QDomDocument sticks in random newlines, go ahead and grab them too
  static QRegExp rx(QString::fromLatin1("(?:\\n+ *)*<i18n>([^<]*)</i18n>(?: *\\n+)*"));
  int pos = rx.search(text);
  while(pos > -1) {
    text.replace(pos, rx.matchedLength(), i18n(rx.cap(1).utf8()));
    pos = rx.search(text, pos+rx.matchedLength());
  }
  return text;
}

QStringList Tellico::findAllSubDirs(const QString& dir_) {
  if(dir_.isEmpty()) {
    return QStringList();
  }

  // TODO: build in symlink checking, for now, prohibit
  QDir dir(dir_, QString::null, QDir::Name | QDir::IgnoreCase, QDir::Dirs | QDir::Readable | QDir::NoSymLinks);

  QStringList allSubdirs; // the whole list

  // find immediate sub directories
  const QStringList subdirs = dir.entryList();
  for(QStringList::ConstIterator subdir = subdirs.begin(); subdir != subdirs.end(); ++subdir) {
    if((*subdir).isEmpty() || *subdir == Latin1Literal(".") || *subdir == Latin1Literal("..")) {
      continue;
    }
    QString absSubdir = dir.absFilePath(*subdir);
    allSubdirs += findAllSubDirs(absSubdir);
    allSubdirs += absSubdir;
  }
  return allSubdirs;
}

// Based on QGDict's hash functions, Copyright (C) 1992-2000 Trolltech AS
// and used from Juk, Copyright (C) 2003 - 2004 by Scott Wheeler
int Tellico::stringHash(const QString& str) {
  uint h = 0;
  uint g = 0;
  for(uint i = 0; i < str.length(); ++i) {
    h = (h << 4) + str.unicode()[i].cell();
    if((g = h & 0xf0000000)) {
      h ^= g >> 24;
    }
    h &= ~g;
  }

  int index = h;
  return index < 0 ? -index : index;
}

QString Tellico::shareString(const QString& str) {
  static QString stringStore[STRING_STORE_SIZE];

  const int hash = stringHash(str) % STRING_STORE_SIZE;
  if(stringStore[hash] != str) {
    stringStore[hash] = str;
  }
  return stringStore[hash];
}

void Tellico::updateContrastColor(const QColorGroup& cg_) {
  // if the value difference between background and highlight is more than ???
  // use highlight, else go lighter or darker
  int h1, s1, v1, h2, s2, v2;
  cg_.background().getHsv(&h1, &s1, &v1);

  QColor hl = cg_.highlight();
  hl.getHsv(&h2, &s2, &v2);
  h2 += 120;
  s2 = 255;
  hl.setHsv(h2, s2, v2);

  if(KABS(v2-v1) < 48) {
    if(v1 < 128) {
      contrastColor = hl.light();
    } else {
      contrastColor = hl.dark();
    }
  } else {
    contrastColor = hl;
  }
}

KLibrary* Tellico::openLibrary(const QString& libName_) {
  QString path = KLibLoader::findLibrary(QFile::encodeName(libName_));
  if(path.isEmpty()) {
    kdWarning() << "Tellico::openLibrary() - Could not find library '" << libName_ << "'" << endl;
    kdWarning() << "ERROR: " << KLibLoader::self()->lastErrorMessage() << endl;
    return 0;
  }

  KLibrary* library = KLibLoader::self()->library(QFile::encodeName(path));
  if(!library) {
    kdWarning() << "Tellico::openLibrary() - Could not load library '" << libName_ << "'" << endl;
    kdWarning() << " PATH: " << path << endl;
    kdWarning() << "ERROR: " << KLibLoader::self()->lastErrorMessage() << endl;
    return 0;
  }

  return library;
}

QColor Tellico::blendColors(const QColor& color1, const QColor& color2, int percent) {
  const float factor2 = percent/100.0;
  const float factor1 = 1.0 - factor2;

  const int r = static_cast<int>(color1.red()   * factor1 + color2.red()   * factor2);
  const int g = static_cast<int>(color1.green() * factor1 + color2.green() * factor2);
  const int b = static_cast<int>(color1.blue()  * factor1 + color2.blue()  * factor2);

  return QColor(r, g, b);
}

QString Tellico::minutes(int seconds) {
  int min = seconds / 60;
  seconds = seconds % 60;
  return QString::number(min) + ':' + QString::number(seconds).rightJustify(2, '0');
}

QString Tellico::saveLocation(const QString& dir_) {
  return KGlobal::dirs()->saveLocation("appdata", dir_, true);
}

Tellico::GUI::CursorSaver::CursorSaver(const QCursor& cursor_) : m_restored(false) {
  kapp->setOverrideCursor(cursor_);
}

Tellico::GUI::CursorSaver::~CursorSaver() {
  if(!m_restored) {
    kapp->restoreOverrideCursor();
  }
}

void Tellico::GUI::CursorSaver::restore() {
  kapp->restoreOverrideCursor();
  m_restored = true;
}
