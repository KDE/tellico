/***************************************************************************
    copyright            : (C) 2007-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "listviewcomparison.h"
#include "field.h"
#include "fieldformat.h"
#include "collection.h"
#include "document.h"
#include "imagefactory.h"
#include "image.h"

#include <QPixmap>
#include <QDateTime>

namespace {
  int compareFloat(const QString& s1, const QString& s2) {
    bool ok1, ok2;
    float n1 = s1.toFloat(&ok1);
    float n2 = s2.toFloat(&ok2);
    return (ok1 && ok2) ? static_cast<int>(n1-n2) : 0;
  }
}

Tellico::ListViewComparison* Tellico::ListViewComparison::create(Data::FieldPtr field_) {
  if(field_->type() == Data::Field::Number) {
    return new NumberComparison(field_);
  } else if(field_->type() == Data::Field::Bool) {
    return new BoolComparison(field_);
  } else if(field_->type() == Data::Field::Rating) {
    return new RatingComparison(field_);
  } else if(field_->type() == Data::Field::Image ||
            field_->type() == Data::Field::Rating) {
    // bool and ratings only have an image
    return new ImageComparison(field_);
  } else if(field_->type() == Data::Field::Dependent) {
    return new DependentComparison(field_);
  } else if(field_->type() == Data::Field::Date || field_->formatFlag() == Data::Field::FormatDate) {
    return new ISODateComparison(field_);
  } else if(field_->type() == Data::Field::Choice) {
    return new ChoiceComparison(field_);
  } else if(field_->formatFlag() == Data::Field::FormatTitle) {
    // Dependent could be title, so put this test after
    return new TitleComparison(field_);
  } else if(field_->property(QLatin1String("lcc")) == QLatin1String("true") ||
            (field_->name() == QLatin1String("lcc") &&
             Data::Document::self()->collection() &&
             (Data::Document::self()->collection()->type() == Data::Collection::Book ||
              Data::Document::self()->collection()->type() == Data::Collection::Bibtex))) {
    // allow LCC comparison if LCC property is set, or if the name is lcc for a book or bibliography collection
    return new LCCComparison(field_);
  }
  return new StringComparison(field_);
}

Tellico::ListViewComparison::ListViewComparison(Data::FieldPtr field_) : m_field(field_) {
}

int Tellico::ListViewComparison::compare(Data::EntryPtr entry1_, Data::EntryPtr entry2_) {
  return compare(entry1_->field(m_field), entry2_->field(m_field));
}

Tellico::BoolComparison::BoolComparison(Data::FieldPtr field) : ListViewComparison(field) {
}

int Tellico::BoolComparison::compare(const QString& str1_, const QString& str2_) {
  return str1_.compare(str2_);
}

Tellico::StringComparison::StringComparison(Data::FieldPtr field) : ListViewComparison(field) {
}

int Tellico::StringComparison::compare(const QString& str1_, const QString& str2_) {
  return str1_.localeAwareCompare(str2_);
}

Tellico::TitleComparison::TitleComparison(Data::FieldPtr field) : ListViewComparison(field) {
}

int Tellico::TitleComparison::compare(const QString& str1_, const QString& str2_) {
  const QString title1 = FieldFormat::sortKeyTitle(str1_);
  const QString title2 = FieldFormat::sortKeyTitle(str2_);
  return title1.localeAwareCompare(title2);
}

Tellico::NumberComparison::NumberComparison(Data::FieldPtr field) : ListViewComparison(field) {
}

int Tellico::NumberComparison::compare(const QString& str1_, const QString& str2_) {
  // by default, an empty string would get sorted before "1" because toFloat() turns it into "0"
  // I want the empty strings to be at the end
  bool ok1, ok2;
  // use section in case of multiple values
  float num1 = str1_.section(';', 0, 0).toFloat(&ok1);
  float num2 = str2_.section(';', 0, 0).toFloat(&ok2);
  if(ok1 && ok2) {
    return static_cast<int>(num1 - num2);
  } else if(ok1 && !ok2) {
    return -1;
  } else if(!ok1 && ok2) {
    return 1;
  }
  return 0;
}

// for details on the LCC comparison, see
// http://www.mcgees.org/2001/08/08/sort-by-library-of-congress-call-number-in-perl/

Tellico::LCCComparison::LCCComparison(Data::FieldPtr field) : StringComparison(field),
  m_regexp(QLatin1String("^([A-Z]+)(\\d+(?:\\.\\d+)?)\\.?([A-Z]*)(\\d*)\\.?([A-Z]*)(\\d*)(?: (\\d\\d\\d\\d))?")) {
}

int Tellico::LCCComparison::compare(const QString& str1_, const QString& str2_) {
//  myDebug() << "LCCComparison::compare() - " << str1_ << " to " << str2_ << endl;
  int pos1 = m_regexp.indexIn(str1_);
  const QStringList cap1 = m_regexp.capturedTexts();
  int pos2 = m_regexp.indexIn(str2_);
  const QStringList cap2 = m_regexp.capturedTexts();
  if(pos1 > -1 && pos2 > -1) {
    int res = compareLCC(cap1, cap2);
//    myLog() << "...result = " << res << endl;
    return res;
  }
  return StringComparison::compare(str1_, str2_);
}

int Tellico::LCCComparison::compareLCC(const QStringList& cap1, const QStringList& cap2) const {
  // the first item in the list is the full match, so start array index at 1
  int res = 0;
  return (res = cap1[1].compare(cap2[1]))                          != 0 ? res :
         (res = compareFloat(cap1[2], cap2[2]))                    != 0 ? res :
         (res = cap1[3].compare(cap2[3]))                          != 0 ? res :
         (res = compareFloat(QLatin1String("0.") + cap1[4],
                             QLatin1String("0.") + cap2[4])) != 0 ? res :
         (res = cap1[5].compare(cap2[5]))                          != 0 ? res :
         (res = compareFloat(QLatin1String("0.") + cap1[6],
                             QLatin1String("0.") + cap2[6])) != 0 ? res :
         (res = compareFloat(cap1[7], cap2[7]))                    != 0 ? res : 0;
}

Tellico::ImageComparison::ImageComparison(Data::FieldPtr field) : ListViewComparison(field) {
}

int Tellico::ImageComparison::compare(Data::EntryPtr entry1_, Data::EntryPtr entry2_) {
  const QString field1 = entry1_->field(field());
  const QString field2 = entry2_->field(field());
  if(field1.isEmpty()) {
    if(field2.isEmpty()) {
      return 0;
    }
    return -1;
  }
  if(field2.isEmpty()) {
    return 1;
  }

  const Data::Image& image1 = ImageFactory::imageById(field1);
  const Data::Image& image2 = ImageFactory::imageById(field2);
  if(image1.isNull()) {
    if(image2.isNull()) {
      return 0;
    }
    return -1;
  }
  if(image2.isNull()) {
    return 1;
  }
  // large images come first
  return image1.width() - image2.width();
}

Tellico::RatingComparison::RatingComparison(Data::FieldPtr field) : ListViewComparison(field) {
}

int Tellico::RatingComparison::compare(const QString& str1_, const QString& str2_) {
  return str1_.compare(str2_);
}

Tellico::DependentComparison::DependentComparison(Data::FieldPtr field) : StringComparison(field) {
  Data::FieldList fields = field->dependsOn(Data::Document::self()->collection());
  foreach(Data::FieldPtr field, fields) {
    m_comparisons.append(create(field));
  }
}

Tellico::DependentComparison::~DependentComparison() {
  qDeleteAll(m_comparisons);
  m_comparisons.clear();
}

int Tellico::DependentComparison::compare(Data::EntryPtr entry1_, Data::EntryPtr entry2_) {
  foreach(ListViewComparison* comp, m_comparisons) {
    int res = comp->compare(entry1_, entry2_);
    if(res != 0) {
      return res;
    }
  }
  return ListViewComparison::compare(entry1_, entry2_);
}

Tellico::ISODateComparison::ISODateComparison(Data::FieldPtr field) : ListViewComparison(field) {
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
  QStringList dlist1 = str1.split('-', QString::KeepEmptyParts);
  bool ok = true;
  int y1 = dlist1.count() > 0 ? dlist1[0].toInt(&ok) : QDate::currentDate().year();
  if(!ok) {
    y1 = QDate::currentDate().year();
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

  QStringList dlist2 = str2.split('-', QString::KeepEmptyParts);
  int y2 = dlist2.count() > 0 ? dlist2[0].toInt(&ok) : QDate::currentDate().year();
  if(!ok) {
    y2 = QDate::currentDate().year();
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

Tellico::ChoiceComparison::ChoiceComparison(Data::FieldPtr field) : ListViewComparison(field) {
  m_values = field->allowed();
}

int Tellico::ChoiceComparison::compare(const QString& str1, const QString& str2) {
  return m_values.indexOf(str1) - m_values.indexOf(str2);
}
