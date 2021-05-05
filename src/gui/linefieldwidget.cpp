/***************************************************************************
    Copyright (C) 2005-2021 Robby Stephenson <robby@periapsis.org>
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

#include "linefieldwidget.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../fieldcompletion.h"
#include "../document.h"
#include "../gui/lineedit.h"
#include "../utils/isbnvalidator.h"

using Tellico::GUI::LineFieldWidget;

LineFieldWidget::LineFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_lineEdit = new GUI::LineEdit(this);
  m_lineEdit->setAllowSpellCheck(true);
  m_lineEdit->setEnableSpellCheck(field_->formatType() != FieldFormat::FormatName);
  connect(m_lineEdit, &QLineEdit::textChanged, this, &LineFieldWidget::checkModified);

  registerWidget();

  if(field_->hasFlag(Data::Field::AllowCompletion)) {
    createCompletionObject(field_->name());
  }

  if(field_->name() == QLatin1String("isbn")) {
    m_lineEdit->setValidator(new ISBNValidator(this));
  }
}

QString LineFieldWidget::text() const {
  QString text = m_lineEdit->text();
  if(field()->hasFlag(Data::Field::AllowMultiple)) {
    text = FieldFormat::fixupValue(text);
  }
  return text.trimmed();
}

void LineFieldWidget::setTextImpl(const QString& text_) {
  m_lineEdit->setText(text_);
}

void LineFieldWidget::clearImpl() {
  m_lineEdit->clear();
  editMultiple(false);
}

void LineFieldWidget::addCompletionObjectItem(const QString& text_) {
  m_lineEdit->completionObject()->addItem(text_);
}

void LineFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  bool wasComplete = m_lineEdit->compObj(); // don't call completionObject() since it recreates it
  bool isComplete = (newField_->hasFlag(Data::Field::AllowCompletion));
  if(!wasComplete && isComplete) {
    createCompletionObject(newField_->name());
  } else if(wasComplete && !isComplete) {
    // auto-deleted (but re-created if completionObject() is called again)
    m_lineEdit->setCompletionObject(nullptr);
  }
}

QWidget* LineFieldWidget::widget() {
  return m_lineEdit;
}

void LineFieldWidget::createCompletionObject(const QString& fieldName_) {
  Q_ASSERT(m_lineEdit);
  FieldCompletion* completion = new FieldCompletion(true);
  completion->setItems(Data::Document::self()->collection()->valuesByFieldName(fieldName_));
  completion->setIgnoreCase(true);
  m_lineEdit->setCompletionObject(completion);
  m_lineEdit->setAutoDeleteCompletionObject(true);
}
