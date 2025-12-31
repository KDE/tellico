/***************************************************************************
    Copyright (C) 2009-2020 Robby Stephenson <robby@periapsis.org>
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

#include "fieldformat.h"
#include "config/tellico_config.h"

using Tellico::FieldFormat;

QString FieldFormat::delimiterString() {
  static QString ds(QStringLiteral("; "));
  return ds;
}

QRegularExpression FieldFormat::delimiterRegularExpression() {
  static const QRegularExpression drx(QStringLiteral("\\s*;\\s*"));
  return drx;
}

QRegularExpression FieldFormat::commaSplitRegularExpression() {
  static const QRegularExpression commaSplitRx(QStringLiteral("\\s*,\\s*"));
  return commaSplitRx;
}

QString FieldFormat::fixupValue(const QString& value_) {
  QString value = value_;
  value.replace(delimiterRegularExpression(), delimiterString());
  return value;
}

QString FieldFormat::columnDelimiterString() {
  static QString cds(QStringLiteral("::"));
  return cds;
}

QString FieldFormat::rowDelimiterString() {
  return QChar(0x2028);
}

QString FieldFormat::matchValueRegularExpression(const QString& value_) {
  // The regular expression accounts for values serialized either with multiple values,
  // values in table columns, or values in table rows
  // Beginning characters don't have to include the column delimiter since the filter
  // only matches values in the first column
  static const QString beginChars = FieldFormat::delimiterString()
                                  + QLatin1String("|")
                                  + FieldFormat::rowDelimiterString();
  static const QString endChars = QLatin1String("[")
                                + FieldFormat::delimiterString().at(0)
                                + FieldFormat::columnDelimiterString().at(0)
                                + FieldFormat::rowDelimiterString().at(0)
                                + QLatin1String("]");
  return QLatin1String("(^|") + beginChars + QLatin1String(")") +
         QRegularExpression::escape(value_) +
         QLatin1String("($|") + endChars + QLatin1String(")");
}

QStringList FieldFormat::splitValue(const QString& string_, SplitParsing parsing_) {
  if(string_.isEmpty()) {
    return QStringList();
  }
  const auto keepFlag = Qt::KeepEmptyParts;
  switch(parsing_) {
    case StringSplit:
      return string_.split(delimiterString(), keepFlag);
    case RegExpSplit:
      return string_.split(delimiterRegularExpression(), keepFlag);
    case CommaRegExpSplit:
      return string_.split(commaSplitRegularExpression(), keepFlag);
  }
  // not needed, but stops warning messages
  return QStringList();
}

QStringList FieldFormat::splitRow(const QString& string_) {
  return string_.isEmpty() ? QStringList() : string_.split(columnDelimiterString(), Qt::KeepEmptyParts);
}

QStringList FieldFormat::splitTable(const QString& string_) {
  return string_.isEmpty() ? QStringList() : string_.split(rowDelimiterString(), Qt::KeepEmptyParts);
}

QString FieldFormat::sortKeyTitle(const QString& title_) {
  foreach(const QString& article, Config::articleList()) {
    // assume white space is already stripped
    // the articles are already in lower-case
    if(title_.startsWith(article + QLatin1Char(' '))) {
      return title_.mid(article.length() + 1);
    }
  }
  // check apostrophes, too
  foreach(const QString& article, Config::articleAposList()) {
    if(title_.startsWith(article)) {
      return title_.mid(article.length());
    }
  }
  return title_;
}

void FieldFormat::stripArticles(QString& value) {
  static QStringList oldArticleList;
  static QList<QRegularExpression> rxList;
  if(oldArticleList != Config::articleList()) {
    oldArticleList = Config::articleList();
    rxList.clear();
    foreach(const QString& article, oldArticleList) {
      rxList << QRegularExpression(QLatin1String("\\b") +
                                   QRegularExpression::escape(article) +
                                   QLatin1String("\\b"));
    }
  }
  foreach(const QRegularExpression& rx, rxList) {
    value.remove(rx);
  }
  value = value.trimmed();
  if(value.endsWith(QLatin1Char(','))) {
    value.chop(1);
  }
}

QString FieldFormat::format(const QString& value_, Type type_, Request request_) {
  if(value_.isEmpty()) {
    return value_;
  }

  Options options;
  if(request_ == ForceFormat || (request_ != AsIsFormat && Config::autoCapitalization())) {
    options |= FormatCapitalize;
  }
  if(request_ == ForceFormat || (request_ != AsIsFormat && Config::autoFormat())) {
    options |= FormatAuto;
  }

  QString text;
  switch(type_) {
    case FormatTitle:
      text = title(value_, options);
      break;
    case FormatName:
      text = name(value_, options);
      break;
    case FormatDate:
      text = date(value_);
      break;
    case FormatPlain:
      text = options.testFlag(FormatCapitalize) ? capitalize(value_) : value_;
      break;
    case FormatNone:
      text = value_;
      break;
  }
  return text;
}

QString FieldFormat::title(const QString& title_, Options opt_) {
  QString newTitle = title_;
  QString tail;
  if(opt_.testFlag(FormatAuto)) {
    // special case for multi-column tables, assume user never has column delimiter in a value
    const int pos = newTitle.indexOf(columnDelimiterString());
    if(pos > -1) {
      tail = columnDelimiterString() + newTitle.mid(pos + columnDelimiterString().length());
      newTitle = newTitle.left(pos);
    }

    // arbitrarily impose rule that a space must follow every comma
    // has to come before the capitalization since the space is significant
    newTitle.replace(commaSplitRegularExpression(), QStringLiteral(", "));
  }

  if(opt_.testFlag(FormatCapitalize)) {
    newTitle = capitalize(newTitle);
  }

  if(opt_.testFlag(FormatAuto)) {
    const QString lower = newTitle.toLower();
    // TODO if the title has ",the" at the end, put it at the front
    foreach(const QString& article, Config::articleList()) {
      // assume white space is already stripped
      // the articles are already in lower-case
      if(lower.startsWith(article + QLatin1Char(' '))) {
        QRegularExpression rx(QLatin1Char('^') + QRegularExpression::escape(article) + QLatin1String("\\s*"),
                              QRegularExpression::CaseInsensitiveOption);
        // can't just use article since it's in lower-case
        QString titleArticle = newTitle.left(article.length());
        newTitle = newTitle.remove(rx)
                           .append(QLatin1String(", "))
                           .append(titleArticle);
        break;
      }
    }
  }

  return newTitle + tail;
}

QString FieldFormat::name(const QString& name_, Options opt_) {
  static const QRegularExpression spaceComma(QStringLiteral("[\\s,]"));
  // the ending look-ahead is so that a space is not added at the end
  static const QRegularExpression periodSpace(QStringLiteral("\\.\\s*(?=.)"));

  QString name = name_;
  name.replace(periodSpace, QStringLiteral(". "));
  if(opt_.testFlag(FormatCapitalize)) {
    name = capitalize(name);
  }

  // split the name by white space and commas
  QStringList words = name.split(spaceComma, Qt::SkipEmptyParts);
  // psycho case where name == ","
  if(words.isEmpty()) {
    return name;
  }

  // if it contains a comma already and the last word is not a suffix, don't format it
  if(!opt_.testFlag(FormatAuto) ||
      (name.indexOf(QLatin1Char(',')) > -1 && !Config::nameSuffixList().contains(words.last(), Qt::CaseInsensitive))) {
    // arbitrarily impose rule that no spaces before a comma and
    // a single space after every comma
    name.replace(commaSplitRegularExpression(), QStringLiteral(", "));
  } else if(words.count() > 1) {
    // otherwise split it by white space, move the last word to the front
    // but only if there is more than one word

    // if the last word is a suffix, it has to be kept with last name
    if(Config::nameSuffixList().contains(words.last(), Qt::CaseInsensitive)) {
      words.prepend(words.last().append(QLatin1Char(',')));
      words.removeLast();
    }

    // now move the word
    // adding comma here when there had been a suffix is because it was originally split with space or comma
    words.prepend(words.last().append(QLatin1Char(',')));
    words.removeLast();

    // this is probably just something for me, limited to english
    // In a previous version of Tellico, using a prefix such as "van der" (with a space) would work
    // because QStringList::contains did substring matching, but now need to add a function for tokenizing
    // the list with whitespace as well as comma
    while(Config::surnamePrefixTokens().contains(words.last(), Qt::CaseInsensitive)) {
      words.prepend(words.last());
      words.removeLast();
    }

    name = words.join(QLatin1String(" "));
  }

  return name;
}

QString FieldFormat::date(const QString& date_) {
  // internally, this is "year-month-day"
  // any of the three may be empty
  // if they're not digits, return the original string
  bool empty = true;
  // for empty year, use current
  // for empty month or date, use 1
  QStringList s = date_.split(QLatin1Char('-'));
  bool ok = true;
  int y = s.count() > 0 ? s[0].toInt(&ok) : QDate::currentDate().year();
  if(ok) {
    empty = false;
  } else {
    y = QDate::currentDate().year();
  }
  int m = s.count() > 1 ? s[1].toInt(&ok) : 1;
  if(ok) {
    empty = false;
  } else {
    m = 1;
  }
  int d = s.count() > 2 ? s[2].toInt(&ok) : 1;
  if(ok) {
    empty = false;
  } else {
    d = 1;
  }
  // rather use ISO date formatting than locale formatting for now. Primarily, it makes sorting just work.
  return empty ? date_ : QDate(y, m, d).toString(Qt::ISODate);
}

QString FieldFormat::capitalize(QString str_) {
  if(str_.isEmpty()) {
    return str_;
  }

  // first letter is always capitalized
  str_.replace(0, 1, str_.at(0).toUpper());

  // regexp to split words
  static const QRegularExpression rx(QStringLiteral("[-\\s,.;]"));

  // special case for french words like l'espace
  QRegularExpressionMatch match = rx.match(str_, 1);
  int pos = match.capturedStart();
  int nextPos;

  QString word = str_.mid(0, pos);
  // now check to see if words starts with apostrophe list
  foreach(const QString& aposArticle, Config::articleAposList()) {
    if(word.startsWith(aposArticle, Qt::CaseInsensitive)) {
      const uint l = aposArticle.length();
      str_.replace(l, 1, str_.at(l).toUpper());
      break;
    }
  }

  while(pos > -1) {
    // also need to compare against list of non-capitalized words
    match = rx.match(str_, pos+1);
    nextPos = match.capturedStart();
    if(nextPos == -1) {
      nextPos = str_.length();
    }
    word = str_.mid(pos+1, nextPos-pos-1);
    bool aposMatch = false;
    // now check to see if words starts with apostrophe list
    foreach(const QString& aposArticle, Config::articleAposList()) {
      if(word.startsWith(aposArticle, Qt::CaseInsensitive)) {
        const uint l = aposArticle.length();
        // if the word is not the end of the string, capitalize the letter after it
        if(int(pos+l+1) < str_.length()) {
          str_.replace(pos+l+1, 1, str_.at(pos+l+1).toUpper());
        }
        aposMatch = true;
        break;
      }
    }

    if(!aposMatch) {
      // check against the noCapitalization list AND the surnamePrefix list
      // does this hold true everywhere other than english?
      if(!Config::noCapitalizationList().contains(word, Qt::CaseInsensitive) &&
         !Config::surnamePrefixTokens().contains(word, Qt::CaseInsensitive) &&
         nextPos-pos > 1) {
        str_.replace(pos+1, 1, str_.at(pos+1).toUpper());
      }
    }

    match = rx.match(str_, pos+1);
    pos = match.capturedStart();
  }
  return str_;
}
