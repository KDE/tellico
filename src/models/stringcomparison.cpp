/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "stringcomparison.h"
#include "../fieldformat.h"
#include "../tellico_debug.h"

#include <QDateTime>

using namespace Tellico;

namespace {
  int compareFloat(const QString& s1, const QString& s2) {
    bool ok1, ok2;
    float n1 = s1.toFloat(&ok1);
    if(!ok1) {
      return 0;
    }
    float n2 = s2.toFloat(&ok2);
    if(!ok2) {
      return 0;
    }
    return n1 > n2 ? 1 : (n1 < n2 ? -1 : 0);
  }
}

Tellico::StringComparison* Tellico::StringComparison::create(Data::FieldPtr field_) {
  if(!field_) {
    myWarning() << "No field for creating a string comparison";
    return nullptr;
  }
  if(field_->type() == Data::Field::Number || field_->type() == Data::Field::Rating) {
    return new NumberComparison();
  } else if(field_->type() == Data::Field::Bool) {
    return new BoolComparison();
  } else if(field_->type() == Data::Field::Date || field_->formatType() == FieldFormat::FormatDate) {
    return new ISODateComparison();
  } else if(field_->formatType() == FieldFormat::FormatTitle) {
    return new TitleComparison();
  } else if(field_->property(QStringLiteral("lcc")) == QLatin1String("true") ||
            field_->name() == QLatin1String("lcc")) {
    // allow LCC comparison if LCC property is set, or if the name is lcc
    return new LCCComparison();
  }
  return new StringComparison();
}

Tellico::StringComparison::StringComparison() {
}

int Tellico::StringComparison::compare(const QString& str1_, const QString& str2_) {
  return str1_.localeAwareCompare(str2_);
}

Tellico::BoolComparison::BoolComparison() : StringComparison() {
}

int Tellico::BoolComparison::compare(const QString& str1_, const QString& str2_) {
  const bool b1 = str1_.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0
                  || str1_ == QLatin1String("1");
  const bool b2 = str2_.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0
                  || str2_ == QLatin1String("1");
  return b1 == b2 ? 0 : (b1 ? 1 : -1);
}

Tellico::TitleComparison::TitleComparison() : StringComparison() {
}

int Tellico::TitleComparison::compare(const QString& str1_, const QString& str2_) {
  // sortKeyTitle compares against the article list (which is already in lower-case)
  // additionally, we want lower case for localeAwareCompare
  const QString title1 = FieldFormat::sortKeyTitle(str1_.toLower());
  const QString title2 = FieldFormat::sortKeyTitle(str2_.toLower());
  const int ret = title1.localeAwareCompare(title2);
  return ret > 0 ? 1 : (ret < 0 ? -1 : 0);
}

Tellico::NumberComparison::NumberComparison() : StringComparison() {
}

int Tellico::NumberComparison::compare(const QString& str1_, const QString& str2_) {
  bool ok1, ok2;
  float num1 = 0, num2 = 0;

  const QStringList values1 = FieldFormat::splitValue(str1_);
  const QStringList values2 = FieldFormat::splitValue(str2_);
  int index = 0;
  do {
    if((ok1 = index < values1.count())) {
      num1 = values1.at(index).toFloat(&ok1);
    }
    if((ok2 = index < values2.count())) {
      num2 = values2.at(index).toFloat(&ok2);
    }
    if(ok1 && ok2) {
      if(!qFuzzyCompare(num1, num2)) {
        const float ret = num1 - num2;
        // if abs(ret) < 0.5, we want to round up/down to -1 or 1
        // so that comparing 0.2 to 0.4 yields 1, for example, and not 0
        return ret < 0 ? qMin(-1, qRound(ret)) : qMax(1, qRound(ret));
      }
    }
    ++index;
  } while(ok1 && ok2);

  if(ok1 && !ok2) {
    return 1;
  } else if(!ok1 && ok2) {
    return -1;
  }
  return 0;
}

// for details on the LCC comparison, see
// http://www.mcgees.org/2001/08/08/sort-by-library-of-congress-call-number-in-perl/
// http://library.dts.edu/Pages/RM/Helps/lc_call.shtml

Tellico::LCCComparison::LCCComparison() : StringComparison(),
  m_regexp(QLatin1String("^([A-Z]+)"
                         "(\\d+(?:\\.\\d+)?)"
                         "\\.?([A-Z]*)"
                         "(\\d*)"
                         "\\.?([A-Z]*)"
                         "(\\d*)"
                         "(?: (.+))?")) {
}

int Tellico::LCCComparison::compare(const QString& str1_, const QString& str2_) {
  if(str1_.isEmpty()) {
    return str2_.isEmpty() ? 0 : -1;
  }
  if(str2_.isEmpty()) {
    return 1;
  }
//  myDebug() << str1_ << " to " << str2_;
  QRegularExpressionMatch match1 = m_regexp.match(str1_);
  if(!match1.hasMatch()) {
    myDebug() << "no regexp match:" << str1_;
    return StringComparison::compare(str1_, str2_);
  }
  QRegularExpressionMatch match2 = m_regexp.match(str2_);
  if(!match2.hasMatch()) {
    myDebug() << "no regexp match:" << str2_;
    return StringComparison::compare(str1_, str2_);
  }
  QStringList cap1 = match1.capturedTexts();
  QStringList cap2 = match2.capturedTexts();
  // QRegularExpression doesn't include an empty string
  // in optional captured groups that don't exist
  while(cap1.size() < 8) {
    cap1 += QString();
  }
  while(cap2.size() < 8) {
    cap2 += QString();
  }
  return compareLCC(cap1, cap2);
}

int Tellico::LCCComparison::compareLCC(const QStringList& cap1, const QStringList& cap2) const {

  Q_ASSERT(cap1.size() == 8);
  Q_ASSERT(cap2.size() == 8);
  // the first item in the list is the full match, so start array index at 1
  int res = 0;
  return (res = cap1[1].compare(cap2[1]))                    != 0 ? res :
         (res = compareFloat(cap1[2], cap2[2]))              != 0 ? res :
         (res = cap1[3].compare(cap2[3]))                    != 0 ? res :
         (res = compareFloat(QLatin1String("0.") + cap1[4],
                             QLatin1String("0.") + cap2[4])) != 0 ? res :
         (res = cap1[5].compare(cap2[5]))                    != 0 ? res :
         (res = compareFloat(QLatin1String("0.") + cap1[6],
                             QLatin1String("0.") + cap2[6])) != 0 ? res :
         (res = cap1[7].compare(cap2[7]))                    != 0 ? res : 0;
}

Tellico::ISODateComparison::ISODateComparison() : StringComparison() {
}

int Tellico::ISODateComparison::compare(const QString& str1, const QString& str2) {
  if(str1.isEmpty()) {
    return str2.isEmpty() ? 0 : -1;
  }
  if(str2.isEmpty()) { // str1 is not
    return 1;
  }
  // modelled after Field::formatDate()
  // so dates would sort as expected without padding month and day with zero
  // and accounting for "current year - 1 - 1" default scheme
  const QDate now = QDate::currentDate();
  QStringList dlist1 = str1.split(QLatin1Char('-'), Qt::KeepEmptyParts);
  bool ok = true;
  int y1 = dlist1.count() > 0 ? dlist1[0].toInt(&ok) : now.year();
  if(!ok) {
    y1 = now.year();
  }
  int m1 = dlist1.count() > 1 ? dlist1[1].toInt(&ok) : 1;
  if(!ok) {
    m1 = 1;
  }
  int d1 = dlist1.count() > 2 ? dlist1[2].toInt(&ok) : 1;
  if(!ok) {
    d1 = 1;
  }
  QDate date1(y1, m1, d1);

  QStringList dlist2 = str2.split(QLatin1Char('-'), Qt::KeepEmptyParts);
  int y2 = dlist2.count() > 0 ? dlist2[0].toInt(&ok) : now.year();
  if(!ok) {
    y2 = now.year();
  }
  int m2 = dlist2.count() > 1 ? dlist2[1].toInt(&ok) : 1;
  if(!ok) {
    m2 = 1;
  }
  int d2 = dlist2.count() > 2 ? dlist2[2].toInt(&ok) : 1;
  if(!ok) {
    d2 = 1;
  }
  QDate date2(y2, m2, d2);

  if(date1 < date2) {
    return -1;
  } else if(date1 > date2) {
    return 1;
  }
  return 0;
}
