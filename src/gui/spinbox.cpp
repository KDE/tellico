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

#include "spinbox.h"

#include <QLineEdit>

using Tellico::GUI::SpinBox;

SpinBox::SpinBox(int min_, int max_, QWidget * parent_) : QSpinBox(parent_) {
  setMinimum(min_);
  setMaximum(max_);
  setAlignment(Qt::AlignRight);
  // I want to be able to have an empty value at the minimum
  // an empty string just removes the special value, so set white space
  setSpecialValueText(QStringLiteral(" "));
  connect(lineEdit(), &QLineEdit::textChanged, this, &SpinBox::checkValue);
}

void SpinBox::checkValue(const QString& text_) {
  Q_UNUSED(text_);
  // if we delete everything in the lineedit, then we want to have an empty value
  // which is equivalent to the minimum, or special value text
  if(cleanText().isEmpty()) {
    setValue(minimum());
  }
}

QValidator::State SpinBox::validate(QString& text_, int& pos_) const {
  if(text_.endsWith(QLatin1Char(' '))) {
    text_.remove(text_.length()-1, 1);
  }
  return QSpinBox::validate(text_, pos_);
}

void SpinBox::stepBy(int steps_) {
  const int oldValue = value();
  const QString oldText = lineEdit()->text();

  QSpinBox::stepBy(steps_);

  // QT bug? Apparently, after the line edit is cleared, the internal value is not changed
  // then when the little buttons are clicked, the internal value is inserted in the line edit
  // but the valueChanged signal is not emitted
  if(oldText != lineEdit()->text() && oldValue == value()) {
    Q_EMIT valueChanged(value());
    Q_EMIT textChanged(text());
  }
}
