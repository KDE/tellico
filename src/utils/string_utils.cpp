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

#include "string_utils.h"
#include "../fieldformat.h"

#include <KCharsets>
#include <KLocalizedString>
#include <KRandom>

#include <QRegularExpression>
#include <QTextCodec>
#include <QVariant>
#include <QCache>

namespace {
  static const int STRING_STORE_SIZE = 997; // too big, too small?
}

QString Tellico::decodeHTML(const QByteArray& data_) {
  return decodeHTML(fromHtmlData(data_));
}

QString Tellico::decodeHTML(const QString& text) {
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
  return s.leftRef(idx).toUInt(ok);
}

QString Tellico::i18nReplace(QString text) {
  // Because QDomDocument sticks in random newlines, go ahead and grab them too
  static QRegularExpression rx(QLatin1String("(?:\\n+ *)*<i18n>(.*?)</i18n>(?: *\\n+)*"),
                               QRegularExpression::OptimizeOnFirstUsageOption);
  QRegularExpressionMatch match = rx.match(text);
  while(match.hasMatch()) {
    // KDE bug 254863, be sure to escape just in case of spurious & entities
    text.replace(match.capturedStart(),
                 match.capturedLength(),
                 i18n(match.captured(1).toUtf8().constData()).toHtmlEscaped());
    match = rx.match(text, match.capturedStart()+1);
  }
  return text;
}

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

QString Tellico::minutes(int seconds) {
  int min = seconds / 60;
  seconds = seconds % 60;
  return QString::number(min) + QLatin1Char(':') + QString::number(seconds).rightJustified(2, QLatin1Char('0'));
}

QString Tellico::fromHtmlData(const QByteArray& data_, const char* codecName) {
  QTextCodec* codec = codecName ? QTextCodec::codecForHtml(data_, QTextCodec::codecForName(codecName))
                                : QTextCodec::codecForHtml(data_);
  return codec->toUnicode(data_);
}

QString Tellico::removeAccents(const QString& value_) {
  static QCache<QString, QString> stringCache(STRING_STORE_SIZE);
  if(stringCache.contains(value_)) {
    return *stringCache.object(value_);
  }
  static QRegularExpression rx;
  if(rx.pattern().isEmpty()) {
    QString pattern(QLatin1String("(?:"));
    for(int i = 0x0300; i <= 0x036F; ++i) {
      pattern += QChar(i) + QLatin1Char('|');
    }
    pattern.chop(1);
    pattern += QLatin1Char(')');
    rx.setPattern(pattern);
    rx.optimize();
  }
  // remove accents from table "Combining Diacritical Marks"
  const QString value2 = value_.normalized(QString::NormalizationForm_D).remove(rx);
  stringCache.insert(value_, new QString(value2));
  return value2;
}

QString Tellico::mapValue(const QVariantMap& map, const char* name) {
  const QVariant v = map.value(QLatin1String(name));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::String)) {
    return v.toString();
  } else if(v.canConvert(QVariant::StringList)) {
    return v.toStringList().join(FieldFormat::delimiterString());
  } else if(v.canConvert(QVariant::Map)) {
    // FilmasterFetcher, OpenLibraryFetcher and VNDBFetcher depend on the default "value" field
    return v.toMap().value(QLatin1String("value")).toString();
  } else {
    return QString();
  }
}

QString Tellico::mapValue(const QVariantMap& map, const char* object, const char* name) {
  const QVariant v = map.value(QLatin1String(object));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::Map)) {
    return mapValue(v.toMap(), name);
  } else if(v.canConvert(QVariant::List)) {
    const QVariantList list = v.toList();
    return list.isEmpty() ? QString() : mapValue(list.at(0).toMap(), name);
  } else {
    return QString();
  }
}
