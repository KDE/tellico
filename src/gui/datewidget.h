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

#ifndef TELLICO_GUI_DATEWIDGET_H
#define TELLICO_GUI_DATEWIDGET_H

#include <QWidget>

class KComboBox;
class KDatePicker;

class QDate;
class QMenu;
class QPushButton;

namespace Tellico {
  namespace GUI {

class SpinBox;

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

Q_SIGNALS:
  void signalModified();

private Q_SLOTS:
  void slotDateChanged();
  void slotShowPicker();
  void slotDateSelected(QDate newDate);
  void slotDateEntered(QDate newDate);

private:
  class DatePickerAction;
  SpinBox* m_daySpin;
  KComboBox* m_monthCombo;
  SpinBox* m_yearSpin;
  QPushButton* m_dateButton;

  QMenu* m_menu;
  KDatePicker* m_picker;
};

  } // end namespace
} // end namespace
#endif
