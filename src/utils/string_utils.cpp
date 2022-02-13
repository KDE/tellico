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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include <QRandomGenerator>
#endif

namespace {
  static const int STRING_STORE_SIZE = 4999; // too big, too small?

  class StringIterator {
    QString::const_iterator pos, e;
  public:
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    explicit StringIterator(QStringView string)
#else
    explicit StringIterator(const QString& string)
#endif
    : pos(string.begin()), e(string.end()) {}
    inline bool hasNext() const { return pos < e; }
    inline uint next() {
      Q_ASSERT(hasNext());
      const QChar uc = *pos++;
      if(uc.isSurrogate()) {
        if(uc.isHighSurrogate() && pos < e && pos->isLowSurrogate())
          return QChar::surrogateToUcs4(uc, *pos++);
        return QChar::ReplacementCharacter;
      }
      return uc.unicode();
    }
  };
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
    uid = QStringLiteral("Tellico");
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
  static QRegularExpression rx(QStringLiteral("(?:\\n+ *)*<i18n>(.*?)</i18n>(?: *\\n+)*"),
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

  const int index = h;
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
    QString pattern(QStringLiteral("(?:"));
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
    return v.toMap().value(QStringLiteral("value")).toString();
  } else {
    return QString();
  }
}

QString Tellico::mapValue(const QVariantMap& map, const char* name1, const char* name2) {
  const QVariant v = map.value(QLatin1String(name1));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::Map)) {
    return mapValue(v.toMap(), name2);
  } else if(v.canConvert(QVariant::List)) {
    QStringList values;
    foreach(QVariant v, v.toList()) {
      const QString s = mapValue(v.toMap(), name2);
      if(!s.isEmpty()) values += s;
    }
    return values.join(FieldFormat::delimiterString());
  } else {
    return QString();
  }
}

QString Tellico::mapValue(const QVariantMap& map, const char* name1, const char* name2, const char* name3) {
  const QVariant v = map.value(QLatin1String(name1));
  if(v.isNull())  {
    return QString();
  } else if(v.canConvert(QVariant::Map)) {
    return mapValue(v.toMap(), name2, name3);
  } else {
    return QString();
  }
}

QByteArray Tellico::obfuscate(const QString& string) {
  QByteArray b;
  b.reserve(string.length() * 2);
  for(int p = 0; p < string.length(); p++) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 10, 0))
    char c = KRandom::random() % 255;
#else
    char c = QRandomGenerator::global()->generate() % 255;
#endif
    b.prepend(c ^ string.at(p).unicode());
    b.prepend(c);
  }
  return b.toHex();
}

QString Tellico::reverseObfuscate(const QByteArray& bytes) {
  if(bytes.length() % 2 != 0 || bytes.isEmpty()) {
    return QString();
  }
  const QByteArray b = QByteArray::fromHex(bytes);
  QString result;
  result.reserve(b.length() / 2);
  for(int p = b.length()-1; p >= 0; p -= 2) {
    result.append(QLatin1Char(b.at(p-1) ^ b.at(p)));
  }
  return result;
}

QString Tellico::removeControlCodes(const QString& string) {
  QString result;
  result.reserve(string.size());
  StringIterator it(string);
  while(it.hasNext()) {
    const auto c = it.next();
    // legal control codes in XML 1.0 are U+0009, U+000A, U+000D
    // https://www.w3.org/TR/xml/#charsets
    if(c > 0x1F || c == 0x9 || c == 0xA || c == 0xD) {
      if(c < 0xd800) result += QChar(c);
      else result += QString::fromUcs4(&c, 1);
    }
  }
  return result;
}
