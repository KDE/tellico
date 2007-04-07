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

#include <qlistview.h>
#include <qpixmap.h>

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
  if(field_->formatFlag() == Data::Field::FormatTitle) {
    return new TitleComparison();
  } else if(field_->type() == Data::Field::Number) {
    return new NumberComparison();
  } else if(field_->type() == Data::Field::Image ||
            field_->type() == Data::Field::Rating ||
            field_->type() == Data::Field::Bool) {
    // bool and ratings only have an image
    return new PixmapComparison();
  } else if(field_->name() == Latin1Literal("lcc") &&
            Data::Document::self()->collection() &&
            (Data::Document::self()->collection()->type() == Data::Collection::Book ||
             Data::Document::self()->collection()->type() == Data::Collection::Bibtex)) {
    return new LCCComparison();
  }
  return new StringComparison();
}

int Tellico::StringComparison::compare(int col_,
                                       const QListViewItem* item1_,
                                       QListViewItem* item2_,
                                       bool asc_)
{
  return item1_->key(col_, asc_).localeAwareCompare(item2_->key(col_, asc_));
}

int Tellico::TitleComparison::compare(int col_,
                                      const QListViewItem* item1_,
                                      QListViewItem* item2_,
                                      bool asc_)
{
  Q_UNUSED(asc_);
  QString title1 = Data::Field::sortKeyTitle(item1_->text(col_));
  QString title2 = Data::Field::sortKeyTitle(item2_->text(col_));
  return title1.localeAwareCompare(title2);
}

int Tellico::NumberComparison::compare(int col_,
                                       const QListViewItem* item1_,
                                       QListViewItem* item2_,
                                       bool asc_)
{
  Q_UNUSED(asc_);
  // by default, an empty string would get sorted before "1" because toFloat() turns it into "0"
  // I want the empty strings to be at the end
  bool ok1, ok2;
  // use section in case of multiple values
  float num1 = item1_->text(col_).section(';', 0, 0).toFloat(&ok1);
  float num2 = item2_->text(col_).section(';', 0, 0).toFloat(&ok2);
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

Tellico::LCCComparison::LCCComparison() : StringComparison(),
  m_regexp(QString::fromLatin1("^([A-Z]+)(\\d+(?:\\.\\d+)?)\\.?([A-Z]*)(\\d*)\\.?([A-Z]*)(\\d*)(?: (\\d\\d\\d\\d))?")) {
}

int Tellico::LCCComparison::compare(int col_,
                                    const QListViewItem* item1_,
                                    QListViewItem* item2_,
                                    bool asc_)
{
//  myDebug() << "LCCComparison::compare() - " << item1_->text(col_) << " to " << item2_->text(col_) << endl;
  int pos1 = m_regexp.search(item1_->text(col_));
  const QStringList cap1 = m_regexp.capturedTexts();
  int pos2 = m_regexp.search(item2_->text(col_));
  const QStringList cap2 = m_regexp.capturedTexts();
  if(pos1 > -1 && pos2 > -1) {
    int res = compareLCC(cap1, cap2);
//    myLog() << "...result = " << res << endl;
    return res;
  }
  return StringComparison::compare(col_, item1_, item2_, asc_);
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

int Tellico::PixmapComparison::compare(int col_,
                                       const QListViewItem* item1_,
                                       QListViewItem* item2_,
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

