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

#ifndef KDATE_H
#define KDATE_H

#include <QtCore/QString>
#include <QtCore/QDate>

#include <kcalendarsystem.h>
#include <klocale.h>

class KLocalizedDatePrivate;
/**
 * @short A class representing a date localized using the local calendar system, language and formats
 *
 * Topics:
 *  - @ref intro
 *  - @ref calsys
 *  - @ref custom
 *  - @ref formatting
 *  - @ref maths
 *
 * @section intro Introduction
 *
 * This class provides a simple and convenient way to localize dates
 *
 * @section calsys Calendar System
 *
 * KDE supports the use of different calendar systems.
 *
 * @section custom Default and Custom Locale and Calendar System
 *
 * In most cases you will want to use the default Global Locale and Calendar
 * System, in which case you need only create a default KLocalizedDate.  If
 * however you specifically need a different Calendar System or Locale settings
 * then you need to take some extra steps.
 *
 * The simplest method is just changing the Calendar System while keeping the
 * current Locale settings.  This is easily done using setCalendarSystem()
 * which will copy the current Locale being used and apply this to the new
 * Calendar System.  Note this means any changes to the old locale settings,
 * either the Global Locale or a custom Locale (see below) will not apply
 * to that date instance.
 *
 * You may however wish to use a custom Locale with the Calendar System.
 * For example, if you want your app to normally show dates using the Global
 * Locale and Calendar System, but wish to show an info box with the Islamic
 * date in Arabic language and format, then you need a custom Locale to do
 * this.
 *
 * \code
 * KLocale *myLocale = new KLocale("myapp", "ar", "eg");
 * KCalendarSystem *myCalendar = KCalendarSystem::create(KLocale::IslamicCivilCalendar, myLocale);
 * KLocalizedDate myDate(QDate(2010,1,1), myCalendar);
 * \endcode
 *
 * In this case you are responsible for the memory management of the KLocale
 * and KCalendarSystem.  This allows you to reuse this calendar across multiple
 * date instances without it being deleted under you.  It also allows you to
 * change any setting in the Locale and have it apply across all those date
 * instances.  @warning Don't try changing the Calendar System via your Locale
 * instance, your KCalendarSystem instance will be deleted and all the dates
 * will be invalid!
 *
 * @see 
 *
 * @section formatting Date Formatting
 *
 * When you display dates or date components to users in a GUI, they will
 * expect them to be displayed in their language and digit set following their
 * local date formatting conventions.  Directly displaying values returned by
 * the normal date component methods such as day() will not conform to these
 * expectations, so you need to use different methods to obtain the localized
 * string form of the date or component.
 *
 * You can either format the entire date, or just a single component of the
 * date such as the month or day.
 *
 * When formatting a full date, it is preferred to use one of the standard date
 * formats defined in the Locale, although you can provide your own format in
 * either the KDE, POSIX, or UNICODE standards.
 *
 * @see formatDate() formatDate()
 *
 * @section parsing Date Parsing
 *
 * Basic concepts on date parsing, then full details on KLocale::ReadDateFlags
 * formats, definging your own date format strings, and setting how strictly
 * the format is appplied.
 *
 * You can choose how strictly a date format is applied in parsing.  Currently
 * only liberal Parsing is supported.
 *
 * The KLocale::LiberalParsing mode applies the following rules:
 * 
 * 1) You must supply a format and string containing at least one of the following combinations to
 * create a valid date:
 * @li a month and day of month
 * @li a day of year
 * @li a ISO week number and day of week
 *
 * 2) If a year number is not supplied then the current year will be assumed.
 *
 * 3) All date components must be separated by a non-numeric character.
 *
 * 4) The format is not applied strictly to the input string:
 * @li extra whitespace is ignored
 * @li leading 0's on numbers are ignored
 * @li capitalisation of literals is ignored
 *
 * @see readDate()
 *
 * @section maths Date Maths
 *
 * A full set of date maths functions are provided which operate in a consistent
 * manner, i.e. you can safely round-trip.
 *
 */

class KLocalizedDate
{
public:

    /**
     * Constructs a localized date with the given date.
     *
     * By default, uses the global Calendar System and Locale.
     *
     * If you pass in a custom Calendar System then you retain ownership of it
     * and are responsible for deleting it.  This allows you to reuse the same
     * custom Calendar System for many localized date instances.
     *
     * See @ref custom for more details on using custom Calendar Systems.
     *
     * @param date the QDate to set the KLocalizedDate to, defaults to invalid date
     * @param locale the locale to use for date formats, defaults to the global
     */
    explicit KLocalizedDate(const QDate &date = QDate(), const KCalendarSystem *calendar = 0);

    /**
     * Constructs a localized date with the given year, month and day.
     *
     * By default, uses the global Calendar System and Locale.
     *
     * If you pass in a custom Calendar System then you retain ownership of it
     * and are responsible for deleting it.  This allows you to reuse the same
     * custom Calendar System for many localized date instances.
     *
     * See @ref custom for more details on using custom Calendar Systems.
     *
     * @param year the year to set the KLocalizedDate to
     * @param month the month to set the KLocalizedDate to
     * @param day the day to set the KLocalizedDate to
     */
    KLocalizedDate(int year, int month, int day, const KCalendarSystem *calendar = 0);

    /**
     * Copy constructor
     *
     * @param rhs the date to copy
     */
    KLocalizedDate(const KLocalizedDate &rhs);

    /**
     * Assignment operator
     *
     * @param rhs the date to assign
     */
    KLocalizedDate &operator=(const KLocalizedDate &rhs);

    /**
     * Assignment operator
     *
     * @param rhs the date to assign
     */
    KLocalizedDate &operator=(const QDate &rhs);

    /**
     * Destructor.
     */
    ~KLocalizedDate();

    /**
     * Set the Calendar System used for this date instance only.
     *
     * This method is mostly useful for when you quickly want to see what the
     * currently set date would look like in a different Calendar System but
     * using the same Locale.
     *
     * When the Calendar System is changed, a copy will be taken of the Locale
     * previously used and this copy will be applied to the new Calendar System.
     * Any changes to the old Locale settings, either the Global or a Custom
     * Locale, will not be applied to this date instance.
     *
     * See @ref custom for more details on using custom Calendar Systems.
     *
     * @see KLocale::CalendarSystem
     * @see calendarSystem()
     * @param calendarSystem the Calendar System to use
     */
    void setCalendarSystem(KLocale::CalendarSystem calendarSystem);

    /**
     * Returns the Calendar System used by this localized date instance
     *
     * @see KLocale::CalendarSystem
     * @see setCalendarSystem()
     * @return the Calendar System currently used
     */
    KLocale::CalendarSystem calendarSystem();

    /**
     * Returns a pointer to the Calendar System object used by this date instance.
     *
     * Usually this will be the Global Calendar System, but this may have been
     * changed.
     *
     * Normally you will not need to access this object unless the KLocalizedDate
     * API does not provide the methods you require.
     *
     * @see KCalendarSystem
     * @see calendarSystem
     * @see setCalendarSystem
     * @return the current calendar system instance
     */
    const KCalendarSystem *calendar() const;

    /**
     * Returns whether the date is null, i.e. invalid in any Calendar System.
     *
     * @see isValid
     * @return @c true if the date is null, @c false otherwise
     */
    bool isNull() const;

    /**
     * Returns whether the date is valid in the current Calendar System.
     *
     * @see isNull
     * @return @c true if the date is valid, @c false otherwise
     */
    bool isValid() const;

    /**
     * Set the date using a QDate.
     *
     * @param date the QDate to set to
     * @return @c true if the date is valid, @c false otherwise
     */
    bool setDate(const QDate &date);

    /**
     * Set the date's year, month and day.
     *
     * The range of the year, month and day, and the validity of the date as a
     * whole depends on which Calendar System is being used.
     *
     * @see getDate()
     * @param year year number
     * @param month month of year number
     * @param day day of month
     * @return @c true if the date is valid, @c false otherwise
     */
    bool setDate(int year, int month, int day);

    /**
     * Set the date using the year number and day of year number only.
     *
     * @see dayOfYear()
     * @param year year
     * @param dayOfYear day of year
     * @return @c true if the date is valid, @c false otherwise
     */
    bool setDate(int year, int dayOfYear);

    /**
     * Set the date using the era, year in era number, month and day
     *
     * @see eraName()
     * @see yearInEra()
     * @param eraName Era string
     * @param year Year In Era number
     * @param month Month number
     * @param day Day Of Month number
     * @return @c true if the date is valid, @c false otherwise
     */
    bool setDate(QString eraName, int yearInEra, int month, int day);

    /**
     * Set the date using the year, week and day of week.
     *
     * Currently only the ISO Week Number System is supported.
     *
     * @see week()
     * @see dayOfWeek()
     * @param weekNumberSystem the week number system to use
     * @param year year
     * @param weekOfYear week of year
     * @param dayOfWeek day of week Mon..Sun (1..7)
     * @return @c true if the date is valid, @c false otherwise
     */
    bool setDate(KLocale::WeekNumberSystem weekNumberSystem, int year, int weekOfYear, int dayOfWeek);

    /**
     * Set the date to today's date
     *
     * @see currentDate()
     * @return @c true if the date is valid, @c false otherwise
     */
    bool setCurrentDate();

    /**
     * Returns a KLocalizedDate set to today's date in the Global Locale and
     * Calendar System.
     *
     * @see setCurrentDate()
     * @return today's localized date
     */
    static KLocalizedDate currentDate();

    /**
     * Returns a KLocalizedDate set the required date in the Global Locale and
     * Calendar System.
     *
     * @param date the date to set to
     * @return a localized date
     */
    static KLocalizedDate fromDate(const QDate &date);

    /**
     * Returns a KLocalizedDate set the required Julian Day number in the Global
     * Locale and Calendar System.
     *
     * @see toJulianDay()
     * @param jd the Julian Day number to set to
     * @return a localized date
     */
    static KLocalizedDate fromJulianDay(int jd);

    /**
     * Returns the currently set date as a Julian Day number
     *
     * @see fromJulianDay()
     * @return the currently set date as a Julian Day number
     */
    int toJulianDay() const;

    /**
     * Returns the currently set date as a QDate
     *
     * @return the currently set date as a QDate
     */
    QDate date() const;

    /**
     * Returns the year, month and day portion of the date in the current
     * Calendar System.
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see setDate()
     * @see formatDate()
     * @param year year number returned in this variable
     * @param month month number returned in this variable
     * @param day day of month returned in this variable
     */
    void getDate(int *year, int *month, int *day) const;

    /**
     * Returns the year portion of the date in the current calendar system
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return the localized year number
     */
    int year() const;

    /**
     * Returns the month portion of the date in the current calendar system
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return the localized month number, 0 if date is invalid
     */
    int month() const;

    /**
     * Returns the day portion of the date in the current calendar system
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return the localized day number, 0 if date is invalid
     */
    int day() const;

    /**
     * Returns the Era Name portion of the date in the current calendar system,
     * for example "AD" or "Anno Domini" for the Gregorian calendar and Christian Era.
     *
     * See @ref formatting for more details on Date Formatting.
     *
     * @see formatDate()
     * @param format format to return, either short or long
     * @return the localized era name, empty string if date is invalid
     */
    QString eraName() const;

    /**
     * Returns the Era Year portion of the date in the current
     * calendar system, for example "2000 AD" or "Heisei 22".
     *
     * See @ref formatting for more details on Date Formatting.
     *
     * @see formatDate()
     * @param format format to return, either short or long
     * @return the localized era year string, empty string if date is invalid
     */
    QString eraYear() const;

    /**
     * Returns the Year In Era portion of the date in the current calendar
     * system, for example 1 for "1 BC".
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @see formatYearInEra()
     * @return the localized Year In Era number, -1 if date is invalid
     */
    int yearInEra() const;

    /**
     * Returns the day number of year for the date
     *
     * The days are numbered 1..daysInYear()
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return day of year number, -1 if date not valid
     */
    int dayOfYear() const;

    /**
     * Returns the weekday number for the date
     *
     * The weekdays are numbered 1..7 for Monday..Sunday.
     *
     * This value is @em not affected by the value of KLocale::weekStartDay()
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return day of week number, -1 if date not valid
     */
    int dayOfWeek() const;

    /**
     * Returns the localized Week Number for the date.
     *
     * See @ref formatting for why you should never display this value.
     *
     * Currently only the ISO Week Number is supported, but in future the US
     * and other week number systems will be added.
     *
     * If you specifically require the ISO Week, you should use
     * week(KLocale::IsoWeekNumber)
     *
     * ISO 8601 defines the first week of the year as the week containing the first Thursday.
     * See http://en.wikipedia.org/wiki/ISO_8601 and http://en.wikipedia.org/wiki/ISO_week_date
     *
     * If the date falls in the last week of the previous year or the first
     * week of the following year, then the yearNum returned will be set to the
     * appropriate year.
     *
     * @see weeksInYear()
     * @see formatDate()
     * @param yearNum returns the year the date belongs to
     * @return localized week number, -1 if input date invalid
     */
    int week(int *yearNum = 0) const;

    /**
     * Returns the Week Number for the date in the required Week Number System.
     *
     * See @ref formatting for why you should never display this value.
     *
     * Unless you want a specific Week Number System (e.g. ISO Week), you should
     * use the localized Week Number form of week().
     *
     * If the date falls in the last week of the previous year or the first
     * week of the following year, then the yearNum returned will be set to the
     * appropriate year.
     *
     * Technically, the ISO Week Number only applies to the ISO/Gregorian Calendar
     * System, but the same rules will be applied to the current Calendar System.
     *
     * @see weeksInYear()
     * @see formatDate()
     * @param weekNumberSystem the Week Number System to use
     * @param yearNum returns the year the date belongs to
     * @return week number, -1 if input date invalid
     */
    int week(KLocale::WeekNumberSystem weekNumberSystem, int *yearNum = 0) const;

    /**
     * Returns number of months in the year
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return number of months in the year, -1 if  date invalid
     */
    int monthsInYear() const;

    /**
     * Returns the number of localized weeks in the currently set year.
     *
     * See @ref formatting for why you should never display this value.
     *
     * Currently only the ISO Week Number is supported, but in future the US
     * and other week number systems will be added.
     *
     * If you specifically require the number of ISO Weeks, you should use
     * weeksInYear(KLocale::IsoWeekNumber)
     *
     * @see week()
     * @see formatDate()
     * @return number of weeks in the year, -1 if  date invalid
     */
    int weeksInYear() const;

    /**
     * Returns the number of Weeks in the currently set year using the required
     * Week Number System.
     *
     * See @ref formatting for why you should never display this value.
     *
     * Unless you specifically want a particular Week Number System (e.g. ISO Weeks)
     * you should use the localized number of weeks provided by weeksInYear().
     *
     * @see week()
     * @see formatDate()
     * @param weekNumberSystem the week number system to use
     * @return number of weeks in the year, -1 if  date invalid
     */
    int weeksInYear(KLocale::WeekNumberSystem weekNumberSystem) const;

    /**
     * Returns the number of days in the year.
     *
     * For example, in the Gregorian calendar most years have 365 days but Leap
     * Years have 366 years.  Other Calendar Systems have different length years.
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return number of days in year, -1 if date invalid
     */
    int daysInYear() const;

    /**
     * Returns the number of days in the month.
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return number of days in month, -1 if date invalid
     */
    int daysInMonth() const;

    /**
     * Returns the number of days in the week.
     *
     * See @ref formatting for why you should never display this value.
     *
     * @see formatDate()
     * @return number of days in week, -1 if date invalid
     */
    int daysInWeek() const;

    /**
     * Returns whether the currently set date falls in a Leap Year in the
     * current Calendar System.
     *
     * @return true if the date falls in a leap year
     */
    bool isLeapYear() const;

    /**
     * Returns the Date as a localized string in the requested standard Locale
     * format.
     *
     * See @ref formatting for more details on Date Formatting and valid Locale
     * formats.
     *
     * @see formatDate()
     * @param dateFormat the standard date format to use
     * @return The date as a localized string
     */
    QString formatDate(KLocale::DateFormat dateFormat = KLocale::LongDate) const;

    /**
     * Returns the Date as a localized string in the requested format.
     *
     * See @ref formatting for more details on Date Formatting and valid format
     * codes.
     *
     * Please use with care and only in situations where the standard Locale
     * formats or the component format methods do not provide what you
     * need.  You should almost always translate your @p formatString as
     * documented above.  Using the standard DateFormat options instead would
     * take care of the translation for you.
     *
     * The toFormat parameter is a good candidate to be made translatable,
     * so that translators can adapt it to their language's convention.
     * There should also be a context using the "kdedt-format" keyword (for
     * automatic validation of translations) and stating the format's purpose:
     * \code
     * QDate reportDate;
     * KGlobal::locale()->calendar()->setDate(reportDate, reportYear, reportMonth, 1);
     * dateFormat = i18nc("(kdedt-format) Report month and year in report header", "%B %Y"));
     * dateString = KGlobal::locale()->calendar()->formatDate(reportDate, dateFormat);
     * \endcode
     *
     * The date format string can be defined using either the KDE, POSIX or the Qt
     * subset of the UNICODE standards.
     *
     * The KDE standard closely follows the POSIX standard but with some exceptions.
     * Always use the KDE standard within KDE, but where interaction is required with
     * external POSIX compliant systems (e.g. Gnome, glibc, etc) the POSIX standard
     * should be used.  The UNICODE standard is provided for comaptability with QDate
     * and so is not yet the full standard, only what Qt currently supports.
     *
     * Date format strings are made up of date components and string literals.
     * Date components are prefixed by a % escape character and are made up of
     * optional padding and case modifier flags, an optional width value, and a
     * compulsary code for the actual date component:
     *   %[Flags][Width][Componant]
     * e.g. %_^5Y
     * No spaces are allowed.
     *
     * The Flags can modify the padding character and/or case of the Date Componant.
     * The Flags are optional and may be combined and/or repeated in any order,
     * in which case the last Padding Flag and last Case Flag will be the
     * ones used.  The Flags must be immediately after the % and before any Width.
     *
     * The Width can modify how wide the date Componant is padded to.  The Width
     * is an optional interger value and must be after any Flags but before the
     * Componant.  If the Width is less than the minimum defined for a Componant
     * then the default minimum will be used instead.
     *
     * By default most numeric Date Componants are right-aligned with leading 0's.
     *
     * By default all string name fields are capital case and unpadded.
     *
     * The following Flags may be specified:
     * @li - (hyphen) no padding (e.g. 1 Jan and "%-j" = "1")
     * @li _ (underscore) pad with spaces (e.g. 1 Jan and "%-j" = "  1")
     * @li 0 (zero) pad with 0's  (e.g. 1 Jan and "%0j" = "001")
     * @li ^ (caret) make uppercase (e.g. 1 Jan and "%^B" = "JANUARY")
     * @li # (hash) invert case (e.g. 1 Jan and "%#B" = "???")
     *
     * The following Date Componants can be specified:
     * @li %Y the year to 4 digits (e.g. "1984" for 1984, "0584" for 584, "0084" for 84)
     * @li %C the 'century' portion of the year to 2 digits (e.g. "19" for 1984, "05" for 584, "00" for 84)
     * @li %y the lower 2 digits of the year to 2 digits (e.g. "84" for 1984, "05" for 2005)
     * @li %EY the full local era year (e.g. "2000 AD")
     * @li %EC the era name short form (e.g. "AD")
     * @li %Ey the year in era to 1 digit (e.g. 1 or 2000)
     * @li %m the month number to 2 digits (January="01", December="12")
     * @li %n the month number to 1 digit (January="1", December="12"), see notes!
     * @li %d the day number of the month to 2 digits (e.g. "01" on the first of March)
     * @li %e the day number of the month to 1 digit (e.g. "1" on the first of March)
     * @li %B the month name long form (e.g. "January")
     * @li %b the month name short form (e.g. "Jan" for January)
     * @li %h the month name short form (e.g. "Jan" for January)
     * @li %A the weekday name long form (e.g. "Wednesday" for Wednesday)
     * @li %a the weekday name short form (e.g. "Wed" for Wednesday)
     * @li %j the day of the year number to 3 digits (e.g. "001"  for 1 Jan)
     * @li %V the ISO week of the year number to 2 digits (e.g. "01"  for ISO Week 1)
     * @li %G the year number in long form of the ISO week of the year to 4 digits (e.g. "2004"  for 1 Jan 2005)
     * @li %g the year number in short form of the ISO week of the year to 2 digits (e.g. "04"  for 1 Jan 2005)
     * @li %u the day of the week number to 1 digit (e.g. "1"  for Monday)
     * @li %D the US short date format (e.g. "%m/%d/%y")
     * @li %F the ISO short date format (e.g. "%Y-%m-%d")
     * @li %x the KDE locale short date format
     * @li %% the literal "%"
     * @li %t a tab character
     *
     * Everything else in the format string will be taken as literal text.
     *
     * Examples:
     * "%Y-%m-%d" = "2009-01-01"
     * "%Y-%-m-%_4d" = "2009-1-   1"
     *
     * The following format codes behave differently in the KDE and POSIX standards
     * @li %e in GNU/POSIX is space padded to 2 digits, in KDE is not padded
     * @li %n in GNU/POSIX is newline, in KDE is short month number
     *
     * The following POSIX format codes are currently not supported:
     * @li %U US week number
     * @li %w US day of week
     * @li %W US week number
     * @li %O locale's alternative numeric symbols, in KDE is not supported
     *
     * %0 is not supported as the returned result is always in the locale's chosen numeric symbol digit set.
     *
     * @see formatDate()
     * @param formatString the date format to use
     * @param formatStandard the standard the @p dateFormat uses, defaults to KDE Standard
     * @return The date as a localized string
     */
    QString formatDate(const QString &formatString,
                       KLocale::DateTimeFormatStandard formatStandard = KLocale::KdeFormat) const;

    /**
     * Returns a Date Component as a localized string in the requested format.
     *
     * See @ref formatting for more details on Date Formatting.
     *
     * Each format size may vary depending on Locale and Calendar System but will
     * generally match the format description.  Some formats may not be directly
     * valid but a sensible value will always be returned.
     *
     * For example for 2010-01-01 the KLocale::Month with en_US Locale and Gregorian calendar may return:
     *   KLocale::ShortNumber = "1"
     *   KLocale::LongNumber  = "01"
     *   KLocale::NarrowName  = "J"
     *   KLocale::ShortName   = "Jan"
     *   KLocale::LongName    = "January"
     *
     * @see formatDate()
     * @param component The date component to return
     * @param format The format to return the @p component in
     * @param weekNumberSystem To override the default Week Number System to use
     * @return The string form of the date component
     */
    QString formatDate(KLocale::DateTimeComponent component,
                       KLocale::DateTimeComponentFormat format = KLocale::DefaultComponentFormat,
                       KLocale::WeekNumberSystem weekNumberSystem = KLocale::DefaultWeekNumber) const;

    /**
     * Converts a localized date string to a KLocalizedDate using either the
     * Global Calendar System and Locale, or the provided Calendar System.
     *
     * See @ref parsing for more details on Date Parsing from strings.
     *
     * This method is more liberal and will return a valid date if the
     * @p dateString matches any of the KLocale::ReadDateFlags formats
     * for the Locale.
     *
     * If you require a certain KLocale::ReadDateFlags format or a customized
     * format string, use one of the other readDate() methods.
     *
     * @param dateString the string to parse
     * @param parseMode how strictly to apply the locale formats to the @p dateString
     * @param calendar the Calendar System to use when parsing the date
     * @return the localized date parsed from the string
     */
    static KLocalizedDate readDate(const QString &dateString,
                                   KLocale::DateTimeParseMode parseMode = KLocale::LiberalParsing,
                                   const KCalendarSystem *calendar = 0);

    /**
     * Converts a localized date string to a KLocalizedDate using either the
     * Global Calendar System and Locale, or the provided Calendar System.
     *
     * See @ref parsing for more details on Date Parsing from strings.
     *
     * This method is stricter and will return a valid date only if the
     * @p dateString matches one of the @p dateFlags formats requested.
     *
     * If you require any KLocale::ReadDateFlags format or a customized format
     * string, use one of the other readDate() methods.
     *
     * @param dateString the string to parse
     * @param formatFlags the locale format(s) to try parse the string with
     * @param parseMode how strictly to apply the @p formatFlags to the @p dateString
     * @param calendar the Calendar System to use when parsing the date
     * @return the localized date parsed from the string
     */
    static KLocalizedDate readDate(const QString &dateString,
                                   KLocale::ReadDateFlags formatFlags,
                                   KLocale::DateTimeParseMode parseMode = KLocale::LiberalParsing,
                                   const KCalendarSystem *calendar = 0);

    /**
     * Converts a localized date string to a KLocalizedDate using either the
     * Global Calendar System and Locale, or the provided Calendar System.
     *
     * See @ref parsing for more details on Date Parsing from strings.
     *
     * This method allows you to define your own date format to parse the date
     * string with.
     *
     * If you require one of the standard any KLocale::ReadDateFlags formats
     * then use one of the other readDate() methods.
     *
     * @param dateString the string to parse
     * @param dateFormat the date format to try parse the string with
     * @param parseMode how strictly to apply the @p dateFormat to the @p dateString
     * @param formatStandard the standard the @p dateFormat format uses
     * @param calendar the Calendar System to use when parsing the date
     * @return the localized date parsed from the string
     */
    static KLocalizedDate readDate(const QString &dateString,
                                   const QString &dateFormat,
                                   KLocale::DateTimeParseMode parseMode = KLocale::LiberalParsing,
                                   KLocale::DateTimeFormatStandard formatStandard = KLocale::KdeFormat,
                                   const KCalendarSystem *calendar = 0);

    /**
     * Returns a KLocalizedDate containing a date @p years years later.
     *
     * @see addYearsTo()
     * @see addMonths() addDays()
     * @see dateDifference() yearsDifference()
     * @param years The number of years to add
     * @return The new date, null date if any errors
     */
    KLocalizedDate addYears(int years) const;

    /**
     * Add years onto this date instance.
     *
     * If the result of the addition is invalid in the current Calendar System
     * then the date will become invalid.
     *
     * @see addYears()
     * @see addMonthsTo() addDaysTo()
     * @see dateDifference() yearsDifference()
     * @param years The number of years to add
     * @return if the resulting date is valid
     */
    bool addYearsTo(int years);

    /**
     * Returns a KLocalizedDate containing a date @p months months later.
     *
     * @see addMonthsTo()
     * @see addYears() addDays()
     * @see dateDifference() yearsDifference()
     * @param months number of months to add
     * @return The new date, null date if any errors
     */
    KLocalizedDate addMonths(int months) const;

    /**
     * Add months onto this date instance.
     *
     * If the result of the addition is invalid in the current Calendar System
     * then the date will become invalid.
     *
     * @see addMonths()
     * @see addYearsTo() addDaysTo()
     * @see dateDifference() yearsDifference()
     * @param months The number of months to add
     * @return if the resulting date is valid
     */
    bool addMonthsTo(int months);

    /**
     * Returns a KLocalizedDate containing a date @p days days later.
     *
     * @see addDaysTo()
     * @see addYears() addMonths()
     * @see dateDifference() yearsDifference()
     * @param days number of days to add
     * @return The new date, null date if any errors
     */
    KLocalizedDate addDays(int days) const;

    /**
     * Add days onto this date instance.
     *
     * If the result of the addition is invalid in the current Calendar System
     * then the date will become invalid.
     *
     * @see addDays()
     * @see addYearsTo(), addMonthsTo()
     * @see dateDifference() yearsDifference()
     * @param days The number of days to add
     * @return if the resulting date is valid
     */
    bool addDaysTo(int days);

    /**
     * Returns the difference between this and another date in years, months and days
     * in the current Calendar System.
     *
     * The difference is always calculated from the earlier date to the later
     * date in year, month and day order, with the @p direction parameter
     * indicating which direction the difference is applied from this date.
     * In other words, this difference can be added onto the earlier date in
     * year, month, day order to reach the later date.
     *
     * For example, the difference between 2010-06-10 and 2012-09-5 is 2 years,
     * 2 months and 26 days.  Note that the difference between two last days of
     * the month is always 1 month, e.g. 2010-01-31 to 2010-02-28 is 1 month
     * not 28 days.
     *
     * @see addYears() addMonths() addDays()
     * @see yearsDifference() monthsDifference() daysDifference()
     * @param toDate The date to end at
     * @param yearsDiff Returns number of years difference
     * @param monthsDiff Returns number of months difference
     * @param daysDiff Returns number of days difference
     * @param direction Returns direction of difference, 1 if this Date <= toDate, -1 otherwise
     */
    void dateDifference(const KLocalizedDate &toDate,
                        int *yearsDiff, int *monthsDiff, int *daysDiff, int *direction) const;

    /**
     * Returns the difference between this and another date in years, months and days
     * in the current Calendar System.
     *
     * The difference is always calculated from the earlier date to the later
     * date in year, month and day order, with the @p direction parameter
     * indicating which direction the difference is applied from this date.
     * In other words, this difference can be added onto the earlier date in
     * year, month, day order to reach the later date.
     *
     * For example, the difference between 2010-06-10 and 2012-09-5 is 2 years,
     * 2 months and 26 days.  Note that the difference between two last days of
     * the month is always 1 month, e.g. 2010-01-31 to 2010-02-28 is 1 month
     * not 28 days.
     *
     * @see addYears() addMonths() addDays()
     * @see yearsDifference() monthsDifference() daysDifference()
     * @param toDate The date to end at
     * @param yearsDiff Returns number of years difference
     * @param monthsDiff Returns number of months difference
     * @param daysDiff Returns number of days difference
     * @param direction Returns direction of difference, 1 if this Date <= toDate, -1 otherwise
     */
    void dateDifference(const QDate &toDate,
                        int *yearsDiff, int *monthsDiff, int *daysDiff, int *direction) const;

    /**
     * Returns the difference between this and another date in completed calendar years
     * in the current Calendar System.
     * 
     * The returned value will be negative if @p toDate < this date.
     *
     * For example, the difference between 2010-06-10 and 2012-09-5 is 2 years.
     *
     * @see addYears()
     * @see dateDifference() monthsDifference() daysDifference()
     * @param toDate The date to end at
     * @return The number of years difference
     */
    int yearsDifference(const KLocalizedDate &toDate) const;

    /**
     * Returns the difference between this and another date in completed calendar years
     * in the current Calendar System.
     *
     * The returned value will be negative if @p toDate < this date.
     *
     * For example, the difference between 2010-06-10 and 2012-09-5 is 2 years.
     *
     * @see addYears()
     * @see dateDifference() monthsDifference() daysDifference()
     * @param toDate The date to end at
     * @return The number of years difference
     */
    int yearsDifference(const QDate &toDate) const;

    /**
     * Returns the difference between this and another date in completed calendar months
     * in the current Calendar System.
     *
     * The returned value will be negative if @p toDate < this date.
     *
     * For example, the difference between 2010-06-10 and 2012-09-5 is 26 months.
     * Note that the difference between two last days of the month is always 1
     * month, e.g. 2010-01-31 to 2010-02-28 is 1 month not 28 days.
     *
     * @see addMonths()
     * @see dateDifference() yearsDifference() daysDifference()
     * @param toDate The date to end at
     * @return The number of months difference
     */
    int monthsDifference(const KLocalizedDate &toDate) const;

    /**
     * Returns the difference between this and another date in completed calendar months
     * in the current Calendar System.
     *
     * The returned value will be negative if @p toDate < this date.
     *
     * For example, the difference between 2010-06-10 and 2012-09-5 is 26 months.
     * Note that the difference between two last days of the month is always 1
     * month, e.g. 2010-01-31 to 2010-02-28 is 1 month not 28 days.
     *
     * @see addMonths()
     * @see dateDifference() yearsDifference() daysDifference()
     * @param toDate The date to end at
     * @return The number of months difference
     */
    int monthsDifference(const QDate &toDate) const;

    /**
     * Returns the difference between this and another date in days
     * The returned value will be negative if @p toDate < this date.
     *
     * @see addDays()
     * @see dateDifference() yearsDifference() monthsDifference()
     * @param toDate The date to end at
     * @return The number of days difference
     */
    int daysDifference(const KLocalizedDate &toDate) const;

    /**
     * Returns the difference between this and another date in days
     * The returned value will be negative if @p toDate < this date.
     *
     * @see addDays()
     * @see dateDifference() yearsDifference() monthsDifference()
     * @param toDate The date to end at
     * @return The number of days difference
     */
    int daysDifference(const QDate &toDate) const;

    /**
     * Returns a KLocalizedDate containing the first day of the currently set year
     *
     * @see lastDayOfYear()
     * @return The first day of the year
     */
    KLocalizedDate firstDayOfYear() const;

    /**
     * Returns a KLocalizedDate containing the last day of the currently set year
     *
     * @see firstDayOfYear()
     * @return The last day of the year
     */
    KLocalizedDate lastDayOfYear() const;

    /**
     * Returns a KLocalizedDate containing the first day of the currently set month
     *
     * @see lastDayOfMonth()
     * @return The first day of the month
     */
    KLocalizedDate firstDayOfMonth() const;

    /**
     * Returns a KLocalizedDate containing the last day of the currently set month
     *
     * @see firstDayOfMonth()
     * @return The last day of the month
     */
    KLocalizedDate lastDayOfMonth() const;

    /**
     * KLocalizedDate equality operator
     *
     * @param other the date to compare
     */
    bool operator==(const KLocalizedDate &other) const;

    /**
     * QDate equality operator
     *
     * @param other the date to compare
     */
    bool operator==(const QDate &other) const;

    /**
     * KLocalizedDate inequality operator
     *
     * @param other the date to compare
     */
    bool operator!=(const KLocalizedDate &other) const;

    /**
     * QDate inequality operator
     *
     * @param other the date to compare
     */
    bool operator!=(const QDate &other) const;

    /**
     * KLocalizedDate less than operator
     *
     * @param other the date to compare
     */
    bool operator<(const KLocalizedDate &other) const;

    /**
     * QDate less than operator
     *
     * @param other the date to compare
     */
    bool operator<(const QDate &other) const;

    /**
     * KLocalizedDate less than or equal to operator
     *
     * @param other the date to compare
     */
    bool operator<=(const KLocalizedDate &other) const;

    /**
     * QDate less than or equal to operator
     *
     * @param other the date to compare
     */
    bool operator<=(const QDate &other) const;

    /**
     * KLocalizedDate greater than operator
     *
     * @param other the date to compare
     */
    bool operator>(const KLocalizedDate &other) const;

    /**
     * QDate greater than operator
     *
     * @param other the date to compare
     */
    bool operator>(const QDate &other) const;

    /**
     * KLocalizedDate greater than or equal to operator
     *
     * @param other the date to compare
     */
    bool operator>=(const KLocalizedDate &other) const;

    /**
     * QDate greater than or equal to operator
     *
     * @param other the date to compare
     */
    bool operator>=(const QDate &other) const;

private:

    friend QDataStream KDECORE_EXPORT &operator<<(QDataStream &out, const KLocalizedDate &date);
    friend QDataStream KDECORE_EXPORT &operator>>(QDataStream &in, KLocalizedDate &date);
    friend QDebug KDECORE_EXPORT operator<<(QDebug, const KLocalizedDate &);

    QSharedDataPointer<KLocalizedDatePrivate> d;
};

Q_DECLARE_METATYPE(KLocalizedDate)

/**
 * Data stream output operator
 *
 * @param other the date to compare
 */
QDataStream KDECORE_EXPORT &operator<<(QDataStream &out, const KLocalizedDate &date);

/**
 * Data stream input operator
 *
 * @param other the date to compare
 */
QDataStream KDECORE_EXPORT &operator>>(QDataStream &in, KLocalizedDate &date);

/**
 * Debug stream output operator
 *
 * @param other the date to print
 */
QDebug KDECORE_EXPORT operator<<(QDebug, const KLocalizedDate &);

#endif // KDATE_H
