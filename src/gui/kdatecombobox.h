/*
    Copyright 2011 John Layt <john@layt.net>

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

#ifndef KDATECOMBOBOX_H
#define KDATECOMBOBOX_H

#include <QtGui/QWidget>

#include <kcombobox.h>
#include <klocale.h>
#include <kdatetime.h>

class KDateComboBoxPrivate;
class KCalendarSystem;

class KDateComboBox : public KComboBox
{
    Q_OBJECT

    Q_PROPERTY(QDate date READ date WRITE setDate NOTIFY dateChanged USER true)
    Q_PROPERTY(QDate minimumDate READ minimumDate WRITE setMinimumDate RESET resetMinimumDate)
    Q_PROPERTY(QDate maximumDate READ maximumDate WRITE setMaximumDate RESET resetMaximumDate)
    Q_PROPERTY(Options options READ options WRITE setOptions)
    Q_FLAGS(Options)

public:

    /**
     * Options provided by the widget
     * @see options()
     * @see setOptions()
     */
    enum Option {
        EditDate         = 0x0001,  /**< Allow the user to manually edit the date in the combo line edit */
        SelectDate       = 0x0002,  /**< Allow the user to select the date from a drop-down menu */
        DatePicker       = 0x0004,  /**< Show a date picker in the drop-down */
        DateKeywords     = 0x0008,  /**< Show date keywords in the drop-down */
        WarnOnInvalid    = 0x0010   /**< Show a warning on focus out if the date is invalid */
    };
    Q_DECLARE_FLAGS(Options, Option)

    /**
     * Create a new KDateComboBox widget
     *
     * By default the EditDate, SelectDate, DatePicker and DateKeywords options
     * are enabled, the ShortDate format is used and the date is set to the
     * current date.
     */
    explicit KDateComboBox(QWidget *parent = 0);

    /**
     * Destroy the widget
     */
    virtual ~KDateComboBox();

    /**
     * Return the currently selected date
     *
     * @return the currently selected date
     */
    QDate date() const;

    /**
     * Returns the Calendar System type used by the widget
     *
     * @see KLocale::CalendarSystem
     * @see setCalendarSystem()
     * @return the Calendar System currently used
     */
    KLocale::CalendarSystem calendarSystem() const;

    /**
     * Returns a pointer to the Calendar System object used by this widget
     *
     * Usually this will be the Global Calendar System using the Global Locale,
     * but this may have been changed to a custom Calendar System possibly
     * using a custom Locale.
     *
     * Normally you will not need to access this object.
     *
     * @see KCalendarSystem
     * @see setCalendar
     * @return the current calendar system instance
     */
    const KCalendarSystem *calendar() const;

    /**
     * Return if the current user input is valid
     *
     * If the user input is null then it is not valid
     *
     * @see isNull()
     * @return if the current user input is valid
     */
    bool isValid() const;

    /**
     * Return if the current user input is null
     *
     * @see isValid()
     * @return if the current user input is null
     */
    bool isNull() const;

    /**
     * Return the currently set widget options
     *
     * @return the currently set widget options
     */
    Options options() const;

    /**
     * Return the currently set date display format
     *
     * By default this is the Short Date
     *
     * @return the currently set date format
     */
    KLocale::DateFormat displayFormat() const;

    /**
     * Return the current minimum date
     *
     * @return the current minimum date
     */
    QDate minimumDate() const;

    /**
     * Return the current maximum date
     *
     * @return the current maximum date
     */
    QDate maximumDate() const;

    /**
     * Return the map of dates listed in the drop-down and their displayed
     * string forms.
     *
     * @see setDateMap()
     * @return the select date map
     */
    QMap<QDate, QString> dateMap() const;

Q_SIGNALS:

    /**
     * Signal if the date has been manually entered or selected by the user.
     *
     * The returned date may be invalid.
     *
     * @param date the new date
     */
    void dateEntered(const QDate &date);

    /**
     * Signal if the date has been changed either manually by the user
     * or programatically.
     *
     * The returned date may be invalid.
     *
     * @param date the new date
     */
    void dateChanged(const QDate &date);

    /**
     * Signal if the date is being manually edited by the user.
     *
     * The returned date may be invalid.
     *
     * @param date the new date
     */
    void dateEdited(const QDate &date);

public Q_SLOTS:

    /**
     * Set the currently selected date
     *
     * You can set an invalid date or a date outside the valid range, validity
     * checking is only done via isValid().
     *
     * @param date the new date
     */
    void setDate(const QDate &date);

    /**
     * Set the Calendar System used for this widget.  Uses the global locale.
     *
     * @see KLocale::CalendarSystem
     * @see calendarSystem()
     * @param calendarSystem the Calendar System to use
     */
    void setCalendarSystem(KLocale::CalendarSystem calendarSystem);

    /**
     * Changes the calendar system to use.  Can use its own local locale if set.
     *
     * You retain ownership of the calendar object, it will not be destroyed with the widget.
     *
     * @param calendar the calendar system object to use, defaults to global
     */
    void setCalendar(KCalendarSystem *calendar = 0);

    /**
     * Set the new widget options
     *
     * @param options the new widget options
     */
    void setOptions(Options options);

    /**
     * Sets the date format to display.
     *
     * By default is the Short Date format.
     *
     * @param format the date format to use
     */
    void setDisplayFormat(KLocale::DateFormat format);

    /**
     * Set the valid date range to be applied by isValid().
     *
     * Both dates must be valid and the minimum date must be less than or equal
     * to the maximum date, otherwise the date range will not be set.
     *
     * @param minDate the minimum date
     * @param maxDate the maximum date
     * @param minWarnMsg the minimum warning message
     * @param maxWarnMsg the maximum warning message
     */
    void setDateRange(const QDate &minDate,
                      const QDate &maxDate,
                      const QString &minWarnMsg = QString(),
                      const QString &maxWarnMsg = QString());

    /**
     * Reset the minimum and maximum date to the default values.
     * @see setDateRange()
     */
    void resetDateRange();

    /**
     * Set the minimum allowed date.
     *
     * If the date is invalid, or greater than current maximum,
     * then the minimum will not be set.
     *
     * @see minimumDate()
     * @see maximumDate()
     * @see setMaximumDate()
     * @see setDateRange()
     * @param minDate the minimum date
     * @param minWarnMsg the minimum warning message
     */
    void setMinimumDate(const QDate &minTime, const QString &minWarnMsg = QString());

    /**
     * Reset the minimum date to the default
     */
    void resetMinimumDate();

    /**
     * Set the maximum allowed date.
     *
     * If the date is invalid, or less than current minimum,
     * then the maximum will not be set.
     *
     * @see minimumDate()
     * @see maximumDate()
     * @see setMaximumDate()
     * @see setDateRange()
     * @param maxDate the maximum date
     * @param maxWarnMsg the maximum warning message
     */
    void setMaximumDate(const QDate &maxDate, const QString &maxWarnMsg = QString());

    /**
     * Reset the maximum date to the default
     */
    void resetMaximumDate();

    /**
     * Set the list of dates able to be selected from the drop-down and the
     * string form to display for those dates, e.g. "2010-01-01" and "Yesterday".
     *
     * Any invalid or duplicate dates will be used, the list will NOT be
     * sorted, and the minimum and maximum date will not be affected.
     *
     * The @p dateMap is keyed by the date to be listed and the value is the
     * string to be displayed.  If you want the date to be displayed in the
     * default date format then the string should be null.  If you want a
     * separator to be displayed then set the string to "seperator".
     *
     * @see dateMap()
     * @param dateMap the map of dates able to be selected
     */
    void setDateMap(QMap<QDate, QString> dateMap);

protected:

    virtual bool eventFilter(QObject *object, QEvent *event);
    virtual void showPopup();
    virtual void hidePopup();
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void resizeEvent(QResizeEvent *event);

    /**
     * Assign the date for the widget.
     *
     * Virtual to allow sub-classes to apply extra validation rules.
     *
     * @param date the new date
     */
    virtual void assignDate(const QDate &date);

    /**
     * Assign the calendar system for the widget.
     *
     * Virtual to allow sub-classes to apply extra validation rules.
     *
     * @param time the new time
     */
    virtual void assignCalendarSystem(KLocale::CalendarSystem calendarSystem);

private:

    friend class KDateComboBoxPrivate;
    KDateComboBoxPrivate *const d;

    Q_PRIVATE_SLOT(d, void clickDate())
    Q_PRIVATE_SLOT(d, void selectDate(QAction*))
    Q_PRIVATE_SLOT(d, void editDate(const QString&))
    Q_PRIVATE_SLOT(d, void enterDate(const QDate&))
    Q_PRIVATE_SLOT(d, void parseDate())

};

Q_DECLARE_OPERATORS_FOR_FLAGS(KDateComboBox::Options)

#endif // KDATECOMBOBOX_H
