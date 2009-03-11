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

#include "choicefieldwidget.h"
#include "../field.h"

#include <kcombobox.h>

using Tellico::GUI::ChoiceFieldWidget;

ChoiceFieldWidget::ChoiceFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_), m_comboBox(0) {

  m_comboBox = new KComboBox(this);
  connect(m_comboBox, SIGNAL(activated(int)), SLOT(checkModified()));
  // always have empty choice
  m_comboBox->addItem(QString());
  m_comboBox->addItems(field_->allowed());
  m_comboBox->setMinimumWidth(5*fontMetrics().maxWidth());

  registerWidget();
}

QString ChoiceFieldWidget::text() const {
  return m_comboBox->currentText();
}

void ChoiceFieldWidget::setTextImpl(const QString& text_) {
  int idx = m_comboBox->findText(text_);
  if(idx < 0) {
    m_comboBox->addItem(text_);
  } else {
    m_comboBox->setCurrentIndex(idx);
  }
  m_comboBox->setCurrentItem(text_);
}

void ChoiceFieldWidget::clearImpl() {
  m_comboBox->setCurrentIndex(0); // first item is empty
  editMultiple(false);
}

void ChoiceFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  int idx = m_comboBox->currentIndex();
  m_comboBox->clear();
  // always have empty choice
  m_comboBox->addItem(QString());
  m_comboBox->addItems(newField_->allowed());
  m_comboBox->setCurrentIndex(idx);
}

QWidget* ChoiceFieldWidget::widget() {
  return m_comboBox;
}

#include "choicefieldwidget.moc"
