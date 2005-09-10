/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
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
#include "../latin1literal.h"
#include "../tellico_kernel.h"

#include <kdebug.h>
#include <klineedit.h>

using Tellico::GUI::LineFieldWidget;

LineFieldWidget::LineFieldWidget(const Data::Field* field_, QWidget* parent_, const char* name_/*=0*/)
    : FieldWidget(field_, parent_, name_) {

  m_lineEdit = new KLineEdit(this);
  connect(m_lineEdit, SIGNAL(textChanged(const QString&)), SIGNAL(modified()));

  registerWidget();

  if(field_->flags() & Data::Field::AllowCompletion) {
    FieldCompletion* completion = new FieldCompletion(field_->flags() & Data::Field::AllowMultiple);
    completion->setItems(Kernel::self()->valuesByFieldName(field_->name()));
    completion->setIgnoreCase(true);
    m_lineEdit->setCompletionObject(completion);
    m_lineEdit->setAutoDeleteCompletionObject(true);
  }

  if(field_->name() == Latin1Literal("isbn")) {
    m_lineEdit->setValidator(new ISBNValidator(this));
  }
}

QString LineFieldWidget::text() const {
  QString text = m_lineEdit->text();
  text.replace(s_semiColon, QString::fromLatin1("; "));
  text.replace(s_comma, QString::fromLatin1(", "));
  text.simplifyWhiteSpace();
  return text;
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

void LineFieldWidget::updateFieldHook(Data::Field* oldField_, Data::Field* newField_) {
  bool wasComplete = (oldField_->flags() & Data::Field::AllowCompletion);
  bool isComplete = (newField_->flags() & Data::Field::AllowCompletion);
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
