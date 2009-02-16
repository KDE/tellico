/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "numberfieldwidget.h"
#include "datewidget.h"
#include "../field.h"

#include <klineedit.h>

#include <QValidator>
#include <QBoxLayout>

using Tellico::GUI::NumberFieldWidget;

NumberFieldWidget::NumberFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_), m_lineEdit(0), m_spinBox(0) {

  if(field_->flags() & Data::Field::AllowMultiple) {
    initLineEdit();
  } else {
    initSpinBox();
  }

  registerWidget();
}

void NumberFieldWidget::initLineEdit() {
  m_lineEdit = new KLineEdit(this);
  connect(m_lineEdit, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));

  // regexp is any number of digits followed optionally by any number of
  // groups of a semi-colon followed optionally by a space, followed by digits
  QRegExp rx(QLatin1String("-?\\d*(; ?-?\\d*)*"));
  m_lineEdit->setValidator(new QRegExpValidator(rx, this));
}

void NumberFieldWidget::initSpinBox() {
  // intentionally allow only positive numbers
  m_spinBox = new GUI::SpinBox(-1, INT_MAX, this);
  connect(m_spinBox, SIGNAL(valueChanged(int)), SIGNAL(modified()));
  // QSpinBox doesn't emit valueChanged if you edit the value with
  // the lineEdit until you change the keyboard focus. Fixed in QT4 ???
//  connect(m_spinBox->child("qt_spinbox_edit"), SIGNAL(textChanged(const QString&)), SIGNAL(modified()));
  // I want to allow no value, so set space as special text. Empty text is ignored
  m_spinBox->setSpecialValueText(QChar(' '));
}

QString NumberFieldWidget::text() const {
  if(isSpinBox()) {
    // minValue = special empty text
    if(m_spinBox->value() > m_spinBox->minimum()) {
      return QString::number(m_spinBox->value());
    }
    return QString();
  }

  QString text = m_lineEdit->text();
  if(field()->flags() & Data::Field::AllowMultiple) {
    text.replace(s_semiColon, QLatin1String("; "));
  }
  return text.simplified();
}

void NumberFieldWidget::setText(const QString& text_) {
  blockSignals(true);

  if(isSpinBox()) {
    bool ok;
    int n = text_.toInt(&ok);
    if(ok) {
      // did just allow positive
      if(n < 0) {
        m_spinBox->setMinimum(INT_MIN+1);
      }
      m_spinBox->blockSignals(true);
      m_spinBox->setValue(n);
      m_spinBox->blockSignals(false);
    }
  } else {
    m_lineEdit->blockSignals(true);
    m_lineEdit->setText(text_);
    m_lineEdit->blockSignals(false);
  }

  blockSignals(false);
}

void NumberFieldWidget::clear() {
  if(isSpinBox()) {
    // show empty special value text
    m_spinBox->setValue(m_spinBox->minimum());
  } else {
    m_lineEdit->clear();
  }
  editMultiple(false);
}

void NumberFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  bool wasLineEdit = !isSpinBox();
  bool nowLineEdit = newField_->flags() & Tellico::Data::Field::AllowMultiple;

  if(wasLineEdit == nowLineEdit) {
    return;
  }

  QString value = text();
  if(wasLineEdit && !nowLineEdit) {
    layout()->removeWidget(m_lineEdit);
    delete m_lineEdit;
    m_lineEdit = 0;
    initSpinBox();
  } else if(!wasLineEdit && nowLineEdit) {
    layout()->removeWidget(m_spinBox);
    delete m_spinBox;
    m_spinBox = 0;
    initLineEdit();
  }

  // should really be FIELD_EDIT_WIDGET_INDEX from fieldwidget.cpp
  static_cast<QBoxLayout*>(layout())->insertWidget(2, widget(), 1 /*stretch*/);
  widget()->show();
  setText(value);
}

QWidget* NumberFieldWidget::widget() {
  if(isSpinBox()) {
    return m_spinBox;
  }
  return m_lineEdit;
}

#include "numberfieldwidget.moc"
