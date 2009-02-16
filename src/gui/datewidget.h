/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
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

#include <QSpinBox>
#include <QDateTime>

class KComboBox;
class KPushButton;
class KDatePicker;
class KVBox;

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
  DateWidget(QWidget* parent);
  ~DateWidget();

  QDate date() const;
  QString text() const;
  void setDate(const QDate& date);
  void setDate(const QString& date);
  void clear();

signals:
  void signalModified();

protected:
  bool eventFilter(QObject *watched, QEvent *event);

private slots:
  void slotDateChanged();
  void slotShowPicker();
  void slotDateSelected(const QDate& newDate);
  void slotDateEntered(const QDate& newDate);

private:
  SpinBox* m_daySpin;
  KComboBox* m_monthCombo;
  SpinBox* m_yearSpin;
  KPushButton* m_dateButton;

  KVBox* m_frame;
  KDatePicker* m_picker;
};

  } // end namespace
} // end namespace
#endif
