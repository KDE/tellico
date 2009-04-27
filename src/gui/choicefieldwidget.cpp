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
