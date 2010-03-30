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

// this class borrows heavily from kdateedit.h in the kdepim module
// which is Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
// and published under the LGPL

#include "datewidget.h"
#include "spinbox.h"

#include <kcombobox.h>
#include <kpushbutton.h>
#include <kdatepicker.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kcalendarsystem.h>

#include <QHBoxLayout>
#include <QFrame>
#include <QEvent>
#include <QMenu>
#include <QWidgetAction>

using Tellico::GUI::DateWidget;

class DateWidget::DatePickerAction : public QWidgetAction
{
  public:
    DatePickerAction( KDatePicker *widget, QObject *parent )
      : QWidgetAction( parent ),
        mDatePicker( widget ), mOriginalParent( widget->parentWidget() )
    {
    }

  protected:
    QWidget *createWidget( QWidget *parent )
    {
      mDatePicker->setParent( parent );
      return mDatePicker;
    }

    void deleteWidget( QWidget *widget )
    {
      if ( widget != mDatePicker ) {
        return;
      }

      mDatePicker->setParent( mOriginalParent );
    }

  private:
    KDatePicker *mDatePicker;
    QWidget *mOriginalParent;
};

DateWidget::DateWidget(QWidget* parent_) : QWidget(parent_) {
  QBoxLayout* l = new QHBoxLayout(this);
  l->setContentsMargins(0, 0, 0, 0);

  KLocale* locale = KGlobal::locale();

  // 0 allows empty value
  m_daySpin = new SpinBox(0, 31, this);
  l->addWidget(m_daySpin, 1);
  l->setStretchFactor(m_daySpin, 1);

  m_monthCombo = new KComboBox(false, this);
  l->addWidget(m_monthCombo, 1);
  l->setStretchFactor(m_monthCombo, 1);
  // allow empty item
  m_monthCombo->addItem(QString());
  QDate d;
  for(int i = 1; ; ++i) {
    QString str = locale->calendar()->monthName(i, locale->calendar()->year(d));
    if(str.isEmpty()) {
      break;
    }
    m_monthCombo->addItem(str);
  }

  m_yearSpin = new SpinBox(locale->calendar()->earliestValidDate().year(),
                           locale->calendar()->latestValidDate().year(), this);
  l->addWidget(m_yearSpin, 1);
  l->setStretchFactor(m_yearSpin, 1);

  connect(m_daySpin, SIGNAL(valueChanged(int)), SLOT(slotDateChanged()));
  connect(m_monthCombo, SIGNAL(activated(int)), SLOT(slotDateChanged()));
  connect(m_yearSpin, SIGNAL(valueChanged(int)), SLOT(slotDateChanged()));

  m_dateButton = new KPushButton(this);
  m_dateButton->setIcon(KIcon(QLatin1String("view-pim-calendar")));
  connect(m_dateButton, SIGNAL(clicked()), SLOT(slotShowPicker()));
  l->addWidget(m_dateButton, 0);

  m_menu = new QMenu(this);
  m_menu->hide();

  m_picker = new KDatePicker(m_menu);
  m_picker->setCloseButton(false);
  connect(m_picker, SIGNAL(dateEntered(QDate)), SLOT(slotDateEntered(QDate)));
  connect(m_picker, SIGNAL(dateSelected(QDate)), SLOT(slotDateSelected(QDate)));

  m_menu->addAction(new DatePickerAction(m_picker, m_menu));
}

DateWidget::~DateWidget() {
}

void DateWidget::slotDateChanged() {
  int day = m_daySpin->value();
  day = qMin(qMax(day, m_daySpin->minimum()), m_daySpin->maximum());

  int m = m_monthCombo->currentIndex();
  m = qMin(qMax(m, 0), m_monthCombo->count()-1);

  int y = m_yearSpin->value();
  y = qMin(qMax(y, m_yearSpin->minimum()), m_yearSpin->maximum());

  // if all are valid, set this date
  if(day > m_daySpin->minimum() && m > 0 && y > m_yearSpin->minimum()) {
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
  if(day == m_daySpin->minimum()) {
    return QDate();
  }
  int month = m_monthCombo->currentIndex();
  if(month == 0) {
    return QDate();
  }
  int year = m_yearSpin->value();
  if(year == m_yearSpin->minimum()) {
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
  if(m_yearSpin->value() > m_yearSpin->minimum()) {
    s += QString::number(m_yearSpin->value());
    empty = false;
  }
  s += QLatin1Char('-');
  // first item is empty
  if(m_monthCombo->currentIndex() > 0) {
    s += QString::number(m_monthCombo->currentIndex());
    empty = false;
  }
  s += QLatin1Char('-');
  if(m_daySpin->value() > m_daySpin->minimum()) {
    s += QString::number(m_daySpin->value());
    empty = false;
  }
  return empty ? QString() : s;
}

void DateWidget::setDate(const QDate& date_) {
  m_daySpin->blockSignals(true);
  m_monthCombo->blockSignals(true);
  m_yearSpin->blockSignals(true);

  const KCalendarSystem* calendar = KGlobal::locale()->calendar();
  m_daySpin->setMaximum(calendar->daysInMonth(date_));
  m_daySpin->setValue(calendar->day(date_));
  m_monthCombo->setCurrentIndex(calendar->month(date_)); // don't subtract 1 since there's the blank first item
  m_yearSpin->setValue(calendar->year(date_));

  m_daySpin->blockSignals(false);
  m_monthCombo->blockSignals(false);
  m_yearSpin->blockSignals(false);
}

void DateWidget::setDate(const QString& date_) {
  m_daySpin->blockSignals(true);
  m_monthCombo->blockSignals(true);
  m_yearSpin->blockSignals(true);

  QStringList s = date_.split(QLatin1Char('-'));
  bool ok = true;
  int y = s.count() > 0 ? s[0].toInt(&ok) : m_yearSpin->minimum();
  if(!ok) {
    y = m_yearSpin->minimum();
    ok = true;
  }
  y = qMin(qMax(y, m_yearSpin->minimum()), m_yearSpin->maximum());
  m_yearSpin->setValue(y);

  int m = s.count() > 1 ? s[1].toInt(&ok) : 0;
  if(!ok) {
    m = 0;
    ok = true;
  }
  m = qMin(qMax(m, 0), m_monthCombo->count()-1);
  m_monthCombo->setCurrentIndex(m);

  // need to update number of days in month
  // for now set date to 1
  QDate date(y, (m == 0 ? 1 : m), 1);
  m_daySpin->blockSignals(true);
  m_daySpin->setMaximum(KGlobal::locale()->calendar()->daysInMonth(date));
  m_daySpin->blockSignals(false);

  int day = s.count() > 2 ? s[2].toInt(&ok) : m_daySpin->minimum();
  if(!ok) {
    day = m_daySpin->minimum();
  }
  day = qMin(qMax(day, m_daySpin->minimum()), m_daySpin->maximum());
  m_daySpin->setValue(day);

  m_daySpin->blockSignals(false);
  m_monthCombo->blockSignals(false);
  m_yearSpin->blockSignals(false);

  // if all are valid, set this date
  if(day > m_daySpin->minimum() && m > 0 && y > m_yearSpin->minimum()) {
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

  m_daySpin->setValue(m_daySpin->minimum());
  m_monthCombo->setCurrentIndex(0);
  m_yearSpin->setValue(m_yearSpin->minimum());
  m_picker->setDate(QDate::currentDate());

  m_daySpin->blockSignals(false);
  m_monthCombo->blockSignals(false);
  m_yearSpin->blockSignals(false);
  m_picker->blockSignals(false);
}

void DateWidget::slotShowPicker() {
  QRect desk = KGlobalSettings::desktopGeometry(this);
  QPoint popupPoint = mapToGlobal(QPoint(0, 0));

  int dateFrameHeight = m_menu->sizeHint().height();
  if(popupPoint.y() + height() + dateFrameHeight > desk.bottom()) {
    popupPoint.setY(popupPoint.y() - dateFrameHeight);
  } else {
    popupPoint.setY(popupPoint.y() + height());
  }
  int dateFrameWidth = m_menu->sizeHint().width();
  if(popupPoint.x() + width() > desk.right()) {
    popupPoint.setX(desk.right() - dateFrameWidth);
  } else {
    popupPoint.setX(popupPoint.x() + width() - dateFrameWidth);
  }

  if(popupPoint.x() < desk.left()) {
    popupPoint.setX( desk.left());
  }
  if(popupPoint.y() < desk.top()) {
    popupPoint.setY(desk.top());
  }

  QDate d = date();
  if(d.isValid()) {
    m_picker->setDate(d);
  }

  m_menu->popup(popupPoint);
}

void DateWidget::slotDateSelected(const QDate& date_) {
  if(date_.isValid()) {
    setDate(date_);
    emit signalModified();
    m_menu->hide();
  }
}

void DateWidget::slotDateEntered(const QDate& date_) {
  if(date_.isValid()) {
    setDate(date_);
    emit signalModified();
  }
}

#include "datewidget.moc"
