/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
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
#include "latin1literal.h"
#include "field.h"
#include "collection.h"
#include "document.h"
#include "entryitem.h"

#include <qlistview.h>
#include <qpixmap.h>
#include <qdatetime.h>

namespace {
  int compareFloat(const QString& s1, const QString& s2) {
    bool ok1, ok2;
    float n1 = s1.toFloat(&ok1);
    float n2 = s2.toFloat(&ok2);
    return (ok1 && ok2) ? static_cast<int>(n1-n2) : 0;
  }
}

Tellico::ListViewComparison* Tellico::ListViewComparison::create(Data::FieldPtr field_) {
  return create(Data::ConstFieldPtr(field_.data()));
}

Tellico::ListViewComparison* Tellico::ListViewComparison::create(Data::ConstFieldPtr field_) {
  if(field_->type() == Data::Field::Number) {
    return new NumberComparison(field_);
  } else if(field_->type() == Data::Field::Image ||
            field_->type() == Data::Field::Rating ||
            field_->type() == Data::Field::Bool) {
    // bool and ratings only have an image
    return new PixmapComparison(field_);
  } else if(field_->type() == Data::Field::Dependent) {
    return new DependentComparison(field_);
  } else if(field_->type() == Data::Field::Date || field_->formatFlag() == Data::Field::FormatDate) {
    return new ISODateComparison(field_);
  } else if(field_->formatFlag() == Data::Field::FormatTitle) {
    // Dependent could be title, so put this test after
    return new TitleComparison(field_);
  } else if(field_->property(QString::fromLatin1("lcc")) == Latin1Literal("true") ||
            (field_->name() == Latin1Literal("lcc") &&
             Data::Document::self()->collection() &&
             (Data::Document::self()->collection()->type() == Data::Collection::Book ||
              Data::Document::self()->collection()->type() == Data::Collection::Bibtex))) {
    // allow LCC comparison if LCC property is set, or if the name is lcc for a book or bibliography collection
    return new LCCComparison(field_);
  }
  return new StringComparison(field_);
}

Tellico::ListViewComparison::ListViewComparison(Data::ConstFieldPtr field) : m_fieldName(field->name()) {
}

int Tellico::ListViewComparison::compare(int col_,
                                         const GUI::ListViewItem* item1_,
                                         const GUI::ListViewItem* item2_,
                                         bool asc_)
{
  return compare(key(item1_, col_, asc_), key(item2_, col_, asc_));
}

QString Tellico::ListViewComparison::key(const GUI::ListViewItem* item_, int col_, bool asc_) {
  return item_->isEntryItem()
         ? static_cast<const EntryItem*>(item_)->entry()->field(m_fieldName)
         : item_->key(col_, asc_);
}

Tellico::StringComparison::StringComparison(Data::ConstFieldPtr field) : ListViewComparison(field) {
}

int Tellico::StringComparison::compare(const QString& str1_, const QString& str2_) {
  return str1_.localeAwareCompare(str2_);
}

Tellico::TitleComparison::TitleComparison(Data::ConstFieldPtr field) : ListViewComparison(field) {
}

int Tellico::TitleComparison::compare(const QString& str1_, const QString& str2_) {
  const QString title1 = Data::Field::sortKeyTitle(str1_);
  const QString title2 = Data::Field::sortKeyTitle(str2_);
  return title1.localeAwareCompare(title2);
}

Tellico::NumberComparison::NumberComparison(Data::ConstFieldPtr field) : ListViewComparison(field) {
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

Tellico::LCCComparison::LCCComparison(Data::ConstFieldPtr field) : StringComparison(field),
  m_regexp(QString::fromLatin1("^([A-Z]+)(\\d+(?:\\.\\d+)?)\\.?([A-Z]*)(\\d*)\\.?([A-Z]*)(\\d*)(?: (\\d\\d\\d\\d))?")) {
}

int Tellico::LCCComparison::compare(const QString& str1_, const QString& str2_) {
//  myDebug() << "LCCComparison::compare() - " << str1_ << " to " << str2_ << endl;
  int pos1 = m_regexp.search(str1_);
  const QStringList cap1 = m_regexp.capturedTexts();
  int pos2 = m_regexp.search(str2_);
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
         (res = compareFloat(QString::fromLatin1("0.") + cap1[4],
                             QString::fromLatin1("0.") + cap2[4])) != 0 ? res :
         (res = cap1[5].compare(cap2[5]))                          != 0 ? res :
         (res = compareFloat(QString::fromLatin1("0.") + cap1[6],
                             QString::fromLatin1("0.") + cap2[6])) != 0 ? res :
         (res = compareFloat(cap1[7], cap2[7]))                    != 0 ? res : 0;
}

Tellico::PixmapComparison::PixmapComparison(Data::ConstFieldPtr field) : ListViewComparison(field) {
}

int Tellico::PixmapComparison::compare(int col_,
                                       const GUI::ListViewItem* item1_,
                                       const GUI::ListViewItem* item2_,
                                       bool asc_)
{
  Q_UNUSED(asc_);
  const QPixmap* pix1 = item1_->pixmap(col_);
  const QPixmap* pix2 = item2_->pixmap(col_);
  if(pix1 && !pix1->isNull()) {
    if(pix2 && !pix2->isNull()) {
      // large images come first
      return pix1->width() - pix2->width();
    }
    return 1;
  } else if(pix2 && !pix2->isNull()) {
    return -1;
  }
  return 0;
}

Tellico::DependentComparison::DependentComparison(Data::ConstFieldPtr field) : StringComparison(field) {
  Data::FieldVec fields = field->dependsOn(Data::Document::self()->collection());
  for(Data::FieldVecIt f = fields.begin(); f != fields.end(); ++f) {
    m_comparisons.append(create(f));
  }
  m_comparisons.setAutoDelete(true);
}

int Tellico::DependentComparison::compare(int col_,
                                          const GUI::ListViewItem* item1_,
                                          const GUI::ListViewItem* item2_,
                                          bool asc_)
{
  Q_UNUSED(col_);
  Q_UNUSED(asc_);
  for(QPtrListIterator<ListViewComparison> it(m_comparisons); it.current(); ++it) {
    int res = it.current()->compare(col_, item1_, item2_, asc_);
    if(res != 0) {
      return res;
    }
  }
  return ListViewComparison::compare(col_, item1_, item2_, asc_);
}

Tellico::ISODateComparison::ISODateComparison(Data::ConstFieldPtr field) : ListViewComparison(field) {
}

int Tellico::ISODateComparison::compare(const QString& str1, const QString& str2) {
  // modelled after Field::formatDate()
  // so dates would sort as expected without padding month and day with zero
  // and accounting for "current year - 1 - 1" default scheme
  QStringList dlist1 = QStringList::split('-', str1, true);
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

  QStringList dlist2 = QStringList::split('-', str2, true);
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
