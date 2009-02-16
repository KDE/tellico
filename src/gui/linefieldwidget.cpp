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

#include "linefieldwidget.h"
#include "../field.h"
#include "../isbnvalidator.h"
#include "../fieldcompletion.h"
#include "../tellico_kernel.h"
#include "../gui/lineedit.h"

using Tellico::GUI::LineFieldWidget;

LineFieldWidget::LineFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_lineEdit = new GUI::LineEdit(this);
  m_lineEdit->setAllowSpellCheck(true);
  m_lineEdit->setEnableSpellCheck(field_->formatFlag() != Tellico::Data::Field::FormatName);
  connect(m_lineEdit, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));

  registerWidget();

  if(field_->flags() & Data::Field::AllowCompletion) {
    FieldCompletion* completion = new FieldCompletion(field_->flags() & Tellico::Data::Field::AllowMultiple);
    completion->setItems(Kernel::self()->valuesByFieldName(field_->name()));
    completion->setIgnoreCase(true);
    m_lineEdit->setCompletionObject(completion);
    m_lineEdit->setAutoDeleteCompletionObject(true);
  }

  if(field_->name() == QLatin1String("isbn")) {
    m_lineEdit->setValidator(new ISBNValidator(this));
  }
}

QString LineFieldWidget::text() const {
  QString text = m_lineEdit->text();
  if(field()->flags() & Data::Field::AllowMultiple) {
    text.replace(s_semiColon, QString::fromLatin1("; "));
  }
  return text.trimmed();
}

void LineFieldWidget::setText(const QString& text_) {
  blockSignals(true);
  m_lineEdit->blockSignals(true);
  m_lineEdit->setText(text_);
  m_lineEdit->blockSignals(false);
  blockSignals(false);
}

void LineFieldWidget::clear() {
  m_lineEdit->clear();
  editMultiple(false);
}

void LineFieldWidget::addCompletionObjectItem(const QString& text_) {
  m_lineEdit->completionObject()->addItem(text_);
}

void LineFieldWidget::updateFieldHook(Tellico::Data::FieldPtr oldField_, Tellico::Data::FieldPtr newField_) {
  bool wasComplete = (oldField_->flags() & Tellico::Data::Field::AllowCompletion);
  bool isComplete = (newField_->flags() & Tellico::Data::Field::AllowCompletion);
  if(!wasComplete && isComplete) {
    FieldCompletion* completion = new FieldCompletion(isComplete);
    completion->setItems(Kernel::self()->valuesByFieldName(newField_->name()));
    completion->setIgnoreCase(true);
    m_lineEdit->setCompletionObject(completion);
    m_lineEdit->setAutoDeleteCompletionObject(true);
  } else if(wasComplete && !isComplete) {
    m_lineEdit->completionObject()->clear();
  }
}

QWidget* LineFieldWidget::widget() {
  return m_lineEdit;
}

#include "linefieldwidget.moc"
