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

// The layout borrows heavily from kmsearchpatternedit.cpp in kmail
// which is authored by Marc Mutz <Marc@Mutz.com> under the GPL

#include "filterrulewidget.h"
#include "combobox.h"
#include "../document.h"
#include "../fieldcompletion.h"

#include <KLocalizedString>
#include <KComboBox>
#include <KLineEdit>
#include <KDateComboBox>

#include <QDialog>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPushButton>

using Tellico::FilterRuleWidget;

FilterRuleWidget::FilterRuleWidget(Tellico::FilterRule* rule_, QWidget* parent_)
    : QWidget(parent_), m_ruleDate(nullptr), m_ruleType(General) {
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(0);
//  l->setSizeConstraint(QLayout::SetFixedSize);
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  initLists();
  initWidget();

  if(rule_) {
    setRule(rule_);
  } else {
    reset();
  }
}

QString FilterRuleWidget::anyFieldString() {
  static const QString anyField = QLatin1Char('<') + i18n("Any Field") + QLatin1Char('>');
  return anyField;
}

void FilterRuleWidget::initLists() {
  //---------- initialize list of filter fields
  if(m_ruleFieldList.isEmpty()) {
    m_ruleFieldList.append(anyFieldString());
    QStringList titles = Data::Document::self()->collection()->fieldTitles();
    titles.sort();
    m_ruleFieldList += titles;
  }

}

void FilterRuleWidget::initWidget() {
  m_ruleField = new KComboBox(this);
  layout()->addWidget(m_ruleField);
  void (KComboBox::* activatedInt)(int) = &KComboBox::activated;
  // change the rule field before emitting modified
  connect(m_ruleField, activatedInt, this, &FilterRuleWidget::slotRuleFieldChanged);
  connect(m_ruleField, activatedInt, this, &FilterRuleWidget::signalModified);

  m_ruleFunc = new GUI::ComboBox(this);
  layout()->addWidget(m_ruleFunc);
  connect(m_ruleFunc, activatedInt, this, &FilterRuleWidget::slotRuleFunctionChanged);
  connect(m_ruleFunc, activatedInt, this, &FilterRuleWidget::signalModified);

  m_valueStack = new QStackedWidget(this);
  layout()->addWidget(m_valueStack);

  m_ruleValue = new KLineEdit(m_valueStack); //krazy:exclude=qclasses
  connect(m_ruleValue, &QLineEdit::textChanged, this, &FilterRuleWidget::signalModified);
  m_valueStack->addWidget(m_ruleValue);

  m_ruleDate = new KDateComboBox(m_valueStack);
  connect(m_ruleDate, &KDateComboBox::dateChanged, this, &FilterRuleWidget::signalModified);
  m_valueStack->addWidget(m_ruleDate);

  m_ruleField->addItems(m_ruleFieldList);
  updateFunctionList();
  slotRuleFunctionChanged(m_ruleFunc->currentIndex());
}

void FilterRuleWidget::slotRuleFieldChanged(int which_) {
  Q_UNUSED(which_);
  m_ruleType = General;
  const QString fieldTitle = m_ruleField->currentText();
  if(fieldTitle.isEmpty() || fieldTitle == anyFieldString()) {
    m_ruleValue->setCompletionObject(nullptr);
    updateFunctionList();
    return;
  }
  Data::FieldPtr field = Data::Document::self()->collection()->fieldByTitle(fieldTitle);
  if(field && field->hasFlag(Data::Field::AllowCompletion)) {
    FieldCompletion* completion = new FieldCompletion(field->hasFlag(Data::Field::AllowMultiple));
    completion->setItems(Data::Document::self()->collection()->valuesByFieldName(field->name()));
    completion->setIgnoreCase(true);
    m_ruleValue->setCompletionObject(completion);
    m_ruleValue->setAutoDeleteCompletionObject(true);
  } else {
    m_ruleValue->setCompletionObject(nullptr);
  }

  if(field) {
    if(field->type() == Data::Field::Date) {
      m_ruleType = Date;
    } else if(field->type() == Data::Field::Number || field->type() == Data::Field::Rating) {
      m_ruleType = Number;
    } else if(field->type() == Data::Field::Image) {
      m_ruleType = Image;
    } else if(field->type() == Data::Field::Bool) {
      m_ruleType = Bool;
    }
  }
  updateFunctionList();
}

void FilterRuleWidget::slotRuleFunctionChanged(int which_) {
  const QVariant data = m_ruleFunc->itemData(which_);

  // don't show the date picker if we're using regular expressions
  if(m_ruleType == Date && data != FilterRule::FuncRegExp && data != FilterRule::FuncNotRegExp) {
    m_valueStack->setCurrentWidget(m_ruleDate);
  } else {
    m_valueStack->setCurrentWidget(m_ruleValue);
    m_ruleValue->setPlaceholderText(m_ruleType == Bool ? QChar(0x2611): QString());
    if(m_ruleType == Number &&
      (data != FilterRule::FuncRegExp && data != FilterRule::FuncNotRegExp)) {
      m_ruleValue->setValidator(new QIntValidator(this));
    } else {
      m_ruleValue->setValidator(nullptr);
    }
    // Bool rules don't use a value
    m_ruleValue->setEnabled(m_ruleType != Bool);
  }
}

void FilterRuleWidget::setRule(const Tellico::FilterRule* rule_) {
  if(!rule_) {
    reset();
    return;
  }

  blockSignals(true);

  m_ruleType = General;
  if(rule_->fieldName().isEmpty()) {
    m_ruleField->setCurrentIndex(0); // "All Fields"
  } else {
    Data::FieldPtr field = Data::Document::self()->collection()->fieldByName(rule_->fieldName());
    if(field && field->type() == Data::Field::Date) {
      m_ruleType = Date;
      const QDate date = QDate::fromString(rule_->pattern(), Qt::ISODate);
      if(date.isValid()) {
        m_ruleDate->setDate(date);
      }
    }
    const int idx = m_ruleField->findText(field ? field->title() : QString());
    m_ruleField->setCurrentIndex(idx);
  }

  // update the rule fields first, before possible values
  slotRuleFieldChanged(m_ruleField->currentIndex());

  //--------------set function and contents
  m_ruleFunc->setCurrentData(rule_->function());
  m_ruleValue->setText(m_ruleType == Bool ? QString() : rule_->pattern());

  slotRuleFunctionChanged(m_ruleFunc->currentIndex());
  blockSignals(false);
}

Tellico::FilterRule* FilterRuleWidget::rule() const {
  QString fieldName; // empty string
  if(m_ruleField->currentIndex() > 0) { // 0 is "All Fields", field is empty
    fieldName = Data::Document::self()->collection()->fieldNameByTitle(m_ruleField->currentText());
  }

  QString ruleValue;
  if(m_valueStack->currentWidget() == m_ruleDate) {
    ruleValue = m_ruleDate->date().toString(Qt::ISODate);
  } else {
    // bool values only get checked against true
    ruleValue = m_ruleType == Bool ? QStringLiteral("true") : m_ruleValue->text().trimmed();
  }

  return new FilterRule(fieldName, ruleValue,
                        static_cast<FilterRule::Function>(m_ruleFunc->currentData().toInt()));
}

void FilterRuleWidget::reset() {
  blockSignals(true);

  m_ruleField->setCurrentIndex(0);
  m_ruleFunc->setCurrentIndex(0);
  m_ruleValue->clear();

  blockSignals(false);
}

void FilterRuleWidget::setFocus() {
  m_ruleValue->setFocus();
}

void FilterRuleWidget::updateFunctionList() {
  Q_ASSERT(m_ruleFunc);
  const QVariant data = m_ruleFunc->currentData();
  m_ruleFunc->clear();
  switch(m_ruleType) {
    case Date:
      m_ruleFunc->addItem(i18n("equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("does not equal"), FilterRule::FuncNotEquals);
      m_ruleFunc->addItem(i18n("matches regular expression"), FilterRule::FuncRegExp);
      m_ruleFunc->addItem(i18n("does not match regular expression"), FilterRule::FuncNotRegExp);
      m_ruleFunc->addItem(i18nc("is before a date", "is before"), FilterRule::FuncBefore);
      m_ruleFunc->addItem(i18nc("is after a date", "is after"), FilterRule::FuncAfter);
      break;
    case Number:
      m_ruleFunc->addItem(i18n("equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("does not equal"), FilterRule::FuncNotEquals);
      m_ruleFunc->addItem(i18n("matches regular expression"), FilterRule::FuncRegExp);
      m_ruleFunc->addItem(i18n("does not match regular expression"), FilterRule::FuncNotRegExp);
      m_ruleFunc->addItem(i18nc("is less than a number", "is less than"), FilterRule::FuncLess);
      m_ruleFunc->addItem(i18nc("is greater than a number", "is greater than"), FilterRule::FuncGreater);
      break;
    case Image:
      m_ruleFunc->addItem(i18n("image size equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("image size does not equal"), FilterRule::FuncNotEquals);
      m_ruleFunc->addItem(i18nc("image size is less than a number", "image size is less than"), FilterRule::FuncLess);
      m_ruleFunc->addItem(i18nc("image size is greater than a number", "image size is greater than"), FilterRule::FuncGreater);
      break;
    case Bool:
      m_ruleFunc->addItem(i18n("equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("does not equal"), FilterRule::FuncNotEquals);
      break;
    case General:
      m_ruleFunc->addItem(i18n("contains"), FilterRule::FuncContains);
      m_ruleFunc->addItem(i18n("does not contain"), FilterRule::FuncNotContains);
      m_ruleFunc->addItem(i18n("equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("does not equal"), FilterRule::FuncNotEquals);
      m_ruleFunc->addItem(i18n("matches regular expression"), FilterRule::FuncRegExp);
      m_ruleFunc->addItem(i18n("does not match regular expression"), FilterRule::FuncNotRegExp);
      break;
  }
  m_ruleFunc->setCurrentData(data);
  slotRuleFunctionChanged(m_ruleFunc->currentIndex());
}
