/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "field.h"

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>

#include <qstringlist.h>
#include <qregexp.h>
#include <qdatetime.h>

namespace {
  static const QRegExp comma_split = QRegExp(QString::fromLatin1("\\s*,\\s*"));
}

using Tellico::Data::Field;

//these get overwritten, but are here since they're static
QStringList Field::s_articles;
QStringList Field::s_articlesApos;
QStringList Field::s_suffixes;
QStringList Field::s_surnamePrefixes;
// put into i18n for translation
// and allow spaces in the regexp, someone might accidently put one there
QStringList Field::s_noCapitalize = QStringList::split(comma_split,
                                          i18n("a,an,and,in,of,the,to"), false);
bool Field::s_autoCapitalize = true;
bool Field::s_autoFormat = true;

QRegExp Field::s_delimiter = QRegExp(QString::fromLatin1("\\s*;\\s*"));

// this constructor is for anything but Choice type
Field::Field(const QString& name_, const QString& title_, Type type_/*=Line*/)
    : m_name(name_), m_title(title_),  m_category(i18n("General")), m_desc(title_),
      m_type(type_), m_flags(0), m_formatFlag(FormatNone) {

#ifndef NDEBUG
  if(m_type == Choice) {
    kdWarning() << "Field() - A different constructor should be called for multiple choice attributes." << endl;
    kdWarning() << "Constructing a Field with name = " << name_ << endl;
  }
#endif
  // a paragraph's category is always its title, along with tables
  if(isSingleCategory()) {
    m_category = m_title;
  }
  if(m_type == Table || m_type == Table2) {
    m_flags = AllowMultiple;
  }
  // hidden from user
  if(type_ == Date) {
    m_formatFlag = FormatDate;
  }
}

// if this constructor is called, the type is necessarily Choice
Field::Field(const QString& name_, const QString& title_, const QStringList& allowed_)
    : m_name(name_), m_title(title_), m_category(i18n("General")), m_desc(title_),
      m_type(Field::Choice), m_allowed(allowed_), m_flags(0), m_formatFlag(FormatNone) {
}

Field::Field(const Field& field_)
    : m_name(field_.name()), m_title(field_.title()), m_category(field_.category()),
      m_desc(field_.description()), m_type(field_.type()),
      m_flags(field_.flags()), m_formatFlag(field_.formatFlag()),
      m_properties(field_.propertyList()) {
  if(m_type == Choice) {
    m_allowed = field_.allowed();
  }
}

Field& Field::operator=(const Field& field_) {
  if(this != &field_) {
    m_name = field_.name();
    m_title = field_.title();
    m_category = field_.category();
    m_desc = field_.description();
    m_type = field_.type();
    if(m_type == Choice) {
      m_allowed = field_.allowed();
    }
    m_flags = field_.flags();
    m_formatFlag = field_.formatFlag();
    m_properties = field_.propertyList();
  }
  return *this;
}

void Field::setTitle(const QString& title_) {
  m_title = title_;
  if(isSingleCategory()) {
    m_category = title_;
  }
}

void Field::setType(Field::Type type_) {
  m_type = type_;
  if(m_type != Field::Choice) {
    m_allowed = QString::null;
  }
  if(m_type == Table || m_type == Table2) {
    m_flags |= AllowMultiple;
  }
  if(isSingleCategory()) {
    m_category = m_title;
  }
  // hidden from user
  if(type_ == Date) {
    m_formatFlag = FormatDate;
  }
}

void Field::setCategory(const QString& category_) {
  if(!isSingleCategory()) {
    m_category = category_;
  }
}

void Field::setFlags(int flags_) {
  // tables always have multiple allowed
  if(m_type == Table || m_type == Table2) {
    m_flags = AllowMultiple | flags_;
  } else {
    m_flags = flags_;
  }
}

bool Field::isSingleCategory() const {
  return (m_type == Para || m_type == Table || m_type == Table2 || m_type == Image);
}

QString Field::format(const QString& value_, FormatFlag flag_) {
  if(value_.isEmpty()) {
    return value_;
  }

  QString text;
  switch(flag_) {
    case FormatTitle:
      text = formatTitle(value_);
      break;
    case FormatName:
      text = formatName(value_);
      break;
    case FormatDate:
      text = formatDate(value_);
      break;
    case FormatPlain:
      text = autoCapitalize() ? capitalize(value_) : value_;
      break;
    default:
      text = value_;
      break;
  }
  return text;
}

QString Field::formatTitle(const QString& title_) {
  QString newTitle = title_;
  // special case for 2-column tables, assume user never has '::' in a value
  const QString colonColon = QString::fromLatin1("::");
  QString tail;
  if(newTitle.find(colonColon) > -1) {
    tail = colonColon + newTitle.section(colonColon, 1);
    newTitle = newTitle.section(colonColon, 0, 0);
  }

  if(autoCapitalize()) {
    newTitle = capitalize(newTitle);
  }

  if(autoFormat()) {
    // TODO if the title has ",the" at the end, put it at the front
    for(QStringList::ConstIterator it = s_articles.begin(); it != s_articles.end(); ++it) {
      // assume white space is already stripped
      // the articles are already in lower-case
      QString lower = newTitle.lower();
      if(lower.startsWith(*it + QChar(' '))) {
        QRegExp regexp(QChar('^') + QRegExp::escape(*it) + QString::fromLatin1("\\s*"), false);
        // can't just use *it since it's in lower-case
        QString article = newTitle.left((*it).length());
        newTitle = newTitle.replace(regexp, QString::null)
                           .append(QString::fromLatin1(", "))
                           .append(article);
        break;
      }
    }
  }

  // also, arbitrarily impose rule that a space must follow every comma
  newTitle.replace(comma_split, QString::fromLatin1(", "));
  return newTitle + tail;
}

QString Field::formatName(const QString& name_, bool multiple_/*=true*/) {
  static const QRegExp spaceComma(QString::fromLatin1("[\\s,]"));

  QStringList entries;
  if(multiple_) {
    // split by semi-colon, optionally preceded or followed by white spacee
    entries = QStringList::split(s_delimiter, name_, false);
  } else {
    entries << name_;
  }

  QRegExp lastWord;
  lastWord.setCaseSensitive(false);
  const QString colonColon = QString::fromLatin1("::");
  // the ending look-ahead is so that a space is not added at the end
  const QRegExp periodSpace(QString::fromLatin1("\\.\\s*(?=.)"));

  QStringList names;
  for(QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it) {
    QString name = *it;
    // special case for 2-column tables, assume user never has '::' in a value
    QString tail;
    if(name.find(colonColon) > -1) {
      tail = colonColon + name.section(colonColon, 1);
      name = name.section(colonColon, 0, 0);
    }
    name.replace(periodSpace, QString::fromLatin1(". "));
    if(autoCapitalize()) {
      name = capitalize(name);
    }

    // split the name by white space and commas
    QStringList words = QStringList::split(spaceComma, name, false);
    lastWord.setPattern(QChar('^') + QRegExp::escape(words.last()) + QChar('$'));

    // if it contains a comma already and the last word is not a suffix, don't format it
    if(!autoFormat() || name.find(',') > -1 && s_suffixes.grep(lastWord).isEmpty()) {
      // arbitrarily impose rule that no spaces before a comma and
      // a single space after every comma
      name.replace(comma_split, QString::fromLatin1(", "));
      names << name + tail;
      continue;
    }
    // otherwise split it by white space, move the last word to the front
    // but only if there is more than one word
    if(words.count() > 1) {
      // if the last word is a suffix, it has to be kept with last name
      if(s_suffixes.grep(lastWord).count() > 0) {
        words.prepend(words.last().append(QChar(',')));
        words.remove(words.fromLast());
      }

      // now move the word
      // adding comma here when there had been a suffix is because it was originally split with space or comma
      words.prepend(words.last().append(QChar(',')));
      words.remove(words.fromLast());

      // update last word regexp
      lastWord.setPattern(QChar('^') + QRegExp::escape(words.last()) + QChar('$'));

      // this is probably just something for me, limited to english
      while(s_surnamePrefixes.grep(lastWord).count() > 0) {
        words.prepend(words.last());
        words.remove(words.fromLast());
        lastWord.setPattern(QChar('^') + QRegExp::escape(words.last()) + QChar('$'));
      }

      names << words.join(QChar(' ')) + tail;
    } else {
      names << name + tail;
    }
  }

  return names.join(QString::fromLatin1("; "));
}

QString Field::formatDate(const QString& date_) {
  // internally, this is "year-month-day"
  // any of the three may be empty
  // for empty year, use current
  // for empty month or date, use 1
  QStringList s = QStringList::split('-', date_, true);
  bool ok = true;
  int y = s.count() > 0 ? s[0].toInt(&ok) : QDate::currentDate().year();
  if(!ok) {
    y = QDate::currentDate().year();
    ok = true;
  }
  int m = s.count() > 1 ? s[1].toInt(&ok) : 1;
  if(!ok) {
    m = 1;
    ok = true;
  }
  int d = s.count() > 2 ? s[2].toInt(&ok) : 1;
  if(!ok) {
    d = 1;
  }
  QDate date(y, m, d);
  // rather use ISO date formatting than locale formatting for now. Primarily, it makes sorting just work.
  return date.toString(Qt::ISODate);
  // use short form
//  return KGlobal::locale()->formatDate(date,  true);
}

QString Field::capitalize(QString str_) {
  // regexp to split words
  static const QRegExp rx(QString::fromLatin1("[-\\s,.;]"));

  if(str_.isEmpty()) {
    return str_;
  }
  // first letter is always capitalized
  str_.replace(0, 1, str_.at(0).upper());

  // special case for french words like l'espace

  int pos = str_.find(rx, 1);
  int nextPos;

  QRegExp wordRx;
  wordRx.setCaseSensitive(false);

  QStringList notCap = s_noCapitalize;
  // don't capitalize the surname prefixes
  // does this hold true everywhere other than english?
  notCap += Field::surnamePrefixList();

  QString word = str_.mid(0, pos);
  // now check to see if words starts with apostrophe list
  for(QStringList::ConstIterator it = s_articlesApos.begin(); it != s_articlesApos.end(); ++it) {
    if(word.lower().startsWith(*it)) {
      uint l = (*it).length();
      str_.replace(l, 1, str_.at(l).upper());
      break;
    }
  }

  while(pos > -1) {
    // also need to compare against list of non-capitalized words
    nextPos = str_.find(rx, pos+1);
    if(nextPos == -1) {
      nextPos = str_.length();
    }
    word = str_.mid(pos+1, nextPos-pos-1);
    bool aposMatch = false;
    // now check to see if words starts with apostrophe list
    for(QStringList::ConstIterator it = s_articlesApos.begin(); it != s_articlesApos.end(); ++it) {
      if(word.lower().startsWith(*it)) {
        uint l = (*it).length();
        str_.replace(pos+l+1, 1, str_.at(pos+l+1).upper());
        aposMatch = true;
        break;
      }
    }

    if(!aposMatch)  {
      wordRx.setPattern(QChar('^') + QRegExp::escape(word) + QChar('$'));
      if(notCap.grep(wordRx).isEmpty() && nextPos-pos > 1) {
        str_.replace(pos+1, 1, str_.at(pos+1).upper());
      }
    }

    pos = str_.find(rx, pos+1);
  }
  return str_;
}

QStringList Field::defaultArticleList() {
// put the articles in i18n() so they can be translated
  return QStringList::split(comma_split, i18n("the"), false);
}

// articles should all be in lower-case
void Field::setArticleList(const QStringList& list_) {
  s_articles = list_;

  s_articlesApos.clear();
  for(QStringList::Iterator it = s_articles.begin(); it != s_articles.end(); ++it) {
    (*it) = (*it).lower();
    if((*it).endsWith(QChar('\''))) {
      s_articlesApos += (*it);
    }
  }
}

QStringList Field::defaultSuffixList() {
// put the suffixes in i18n() so they can be translated
  return QStringList::split(comma_split, i18n("jr.,jr,iii,iv"), false);
}

QStringList Field::defaultSurnamePrefixList() {
// put the articles in i18n() so they can be translated
  return QStringList::split(comma_split, i18n("de,van,der,von"), false);
}

// if these are changed, then CollectionFieldsDialog should be checked since it
// checks for equality against some of these strings
QMap<Field::Type, QString> Field::typeMap() {
  QMap<Field::Type, QString> map;
  map[Field::Line]      = i18n("Simple Text");
  map[Field::Para]      = i18n("Paragraph");
  map[Field::Choice]    = i18n("Choice");
  map[Field::Bool]      = i18n("Checkbox");
  map[Field::Number]    = i18n("Number");
  map[Field::URL]       = i18n("URL");
  map[Field::Table]     = i18n("Table");
  map[Field::Table2]    = i18n("Table (2 Columns)");
  map[Field::Image]     = i18n("Image");
  map[Field::Dependent] = i18n("Dependent");
//  map[Field::ReadOnly] = i18n("Read Only");
  map[Field::Date]      = i18n("Date");
  return map;
}

// just for formatting's sake
QStringList Field::typeTitles() {
  const QMap<Field::Type, QString>& map = typeMap();
  QStringList list;
  list.append(map[Field::Line]);
  list.append(map[Field::Para]);
  list.append(map[Field::Choice]);
  list.append(map[Field::Bool]);
  list.append(map[Field::Number]);
  list.append(map[Field::URL]);
  list.append(map[Field::Date]);
  list.append(map[Field::Table]);
  list.append(map[Field::Table2]);
  list.append(map[Field::Image]);
  list.append(map[Field::Dependent]);
  return list;
}

QStringList Field::split(const QString& string_, bool allowEmpty_) {
  return string_.isEmpty() ? QStringList() : QStringList::split(s_delimiter, string_, allowEmpty_);
}

void Field::addAllowed(const QString& value_) {
  if(m_type != Choice) {
    return;
  }
  if(m_allowed.findIndex(value_) == -1) {
    m_allowed += value_;
  }
}
