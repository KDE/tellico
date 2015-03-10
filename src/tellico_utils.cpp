/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellico_utils.h"
#include "tellico_debug.h"

#include <kapplication.h>
#include <klocale.h>
#include <kglobal.h>
#include <klibrary.h>
#include <kstandarddirs.h>
#include <kcharsets.h>
#include <krandom.h>
#include <KLocalizedString>

#include <QRegExp>
#include <QDir>
#include <QTextCodec>
#include <QTextDocument>

namespace {
  static const int STRING_STORE_SIZE = 997; // too big, too small?
}

QString Tellico::decodeHTML(const QByteArray& data_) {
  return decodeHTML(fromHtmlData(data_));
}

QString Tellico::decodeHTML(QString text) {
  return KCharsets::resolveEntities(text);
}

QString Tellico::uid(int l, bool prefix) {
  QString uid;
  if(prefix) {
    uid = QLatin1String("Tellico");
  }
  uid.append(KRandom::randomString(qMax(l - uid.length(), 0)));
  return uid;
}

uint Tellico::toUInt(const QString& s, bool* ok) {
  if(s.isEmpty()) {
    if(ok) {
      *ok = false;
    }
    return 0;
  }

  int idx = 0;
  while(idx < s.length() && s[idx].isDigit()) {
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
  static QRegExp rx(QLatin1String("(?:\\n+ *)*<i18n>([^<]*)</i18n>(?: *\\n+)*"));
  int pos = rx.indexIn(text);
  while(pos > -1) {
    // KDE bug 254863, be sure to escape just in case of spurious & entities
    text.replace(pos, rx.matchedLength(), Qt::escape(i18n(rx.cap(1).toUtf8())));
    pos = rx.indexIn(text, pos+rx.matchedLength());
  }
  return text;
}

QString Tellico::removeAcceleratorMarker(const QString& label_) {
  return KLocalizedString::removeAcceleratorMarker(label_);
}

QStringList Tellico::findAllSubDirs(const QString& dir_) {
  if(dir_.isEmpty()) {
    return QStringList();
  }

  // TODO: build in symlink checking, for now, prohibit
  QDir dir(dir_, QString(), QDir::Name | QDir::IgnoreCase, QDir::Dirs | QDir::Readable | QDir::NoSymLinks);

  QStringList allSubdirs; // the whole list

  // find immediate sub directories
  const QStringList subdirs = dir.entryList();
  for(QStringList::ConstIterator subdir = subdirs.begin(); subdir != subdirs.end(); ++subdir) {
    if((*subdir).isEmpty() || *subdir == QLatin1String(".") || *subdir == QLatin1String("..")) {
      continue;
    }
    QString absSubdir = dir.absoluteFilePath(*subdir);
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
  for(int i = 0; i < str.length(); ++i) {
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

KLibrary* Tellico::openLibrary(const QString& libName_) {
  KLibrary* library = new KLibrary(libName_);
  if(!library->load()) {
    myWarning() << "Could not load library'" << libName_ << "'";
    myWarning() << "ERROR:" << library->errorString();
    return 0;
  }

  return library;
}

QString Tellico::minutes(int seconds) {
  int min = seconds / 60;
  seconds = seconds % 60;
  return QString::number(min) + QLatin1Char(':') + QString::number(seconds).rightJustified(2, QLatin1Char('0'));
}

QString Tellico::saveLocation(const QString& dir_) {
  return KGlobal::dirs()->saveLocation("appdata", dir_, true);
}

QString Tellico::fromHtmlData(const QByteArray& data_, const char* codecName) {
  QTextCodec* codec = codecName ? QTextCodec::codecForHtml(data_, QTextCodec::codecForName(codecName))
                                : QTextCodec::codecForHtml(data_);
  return codec->toUnicode(data_);
}

QString Tellico::removeAccents(const QString& value_) {
  QString value2 = value_.normalized(QString::NormalizationForm_D);
  // remove accents from table "Combining Diacritical Marks"
  for(int i = 0x0300; i <= 0x036F; ++i) {
    value2.remove(QChar(i));
  }
  return value2;
}
