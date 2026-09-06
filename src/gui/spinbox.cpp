/***************************************************************************
    Copyright (C) 2003-2026 Robby Stephenson <robby@periapsis.org>
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

SpinBox::SpinBox(qint64 min_, qint64 max_, QWidget* parent_)
    : QAbstractSpinBox(parent_), m_value(0), m_min(min_), m_max(max_)
{
  setAlignment(Qt::AlignRight);
  // I want to be able to have an empty value at the minimum
  // an empty string just removes the special value, so set white space
  setSpecialValueText(QStringLiteral(" "));

  connect(lineEdit(), &QLineEdit::textEdited, this, [this](const QString& text) {
    if(text.isEmpty()) {
      m_value = minimum();
      Q_EMIT valueChanged(m_value);
      Q_EMIT textChanged(QString());
      return;
    }
    bool ok;
    const qint64 val = text.toLongLong(&ok);
    if(ok) {
      lineEdit()->blockSignals(true);
      setValue(val);
      lineEdit()->blockSignals(false);
    }
  });
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
  if(text_.isEmpty() || text_ == QLatin1StringView(" ")) {
    text_.clear();
    pos_ = 0;
    return QValidator::Intermediate;
  }
  if(text_.endsWith(QLatin1Char(' '))) {
    if(pos_ == text_.length()) --pos_;
    text_.chop(1);
  }

  bool ok;
  const qint64 val = text_.toLongLong(&ok);

  if(!ok) return QValidator::Invalid;

  if(val < m_min || val > m_max) return QValidator::Invalid;
  return QValidator::Acceptable;
}

void SpinBox::stepBy(int steps_) {
  setValue(m_value + steps_);
}

void SpinBox::setValue(qint64 val_) {
  if(val_ < m_min) val_ = m_min;
  if(val_ > m_max) val_ = m_max;

  if(m_value != val_) {
    m_value = val_;

    if(m_value == m_min) {
      lineEdit()->setText(specialValueText());
    } else {
      const QString textVal = QString::number(m_value);
      lineEdit()->setText(textVal);
    }

    Q_EMIT valueChanged(m_value);
    Q_EMIT textChanged(lineEdit()->text());
  }
}

QString SpinBox::cleanText() const {
  return lineEdit()->text().trimmed();
}

QAbstractSpinBox::StepEnabled SpinBox::stepEnabled() const {
  QAbstractSpinBox::StepEnabled flags = QAbstractSpinBox::StepNone;
  if (m_value > m_min) flags |= QAbstractSpinBox::StepDownEnabled;
  if (m_value < m_max) flags |= QAbstractSpinBox::StepUpEnabled;
  return flags;
}
