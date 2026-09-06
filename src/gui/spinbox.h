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

#ifndef TELLICO_GUI_SPINBOX_H
#define TELLICO_GUI_SPINBOX_H

#include <QAbstractSpinBox>

class FieldWidgetTest;

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 * @author Hermann Brockmann
 */
class SpinBox : public QAbstractSpinBox {
Q_OBJECT

friend class ::FieldWidgetTest;

public:
  SpinBox(qint64 min, qint64 max, QWidget* parent);

  virtual void stepBy(int steps) override;
  void setValue(qint64 val);
  qint64 value() const { return m_value; }

  void setMinimum(qint64 min) { m_min = min; }
  void setMaximum(qint64 max) { m_max = max; }
  qint64 minimum() const { return m_min; }
  qint64 maximum() const { return m_max; }
  QString cleanText() const;

protected:
  StepEnabled stepEnabled() const override;

Q_SIGNALS:
  void valueChanged(qint64 newValue);
  void textChanged(const QString &text);

private Q_SLOTS:
  void checkValue(const QString&);

private:
  QValidator::State validate(QString& text, int& pos) const override;
  qint64 m_value;
  qint64 m_min;
  qint64 m_max;
};

  } // end namespace
} // end namespace
#endif
