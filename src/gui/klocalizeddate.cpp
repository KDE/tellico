/*
    Copyright 2010 John Layt <john@layt.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "klocalizeddate.h"

#include <kglobal.h>
#include <kdebug.h>

/*****************************************************************************
 *
 *                               Private Section
 *
 *****************************************************************************/

class KLocalizedDatePrivate : public QSharedData
{
public:
    explicit KLocalizedDatePrivate(const QDate &date, const KCalendarSystem *calendar, bool manageCalendar);
    KLocalizedDatePrivate(const KLocalizedDatePrivate &rhs);
    KLocalizedDatePrivate &operator=(const KLocalizedDatePrivate &rhs);
    virtual ~KLocalizedDatePrivate();

    QDate m_date;
    const KCalendarSystem *m_calendar;
    bool m_manageCalendar;
};

KLocalizedDatePrivate::KLocalizedDatePrivate(const QDate &date, const KCalendarSystem *calendar, bool manageCalendar)
            : QSharedData(),
              m_date(date),
              m_calendar(calendar),
              m_manageCalendar(manageCalendar)
{
}

KLocalizedDatePrivate::KLocalizedDatePrivate(const KLocalizedDatePrivate &rhs)
            : QSharedData(rhs),
              m_date(rhs.m_date),
              m_calendar(rhs.m_calendar),
              m_manageCalendar(rhs.m_manageCalendar)
{
    // If we're managing the calendar object, then take a copy,
    // i.e. user called setCalendarSystem() rather than passing a custom one into the constructor
    if(m_manageCalendar) {
        m_calendar =  KCalendarSystem::create(m_calendar->calendarSystem(), new KLocale(*m_calendar->locale()));
    }
}

KLocalizedDatePrivate &KLocalizedDatePrivate::operator=(const KLocalizedDatePrivate &rhs)
{
    m_date = rhs.m_date;
    m_calendar = rhs.m_calendar;
    m_manageCalendar = rhs.m_manageCalendar;
    // If we're managing the calendar object, then take a copy,
    // i.e. user called setCalendarSystem() rather than passing a custom one into the constructor
    if(rhs.m_manageCalendar) {
        m_calendar =  KCalendarSystem::create(m_calendar->calendarSystem(), new KLocale(*m_calendar->locale()));
    }
    return *this;
}

KLocalizedDatePrivate::~KLocalizedDatePrivate()
{
    // If we're managing the calendar object, then delete it,
    // i.e. user called setCalendarSystem() rather than passing a custom one into the constructor
    if (m_manageCalendar) {
        delete m_calendar;
    }
}

/*****************************************************************************
 *
 *                            Date Creation Section
 *
 *****************************************************************************/

KLocalizedDate::KLocalizedDate(const QDate &date, const KCalendarSystem *calendarSystem)
     : d(new KLocalizedDatePrivate(date, calendarSystem, false))
{
}

KLocalizedDate::KLocalizedDate(int year, int month, int day, const KCalendarSystem *calendarSystem)
     : d(new KLocalizedDatePrivate(QDate(), calendarSystem, false))
{
    setDate(year, month, day);
}

KLocalizedDate::KLocalizedDate(const KLocalizedDate &rhs)
     : d(new KLocalizedDatePrivate(*rhs.d))
{
}

KLocalizedDate &KLocalizedDate::operator=(const KLocalizedDate &rhs)
{
    *d = *rhs.d;
    return *this;
}

KLocalizedDate &KLocalizedDate::operator=(const QDate &rhs)
{
    d->m_date = rhs;
    return *this;
}

KLocalizedDate::~KLocalizedDate()
{
}

/*****************************************************************************
 *
 *                           Calendar System Section
 *
 *****************************************************************************/

void KLocalizedDate::setCalendarSystem(KLocale::CalendarSystem calendarSystem)
{
    if (calendarSystem == calendar()->calendarSystem()) {
        return;
    }
    KCalendarSystem *newCalendar =  KCalendarSystem::create(calendarSystem,
                                                            new KLocale(*calendar()->locale()));
    if (d->m_manageCalendar) {
        delete d->m_calendar;
    }
    d->m_calendar = newCalendar;
}

KLocale::CalendarSystem KLocalizedDate::calendarSystem()
{
    return calendar()->calendarSystem();
}

const KCalendarSystem *KLocalizedDate::calendar() const
{
    if ( d->m_calendar ) {
        return d->m_calendar;
    }
    return  KGlobal::locale()->calendar();
}

/*****************************************************************************
 *
 *                           Date Status Section
 *
 *****************************************************************************/

bool KLocalizedDate::isNull() const
{
    return date().isNull();
}

bool KLocalizedDate::isValid() const
{
    return calendar()->isValid( date() );
}

/*****************************************************************************
 *
 *                           Date Setting Section
 *
 *****************************************************************************/

bool KLocalizedDate::setDate(const QDate &date)
{
    d->m_date = date;
    return isValid();
}

bool KLocalizedDate::setDate(int year, int month, int day)
{
    calendar()->setDate(d->m_date, year, month, day);
    return isValid();
}

bool KLocalizedDate::setDate(int year, int dayOfYear)
{
    calendar()->setDate(d->m_date, year, dayOfYear);
    return isValid();
}

bool KLocalizedDate::setDate(QString eraName, int yearInEra, int month, int day)
{
    calendar()->setDate(d->m_date, eraName, yearInEra, month, day);
    return isValid();
}

bool KLocalizedDate::setDate(KLocale::WeekNumberSystem weekNumberSystem, int year, int isoWeekNumber, int dayOfIsoWeek)
{
    Q_UNUSED(weekNumberSystem); // Only support ISO Week at the moment
    calendar()->setDateIsoWeek(d->m_date, year, isoWeekNumber, dayOfIsoWeek);
    return isValid();
}

bool KLocalizedDate::setCurrentDate()
{
    d->m_date = QDate::currentDate();
    return isValid();
}

/*****************************************************************************
 *
 *                        Static Date Creation Section
 *
 *****************************************************************************/

KLocalizedDate KLocalizedDate::currentDate()
{
    return KLocalizedDate(QDate::currentDate());
}

KLocalizedDate KLocalizedDate::fromDate(const QDate &date)
{
    return KLocalizedDate(date);
}

KLocalizedDate KLocalizedDate::fromJulianDay(int jd)
{
    return KLocalizedDate(QDate::fromJulianDay(jd));
}

/*****************************************************************************
 *
 *                             Date Componant Section
 *
 *****************************************************************************/

int KLocalizedDate::toJulianDay() const
{
    return d->m_date.toJulianDay();
}

QDate KLocalizedDate::date() const
{
    return d->m_date;
}

void KLocalizedDate::getDate(int *year, int *month, int *day) const
{
    calendar()->getDate(date(), year, month, day);
}

int KLocalizedDate::year() const
{
    return calendar()->year(date());
}

int KLocalizedDate::month() const
{
    return calendar()->month(date());
}

int KLocalizedDate::day() const
{
    return calendar()->day(date());
}

QString KLocalizedDate::eraName() const
{
    return formatDate(KLocale::EraName);
}

QString KLocalizedDate::eraYear() const
{
    return formatDate(KLocale::EraYear);
}

int KLocalizedDate::yearInEra() const
{
    return calendar()->yearInEra(date());
}

int KLocalizedDate::dayOfYear() const
{
    return calendar()->dayOfYear(date());
}

int KLocalizedDate::dayOfWeek() const
{
    return calendar()->dayOfWeek(date());
}

int KLocalizedDate::week(int *yearNum) const
{
    return calendar()->weekNumber(date(), yearNum);
}

int KLocalizedDate::week(KLocale::WeekNumberSystem weekNumberSystem, int *yearNum) const
{
    Q_UNUSED(weekNumberSystem);
    return calendar()->weekNumber(date(), yearNum);
}

int KLocalizedDate::monthsInYear() const
{
    return calendar()->monthsInYear(date());
}

int KLocalizedDate::weeksInYear() const
{
    return calendar()->weeksInYear(date());
}

int KLocalizedDate::weeksInYear(KLocale::WeekNumberSystem weekNumberSystem) const
{
    Q_UNUSED(weekNumberSystem);
    return calendar()->weeksInYear(date());
}

int KLocalizedDate::daysInYear() const
{
    return calendar()->daysInYear(date());
}

int KLocalizedDate::daysInMonth() const
{
    return calendar()->daysInMonth(date());
}

int KLocalizedDate::daysInWeek() const
{
    return calendar()->daysInWeek(date());
}

bool KLocalizedDate::isLeapYear() const
{
    return calendar()->isLeapYear(date());
}

/*****************************************************************************
 *
 *                             Date Formatting Section
 *
 *****************************************************************************/

QString KLocalizedDate::formatDate(KLocale::DateFormat toFormat) const
{
    return calendar()->formatDate(date(), toFormat);
}

QString KLocalizedDate::formatDate(const QString &toFormat, KLocale::DateTimeFormatStandard formatStandard) const
{
    return calendar()->formatDate(date(), toFormat, formatStandard);
}

QString KLocalizedDate::formatDate(KLocale::DateTimeComponent component,
                                   KLocale::DateTimeComponentFormat format,
                                   KLocale::WeekNumberSystem weekNumberSystem) const
{
    return calendar()->formatDate(date(), component, format, weekNumberSystem);
}

/*****************************************************************************
 *
 *                             Date Parsing Section
 *
 *****************************************************************************/

KLocalizedDate KLocalizedDate::readDate(const QString &dateString,
                                        KLocale::DateTimeParseMode parseMode,
                                        const KCalendarSystem *calendar)
{
    Q_UNUSED(parseMode);
    if (!calendar) {
        calendar = KGlobal::locale()->calendar();
    }
    return KLocalizedDate(calendar->readDate(dateString));
}

KLocalizedDate KLocalizedDate::readDate(const QString &dateString,
                                        KLocale::ReadDateFlags formatFlags,
                                        KLocale::DateTimeParseMode parseMode,
                                        const KCalendarSystem *calendar)
{
    Q_UNUSED(parseMode);
    if (!calendar) {
        calendar = KGlobal::locale()->calendar();
    }
    return KLocalizedDate(calendar->readDate(dateString, formatFlags));
}

KLocalizedDate KLocalizedDate::readDate(const QString &dateString,
                                        const QString &dateFormat,
                                        KLocale::DateTimeParseMode parseMode,
                                        KLocale::DateTimeFormatStandard formatStandard,
                                        const KCalendarSystem *calendar)
{
    Q_UNUSED(parseMode);
    if (!calendar) {
        calendar = KGlobal::locale()->calendar();
    }
    return KLocalizedDate(calendar->readDate(dateString, dateFormat, 0, formatStandard));
}

/*****************************************************************************
 *
 *                             Date Maths Section
 *
 *****************************************************************************/

KLocalizedDate KLocalizedDate::addYears(int years) const
{
    KLocalizedDate newDate;
    newDate = *this;
    newDate.setDate(calendar()->addYears(date(), years));
    return newDate;
}

bool KLocalizedDate::addYearsTo(int years)
{
    d->m_date = calendar()->addYears(date(), years);
    return isValid();
}

KLocalizedDate KLocalizedDate::addMonths(int months) const
{
    KLocalizedDate newDate(*this);
    newDate.setDate(calendar()->addMonths(date(), months));
    return newDate;
}

bool KLocalizedDate::addMonthsTo(int months)
{
    d->m_date = calendar()->addMonths(date(), months);
    return isValid();
}

KLocalizedDate KLocalizedDate::addDays(int days) const
{
    KLocalizedDate newDate(*this);
    newDate.setDate(calendar()->addDays(date(), days));
    return newDate;
}

bool KLocalizedDate::addDaysTo(int days)
{
    d->m_date = calendar()->addDays(date(), days);
    return isValid();
}

void KLocalizedDate::dateDifference(const KLocalizedDate &toDate,
                           int *yearsDiff, int *monthsDiff, int *daysDiff, int *direction) const
{
    dateDifference(toDate.date(), yearsDiff, monthsDiff, daysDiff, direction);
}

void KLocalizedDate::dateDifference(const QDate &toDate,
                           int *yearsDiff, int *monthsDiff, int *daysDiff, int *direction) const
{
    calendar()->dateDifference(date(), toDate, yearsDiff, monthsDiff, daysDiff, direction);
}

int KLocalizedDate::yearsDifference(const KLocalizedDate &toDate) const
{
    return yearsDifference(toDate.date());
}

int KLocalizedDate::yearsDifference(const QDate &toDate) const
{
    return calendar()->yearsDifference(date(), toDate);
}

int KLocalizedDate::monthsDifference(const KLocalizedDate &toDate) const
{
    return monthsDifference(toDate.date());
}

int KLocalizedDate::monthsDifference(const QDate &toDate) const
{
    return calendar()->monthsDifference(date(), toDate);
}

int KLocalizedDate::daysDifference(const KLocalizedDate &toDate) const
{
    return daysDifference(toDate.date());
}

int KLocalizedDate::daysDifference(const QDate &toDate) const
{
    return calendar()->daysDifference(date(), toDate);
}

KLocalizedDate KLocalizedDate::firstDayOfYear() const
{
    KLocalizedDate newDate(*this);
    newDate.setDate(calendar()->firstDayOfYear(date()));
    return newDate;
}

KLocalizedDate KLocalizedDate::lastDayOfYear() const
{
    KLocalizedDate newDate(*this);
    newDate.setDate(calendar()->lastDayOfYear(date()));
    return newDate;
}

KLocalizedDate KLocalizedDate::firstDayOfMonth() const
{
    KLocalizedDate newDate(*this);
    newDate.setDate(calendar()->firstDayOfMonth(date()));
    return newDate;
}

KLocalizedDate KLocalizedDate::lastDayOfMonth() const
{
    KLocalizedDate newDate(*this);
    newDate.setDate(calendar()->lastDayOfMonth(date()));
    return newDate;
}

/*****************************************************************************
 *
 *                             Date Operators Section
 *
 *****************************************************************************/

bool KLocalizedDate::operator==(const KLocalizedDate &rhs) const
{
    return (date() == rhs.date());
}

bool KLocalizedDate::operator==(const QDate &rhs) const
{
    return (date() == rhs);
}

bool KLocalizedDate::operator!=(const KLocalizedDate &rhs) const
{
    return (date() != rhs.date());
}

bool KLocalizedDate::operator!=(const QDate &rhs) const
{
    return (date() != rhs);
}

bool KLocalizedDate::operator<(const KLocalizedDate &rhs) const
{
    return (date() < rhs.date());
}

bool KLocalizedDate::operator<(const QDate &rhs) const
{
    return (date() < rhs);
}

bool KLocalizedDate::operator<=(const KLocalizedDate &rhs) const
{
    return (d->m_date <= rhs.date());
}

bool KLocalizedDate::operator<=(const QDate &rhs) const
{
    return (date() <= rhs);
}

bool KLocalizedDate::operator>(const KLocalizedDate &rhs) const
{
    return (date() > rhs.date());
}

bool KLocalizedDate::operator>(const QDate &rhs) const
{
    return (date() > rhs);
}

bool KLocalizedDate::operator>=(const KLocalizedDate &rhs) const
{
    return (date() >= rhs.date());
}

bool KLocalizedDate::operator>=(const QDate &rhs) const
{
    return (date() >= rhs);
}

QDataStream &operator<<(QDataStream &out, const KLocalizedDate &date)
{
    return out << (quint32)(date.toJulianDay()) << date.calendar()->calendarSystem();
}

QDataStream &operator>>(QDataStream &in, KLocalizedDate &date)
{
    quint32 jd;
    int calendarSystem;
    in >> jd >> calendarSystem;
    date.setDate(QDate::fromJulianDay(jd));
    date.setCalendarSystem((KLocale::CalendarSystem)calendarSystem);
    return in;
}

QDebug operator<<(QDebug dbg, const KLocalizedDate &date)
{
    if (date.calendar()->calendarType() == QLatin1String("gregorian")) {
        dbg.nospace() << "KLocalizedDate(" << date.formatDate(KLocale::IsoDate) << ", "
                      << date.calendar()->calendarLabel() << ')';
    } else {
        dbg.nospace() << "KLocalizedDate(" << date.formatDate(KLocale::IsoDate) << ", "
                      << date.calendar()->calendarLabel() << ')'
                      << " = QDate(" << date.date().toString(Qt::ISODate) << ')';
    }
    return dbg.space();
}
