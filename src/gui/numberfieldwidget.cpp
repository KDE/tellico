/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "numberfieldwidget.h"
#include "spinbox.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../tellico_debug.h"

#include <QLineEdit>
#include <QValidator>
#include <QBoxLayout>

#include <limits>

using Tellico::GUI::NumberFieldWidget;

NumberFieldWidget::NumberFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_), m_lineEdit(nullptr), m_spinBox(nullptr) {

  if(field_->hasFlag(Data::Field::AllowMultiple)) {
    initLineEdit();
  } else {
    initSpinBox();
  }

  registerWidget();
}

void NumberFieldWidget::initLineEdit() {
  m_lineEdit = new QLineEdit(this);
  connect(m_lineEdit, &QLineEdit::textChanged, this, &NumberFieldWidget::checkModified);

  // regexp is any number of digits followed optionally by any number of
  // groups of a semi-colon followed optionally by a space, followed by digits
  QRegularExpression rx(QLatin1String("-?\\d*(; ?-?\\d*)*"));
  m_lineEdit->setValidator(new QRegularExpressionValidator(rx, this));
}

void NumberFieldWidget::initSpinBox() {
  // intentionally allow only positive numbers. -1 means non empty
  m_spinBox = new GUI::SpinBox(-1, std::numeric_limits<int>::max(), this);
  m_spinBox->setValue(-1);
  void (GUI::SpinBox::* textChanged)(const QString&) = &GUI::SpinBox::textChanged;
  connect(m_spinBox, textChanged, this, &NumberFieldWidget::checkModified);
}

QString NumberFieldWidget::text() const {
  if(isSpinBox()) {
    return m_spinBox->cleanText();
  }

  QString text = m_lineEdit->text();
  if(field()->hasFlag(Data::Field::AllowMultiple)) {
    text = FieldFormat::fixupValue(text);
  }
  return text.simplified();
}

void NumberFieldWidget::setTextImpl(const QString& text_) {
  if(isSpinBox()) {
    if(text_.isEmpty()) {
      m_spinBox->clear();
      return;
    }
    bool ok;
    int n = text_.toInt(&ok);
    if(ok) {
      // did just allow positive
      if(n < m_spinBox->minimum()+1) {
        m_spinBox->setMinimum(std::numeric_limits<int>::min()+1);
      }
      m_spinBox->setValue(n);
    }
  } else {
    m_lineEdit->setText(text_);
  }
}

void NumberFieldWidget::clearImpl() {
  if(isSpinBox()) {
    m_spinBox->clear();
  } else {
    m_lineEdit->clear();
  }
  editMultiple(false);
}

void NumberFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  bool wasLineEdit = !isSpinBox();
  bool nowLineEdit = newField_->hasFlag(Data::Field::AllowMultiple);

  if(wasLineEdit == nowLineEdit) {
    return;
  }

  const int widgetIndex = layout()->indexOf(widget());
  Q_ASSERT(widgetIndex > -1);

  const QString value = text();
  if(wasLineEdit && !nowLineEdit) {
    layout()->removeWidget(m_lineEdit);
    delete m_lineEdit;
    m_lineEdit = nullptr;
    initSpinBox();
  } else if(!wasLineEdit && nowLineEdit) {
    layout()->removeWidget(m_spinBox);
    delete m_spinBox;
    m_spinBox = nullptr;
    initLineEdit();
  }

  static_cast<QBoxLayout*>(layout())->insertWidget(widgetIndex, widget(), 1 /*stretch*/);
  widget()->show();
  setText(value);
}

QWidget* NumberFieldWidget::widget() {
  if(isSpinBox()) {
    return m_spinBox;
  }
  return m_lineEdit;
}
