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

#ifndef TELLICODATEWIDGET_H
#define TELLICODATEWIDGET_H

#include <qspinbox.h>
#include <qdatetime.h>

class KComboBox;
class KPushButton;
class KDatePicker;

class QVBox;
class QString;

namespace Tellico {
  namespace GUI {

class SpinBox : public QSpinBox {
Q_OBJECT

public:
  SpinBox(int min, int max, QWidget *parent);
};

/**
 * @author Robby Stephenson
 */
class DateWidget : public QWidget {
Q_OBJECT

public:
  DateWidget(QWidget* parent, const char* name = 0);
  ~DateWidget() {}

  QDate date() const;
  QString text() const;
  void setDate(const QDate& date);
  void setDate(const QString& date);
  void clear();

signals:
  void signalModified();

private slots:
  void slotDateChanged();
  void slotShowPicker();
  void slotDateSelected(QDate newDate);
  void slotDateEntered(QDate newDate);

private:
  SpinBox* m_daySpin;
  KComboBox* m_monthCombo;
  SpinBox* m_yearSpin;
  KPushButton* m_dateButton;

  QVBox* m_frame;
  KDatePicker* m_picker;
};

  } // end namespace
} // end namespace
#endif
