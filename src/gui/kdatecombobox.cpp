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

#include "kdatecombobox.h"

#include <QtGui/QAbstractItemView>
#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QLineEdit>
#include <QtGui/QWidgetAction>

#include <kdebug.h>
#include <klocale.h>
#include <klocalizeddate.h>
#include <kcombobox.h>
#include <kdatepicker.h>
#include <kmessagebox.h>

class KDateComboBoxPrivate
{
public:

    KDateComboBoxPrivate(KDateComboBox *q);
    virtual ~KDateComboBoxPrivate();

    QDate defaultMinDate();
    QDate defaultMaxDate();

    QString formatDate(const QDate &date);

    void initDateWidget();
    void addMenuAction(const QString &text, const QDate &date);
    void updateDateWidget();

// Q_PRIVATE_SLOTs
    void clickDate();
    void selectDate(QAction *action);
    void editDate(const QString &text);
    void enterDate(const QDate &date);
    void parseDate();
    void warnDate();

    KDateComboBox *const q;
    QMenu *m_dateMenu;
    KDatePicker *m_datePicker;
    QWidgetAction *m_datePickerAction;

    KLocalizedDate m_date;
    KDateComboBox::Options m_options;
    QDate m_minDate;
    QDate m_maxDate;
    QString m_minWarnMsg;
    QString m_maxWarnMsg;
    bool m_warningShown;
    KLocale::DateFormat m_displayFormat;
    QMap<QDate, QString> m_dateMap;
};

KDateComboBoxPrivate::KDateComboBoxPrivate(KDateComboBox *q)
                     :q(q),
                      m_dateMenu(new QMenu(q)),
                      m_datePicker(new KDatePicker(q)),
                      m_datePickerAction(new QWidgetAction(q)),
                      m_displayFormat(KLocale::ShortDate)
{
    m_options = KDateComboBox::EditDate | KDateComboBox::SelectDate | KDateComboBox::DatePicker | KDateComboBox::DateKeywords;
    m_date.setDate(QDate::currentDate());
    m_minDate = defaultMinDate();
    m_maxDate = defaultMaxDate();
    m_datePicker->setCloseButton(false);
    m_datePickerAction->setObjectName(QLatin1String("DatePicker"));
    m_datePickerAction->setDefaultWidget(m_datePicker);
}

KDateComboBoxPrivate::~KDateComboBoxPrivate()
{
}

QDate KDateComboBoxPrivate::defaultMinDate()
{
    return m_date.calendar()->earliestValidDate();
}

QDate KDateComboBoxPrivate::defaultMaxDate()
{
    return m_date.calendar()->latestValidDate();
}

QString KDateComboBoxPrivate::formatDate(const QDate &date)
{
    return m_date.calendar()->formatDate(date, m_displayFormat);
}

void KDateComboBoxPrivate::initDateWidget()
{
    q->blockSignals(true);
    q->clear();

    // If EditTime then set the line edit
    q->lineEdit()->setReadOnly((m_options &KDateComboBox::EditDate) != KDateComboBox::EditDate);

    // If SelectTime then make list items visible
    if ((m_options &KDateComboBox::SelectDate) == KDateComboBox::SelectDate ||
        (m_options &KDateComboBox::DatePicker) == KDateComboBox::DatePicker ||
        (m_options &KDateComboBox::DatePicker) == KDateComboBox::DateKeywords) {
        q->setMaxVisibleItems(1);
    } else {
        q->setMaxVisibleItems(0);
    }

    q->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    q->addItem(m_date.formatDate(m_displayFormat));
    q->setCurrentIndex(0);
    q->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    q->blockSignals(false);

    m_dateMenu->clear();

    if ((m_options &KDateComboBox::SelectDate) == KDateComboBox::SelectDate) {

        if ((m_options &KDateComboBox::DatePicker) == KDateComboBox::DatePicker) {
            m_dateMenu->addAction(m_datePickerAction);
            m_dateMenu->addSeparator();
        }

        if ((m_options &KDateComboBox::DateKeywords) == KDateComboBox::DateKeywords) {
            if (m_dateMap.isEmpty()) {
                addMenuAction(i18nc("@option next year",  "Next Year" ), m_date.addYears(1).date());
                addMenuAction(i18nc("@option next month", "Next Month"), m_date.addMonths(1).date());
                addMenuAction(i18nc("@option next week",  "Next Week" ), m_date.addDays(m_date.daysInWeek()).date());
                addMenuAction(i18nc("@option tomorrow",   "Tomorrow"  ), m_date.addDays(1).date());
                addMenuAction(i18nc("@option today",      "Today"     ), m_date.date());
                addMenuAction(i18nc("@option yesterday",  "Yesterday" ), m_date.addDays(-1).date());
                addMenuAction(i18nc("@option last week",  "Last Week" ), m_date.addDays(-m_date.daysInWeek()).date());
                addMenuAction(i18nc("@option last month", "Last Month"), m_date.addMonths(-1).date());
                addMenuAction(i18nc("@option last year",  "Last Year" ), m_date.addYears(-1).date());
                m_dateMenu->addSeparator();
                addMenuAction(i18nc("@option do not specify a date", "No Date"), QDate());
            } else {
                QMapIterator<QDate, QString> i(m_dateMap);
                while (i.hasNext()) {
                    i.next();
                    if (i.value().isEmpty()) {
                        addMenuAction(formatDate(i.key()), i.key());
                    } else if (i.value().toLower() == QLatin1String("separator")) {
                        m_dateMenu->addSeparator();
                    } else {
                        addMenuAction(i.value(), i.key());
                    }
                }
            }
        }

    }
}

void KDateComboBoxPrivate::addMenuAction(const QString &text, const QDate &date)
{
    QAction *action = new QAction(m_dateMenu);
    action->setText(text);
    action->setData(date);
    m_dateMenu->addAction(action);
}

void KDateComboBoxPrivate::updateDateWidget()
{
    q->blockSignals(true);
    m_datePicker->blockSignals(true);
    m_datePicker->setDate(m_date.date());
    int pos = q->lineEdit()->cursorPosition();
    q->setItemText(0, m_date.formatDate(m_displayFormat));
    q->lineEdit()->setText(m_date.formatDate(m_displayFormat));
    q->lineEdit()->setCursorPosition(pos);
    m_datePicker->blockSignals(false);
    q->blockSignals(false);
}

void KDateComboBoxPrivate::selectDate(QAction *action)
{
    if (action->objectName() != QLatin1String("DatePicker")) {
        enterDate(action->data().toDate());
    }
}

void KDateComboBoxPrivate::clickDate()
{
    enterDate(m_datePicker->date());
}

void KDateComboBoxPrivate::editDate(const QString &text)
{
    m_warningShown = false;
    emit q->dateEdited(m_date.readDate(text).date());
}

void KDateComboBoxPrivate::parseDate()
{
    m_date.setDate(m_date.readDate(q->lineEdit()->text()).date());
}

void KDateComboBoxPrivate::enterDate(const QDate &date)
{
    q->setDate(date);
    // Re-add the combo box item in order to retain the correct widget width
    q->blockSignals(true);
    q->clear();
    q->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    q->addItem(m_date.formatDate(m_displayFormat));
    q->setCurrentIndex(0);
    q->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    q->blockSignals(false);

    m_dateMenu->hide();
    warnDate();
    emit q->dateEntered(m_date.date());
}

void KDateComboBoxPrivate::warnDate()
{
    if (!m_warningShown && !q->isValid() &&
        (m_options &KDateComboBox::WarnOnInvalid) == KDateComboBox::WarnOnInvalid) {
        QString warnMsg;
        if (!m_date.date().isValid()) {
            warnMsg = i18nc("@info", "The date you entered is invalid");
        } else if (m_date.date() < m_minDate) {
            if (m_minWarnMsg.isEmpty()) {
                warnMsg = i18nc("@info", "Date cannot be earlier than %1", formatDate(m_minDate));
            } else {
                warnMsg = m_minWarnMsg;
                warnMsg.replace(QLatin1String("%1"), formatDate(m_minDate));
            }
        } else if (m_date.date() > m_maxDate) {
            if (m_maxWarnMsg.isEmpty()) {
                warnMsg = i18nc("@info", "Date cannot be later than %1", formatDate(m_maxDate));
            } else {
                warnMsg = m_maxWarnMsg;
                warnMsg.replace(QLatin1String("%1"), formatDate(m_maxDate));
            }
        }
        m_warningShown = true;
        KMessageBox::sorry(q, warnMsg);
    }
}


KDateComboBox::KDateComboBox(QWidget *parent)
              :KComboBox(parent),
               d(new KDateComboBoxPrivate(this))
{
    setEditable(true);
    setMaxVisibleItems(1);
    setInsertPolicy(QComboBox::NoInsert);
    d->m_datePicker->installEventFilter(this);
    d->initDateWidget();
    d->updateDateWidget();

    connect(d->m_dateMenu,         SIGNAL(triggered(QAction*)),
            this,                  SLOT(selectDate(QAction*)));
    connect(this,                  SIGNAL(editTextChanged(const QString&)),
            this,                  SLOT(editDate(const QString&)));
    connect(d->m_datePicker,       SIGNAL(dateEntered(const QDate&)),
            this,                  SLOT(enterDate(const QDate&)));
    connect(d->m_datePicker,       SIGNAL(tableClicked()),
            this,                  SLOT(clickDate()));
}

KDateComboBox::~KDateComboBox()
{
    delete d;
}

QDate KDateComboBox::date() const
{
    d->parseDate();
    return d->m_date.date();
}

void KDateComboBox::setDate(const QDate &date)
{
    if (date == d->m_date.date()) {
        return;
    }

    assignDate(date);
    d->updateDateWidget();
    emit dateChanged(d->m_date.date());
}

void KDateComboBox::assignDate(const QDate &date)
{
    d->m_date = date;
}

KLocale::CalendarSystem KDateComboBox::calendarSystem() const
{
    return d->m_date.calendarSystem();
}

void KDateComboBox::setCalendarSystem(KLocale::CalendarSystem calendarSystem)
{
    if (calendarSystem != d->m_date.calendarSystem()) {
        assignCalendarSystem(calendarSystem);
    }
}

void KDateComboBox::assignCalendarSystem(KLocale::CalendarSystem calendarSystem)
{
    d->m_date.setCalendarSystem(calendarSystem);
}

const KCalendarSystem *KDateComboBox::calendar() const
{
    return d->m_date.calendar();
}

void KDateComboBox::setCalendar(KCalendarSystem *calendar)
{
    d->m_date = KLocalizedDate(d->m_date.date(), calendar);
}

bool KDateComboBox::isValid() const
{
    d->parseDate();
    return d->m_date.isValid() &&
           d->m_date >= d->m_minDate &&
           d->m_date <= d->m_maxDate;
}

bool KDateComboBox::isNull() const
{
    return lineEdit()->text().isEmpty();
}

KDateComboBox::Options KDateComboBox::options() const
{
    return d->m_options;
}

void KDateComboBox::setOptions(Options options)
{
    if (options != d->m_options) {
        d->m_options = options;
        d->initDateWidget();
        d->updateDateWidget();
    }
}

QDate KDateComboBox::minimumDate() const
{
    return d->m_minDate;
}

void KDateComboBox::setMinimumDate(const QDate &minDate, const QString &minWarnMsg)
{
    setDateRange(minDate, d->m_maxDate, minWarnMsg, d->m_maxWarnMsg);
}

void KDateComboBox::resetMinimumDate()
{
    setDateRange(d->defaultMinDate(), d->m_maxDate, QString(), d->m_maxWarnMsg);
}

QDate KDateComboBox::maximumDate() const
{
    return d->m_maxDate;
}

void KDateComboBox::setMaximumDate(const QDate &maxDate, const QString &maxWarnMsg)
{
    setDateRange(d->m_minDate, maxDate, d->m_minWarnMsg, maxWarnMsg);
}

void KDateComboBox::resetMaximumDate()
{
    setDateRange(d->m_minDate, d->defaultMaxDate(), d->m_minWarnMsg, QString());
}

void KDateComboBox::setDateRange(const QDate &minDate,
                                 const QDate &maxDate,
                                 const QString &minWarnMsg,
                                 const QString &maxWarnMsg)
{
    if (!minDate.isValid() || !maxDate.isValid() || minDate > maxDate) {
        return;
    }

    if (minDate != d->m_minDate || maxDate != d->m_maxDate ||
        minWarnMsg != d->m_minWarnMsg || maxWarnMsg != d->m_maxWarnMsg) {
        d->m_minDate = minDate;
        d->m_maxDate = maxDate;
        d->m_minWarnMsg = minWarnMsg;
        d->m_maxWarnMsg = maxWarnMsg;
    }
}

void KDateComboBox::resetDateRange()
{
    setDateRange(d->defaultMinDate(), d->defaultMaxDate(), QString(), QString());
}

KLocale::DateFormat KDateComboBox::displayFormat() const
{
    return d->m_displayFormat;
}

void KDateComboBox::setDisplayFormat(KLocale::DateFormat format)
{
    if (format != d->m_displayFormat) {
        d->m_displayFormat = format;
        d->initDateWidget();
        d->updateDateWidget();
    }
}

QMap<QDate, QString> KDateComboBox::dateMap() const
{
    return d->m_dateMap;
}

void KDateComboBox::setDateMap(QMap<QDate, QString> dateMap)
{
    if (dateMap != d->m_dateMap) {
        d->m_dateMap.clear();
        d->m_dateMap = dateMap;
        d->initDateWidget();
    }
}

bool KDateComboBox::eventFilter(QObject *object, QEvent *event)
{
    return KComboBox::eventFilter(object, event);
}

void KDateComboBox::keyPressEvent(QKeyEvent *keyEvent)
{
    QDate temp;
    switch (keyEvent->key()) {
    case Qt::Key_Down:
        temp = d->m_date.addDays(-1).date();
        break;
    case Qt::Key_Up:
        temp = d->m_date.addDays(1).date();
        break;
    case Qt::Key_PageDown:
        temp = d->m_date.addMonths(-1).date();
        break;
    case Qt::Key_PageUp:
        temp = d->m_date.addMonths(1).date();
        break;
    default:
        KComboBox::keyPressEvent(keyEvent);
        return;
    }
    if (temp.isValid() && temp >= d->m_minDate && temp <= d->m_maxDate) {
        d->enterDate(temp);
    }
}

void KDateComboBox::focusOutEvent(QFocusEvent *event)
{
    d->parseDate();
    d->warnDate();
    KComboBox::focusOutEvent(event);
}

void KDateComboBox::showPopup()
{
    if (!isEditable() ||
        !d->m_dateMenu ||
        (d->m_options &KDateComboBox::SelectDate) != KDateComboBox::SelectDate) {
        return;
    }

    d->m_datePicker->blockSignals(true);
    d->m_datePicker->setDate(d->m_date.date());
    d->m_datePicker->blockSignals(false);

    const QRect desk = KGlobalSettings::desktopGeometry(this);

    QPoint popupPoint = mapToGlobal(QPoint(0, 0));

    const int dateFrameHeight = d->m_dateMenu->sizeHint().height();
    if (popupPoint.y() + height() + dateFrameHeight > desk.bottom()) {
        popupPoint.setY(popupPoint.y() - dateFrameHeight);
    } else {
        popupPoint.setY(popupPoint.y() + height());
    }

    const int dateFrameWidth = d->m_dateMenu->sizeHint().width();
    if (popupPoint.x() + dateFrameWidth > desk.right()) {
        popupPoint.setX(desk.right() - dateFrameWidth);
    }

    if (popupPoint.x() < desk.left()) {
        popupPoint.setX(desk.left());
    }

    if (popupPoint.y() < desk.top()) {
        popupPoint.setY(desk.top());
    }

    d->m_dateMenu->popup(popupPoint);
}

void KDateComboBox::hidePopup()
{
    KComboBox::hidePopup();
}

void KDateComboBox::mousePressEvent(QMouseEvent *event)
{
    KComboBox::mousePressEvent(event);
}

void KDateComboBox::wheelEvent(QWheelEvent *event)
{
    KComboBox::wheelEvent(event);
}

void KDateComboBox::focusInEvent(QFocusEvent *event)
{
    KComboBox::focusInEvent(event);
}

void KDateComboBox::resizeEvent(QResizeEvent *event)
{
    KComboBox::resizeEvent(event);
}

