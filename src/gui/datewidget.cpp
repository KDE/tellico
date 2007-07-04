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

// this class borrows heavily from kdateedit.h in the kdepim module
// which is Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
// and published under the LGPL

#include "datewidget.h"

#include <kdebug.h>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <kdatepicker.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kcalendarsystem.h>

#include <qvbox.h>
#include <qlayout.h>

using Tellico::GUI::SpinBox;
using Tellico::GUI::DateWidget;

SpinBox::SpinBox(int min, int max, QWidget *parent) : QSpinBox(min, max, 1, parent)
{
  editor()->setAlignment(AlignRight);
  // I want to be able to omit the day
  // an empty string just removes the special value, so set white space
  setSpecialValueText(QChar(' '));
}

DateWidget::DateWidget(QWidget* parent_, const char* name_) : QWidget(parent_, name_) {
  QHBoxLayout* l = new QHBoxLayout(this, 0, 4);

  KLocale* locale = KGlobal::locale();

  // 0 allows empty value
  m_daySpin = new SpinBox(0, 31, this);
  l->addWidget(m_daySpin, 1);

  m_monthCombo = new KComboBox(false, this);
  l->addWidget(m_monthCombo, 1);
  // allow empty item
  m_monthCombo->insertItem(QString::null);
  QDate d;
  for(int i = 1; ; ++i) {
    QString str = locale->calendar()->monthName(i, locale->calendar()->year(d));
    if(str.isNull()) {
      break;
    }
    m_monthCombo->insertItem(str);
  }

  m_yearSpin = new SpinBox(locale->calendar()->minValidYear(),
                           locale->calendar()->maxValidYear(), this);
  l->addWidget(m_yearSpin, 1);

  connect(m_daySpin, SIGNAL(valueChanged(int)), SLOT(slotDateChanged()));
  connect(m_monthCombo, SIGNAL(activated(int)), SLOT(slotDateChanged()));
  connect(m_yearSpin, SIGNAL(valueChanged(int)), SLOT(slotDateChanged()));

  m_dateButton = new KPushButton(this);
  m_dateButton->setIconSet(SmallIconSet(QString::fromLatin1("date")));
  connect(m_dateButton, SIGNAL(clicked()), SLOT(slotShowPicker()));
  l->addWidget(m_dateButton, 0);

  m_frame = new QVBox(0, 0, WType_Popup);
  m_frame->setFrameStyle(QFrame::PopupPanel | QFrame::Raised);
  m_frame->setLineWidth(3);
  m_frame->hide();

  m_picker = new KDatePicker(m_frame, 0); // must include name to get correct constructor
  connect(m_picker, SIGNAL(dateEntered(QDate)), SLOT(slotDateEntered(QDate)));
  connect(m_picker, SIGNAL(dateSelected(QDate)), SLOT(slotDateSelected(QDate)));
}

void DateWidget::slotDateChanged() {
  int day = m_daySpin->value();
  day = KMIN(KMAX(day, m_daySpin->minValue()), m_daySpin->maxValue());

  int m = m_monthCombo->currentItem();
  m = KMIN(KMAX(m, 0), m_monthCombo->count()-1);

  int y = m_yearSpin->value();
  y = KMIN(KMAX(y, m_yearSpin->minValue()), m_yearSpin->maxValue());

  // if all are valid, set this date
  if(day > m_daySpin->minValue() && m > 0 && y > m_yearSpin->minValue()) {
    QDate d(y, m, day);
    setDate(d);
  }
  emit signalModified();
}

QDate DateWidget::date() const {
  // possible for either day, month, or year to be empty
  // in which case a null date is returned
  int day = m_daySpin->value();
  // min value is the empty one
  if(day == m_daySpin->minValue()) {
    return QDate();
  }
  int month = m_monthCombo->currentItem();
  if(month == 0) {
    return QDate();
  }
  int year = m_yearSpin->value();
  if(year == m_yearSpin->minValue()) {
    return QDate();
  }
  return QDate(year, month, day);
}

QString DateWidget::text() const {
  // possible for either day, month, or year to be empty
  // but not all three
  bool empty = true;
  // format is "year-month-day"
  QString s;
  if(m_yearSpin->value() > m_yearSpin->minValue()) {
    s += QString::number(m_yearSpin->value());
    empty = false;
  }
  s += '-';
  // first item is empty
  if(m_monthCombo->currentItem() > 0) {
    s += QString::number(m_monthCombo->currentItem());
    empty = false;
  }
  s += '-';
  if(m_daySpin->value() > m_daySpin->minValue()) {
    s += QString::number(m_daySpin->value());
    empty = false;
  }
  return empty ? QString() : s;
}

void DateWidget::setDate(const QDate& date_) {
  m_daySpin->blockSignals(true);
  m_monthCombo->blockSignals(true);
  m_yearSpin->blockSignals(true);

  const KCalendarSystem * calendar = KGlobal::locale()->calendar();
  m_daySpin->setMaxValue(calendar->daysInMonth(date_));
  m_daySpin->setValue(calendar->day(date_));
  m_monthCombo->setCurrentItem(calendar->month(date_)); // don't subtract 1 since there's the blank first item
  m_yearSpin->setValue(calendar->year(date_));

  m_daySpin->blockSignals(false);
  m_monthCombo->blockSignals(false);
  m_yearSpin->blockSignals(false);
}

void DateWidget::setDate(const QString& date_) {
  m_daySpin->blockSignals(true);
  m_monthCombo->blockSignals(true);
  m_yearSpin->blockSignals(true);

  QStringList s = QStringList::split('-', date_, true);
  bool ok = true;
  int y = s.count() > 0 ? s[0].toInt(&ok) : m_yearSpin->minValue();
  if(!ok) {
    y = m_yearSpin->minValue();
    ok = true;
  }
  y = KMIN(KMAX(y, m_yearSpin->minValue()), m_yearSpin->maxValue());
  m_yearSpin->setValue(y);

  int m = s.count() > 1 ? s[1].toInt(&ok) : 0;
  if(!ok) {
    m = 0;
    ok = true;
  }
  m = KMIN(KMAX(m, 0), m_monthCombo->count()-1);
  m_monthCombo->setCurrentItem(m);

  // need to update number of days in month
  // for now set date to 1
  QDate date(y, (m == 0 ? 1 : m), 1);
  m_daySpin->blockSignals(true);
  m_daySpin->setMaxValue(KGlobal::locale()->calendar()->daysInMonth(date));
  m_daySpin->blockSignals(false);

  int day = s.count() > 2 ? s[2].toInt(&ok) : m_daySpin->minValue();
  if(!ok) {
    day = m_daySpin->minValue();
  }
  day = KMIN(KMAX(day, m_daySpin->minValue()), m_daySpin->maxValue());
  m_daySpin->setValue(day);

  m_daySpin->blockSignals(false);
  m_monthCombo->blockSignals(false);
  m_yearSpin->blockSignals(false);

  // if all are valid, set this date
  if(day > m_daySpin->minValue() && m > 0 && y > m_yearSpin->minValue()) {
    QDate d(y, m, day);
    m_picker->blockSignals(true);
    m_picker->setDate(d);
    m_picker->blockSignals(false);
  }
}

void DateWidget::clear() {
  m_daySpin->blockSignals(true);
  m_monthCombo->blockSignals(true);
  m_yearSpin->blockSignals(true);
  m_picker->blockSignals(true);

  m_daySpin->setValue(m_daySpin->minValue());
  m_monthCombo->setCurrentItem(0);
  m_yearSpin->setValue(m_yearSpin->minValue());
  m_picker->setDate(QDate::currentDate());

  m_daySpin->blockSignals(false);
  m_monthCombo->blockSignals(false);
  m_yearSpin->blockSignals(false);
  m_picker->blockSignals(false);
}

void DateWidget::slotShowPicker() {
  QRect desk = KGlobalSettings::desktopGeometry(this);
  QPoint popupPoint = mapToGlobal(QPoint(0, 0));

  int dateFrameHeight = m_frame->sizeHint().height();
  if(popupPoint.y() + height() + dateFrameHeight > desk.bottom()) {
    popupPoint.setY(popupPoint.y() - dateFrameHeight);
  } else {
    popupPoint.setY(popupPoint.y() + height());
  }
  int dateFrameWidth = m_frame->sizeHint().width();
  if(popupPoint.x() + dateFrameWidth > desk.right()) {
    popupPoint.setX(desk.right() - dateFrameWidth);
  }

  if(popupPoint.x() < desk.left()) {
    popupPoint.setX( desk.left());
  }
  if(popupPoint.y() < desk.top()) {
    popupPoint.setY(desk.top());
  }

  m_frame->move(popupPoint);

  QDate d = date();
  if(d.isValid()) {
    m_picker->setDate(d);
  }

  m_frame->show();
}

void DateWidget::slotDateSelected(QDate date_) {
  if(date_.isValid()) {
    setDate(date_);
    emit signalModified();
    m_frame->hide();
  }
}

void DateWidget::slotDateEntered(QDate date_) {
  if(date_.isValid()) {
    setDate(date_);
    emit signalModified();
  }
}

#include "datewidget.moc"
