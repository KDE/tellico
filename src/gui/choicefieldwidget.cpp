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
#include "checkablecombobox.h"
#include "../field.h"

#include <QGuiApplication>
#include <QScreen>
#include <QBoxLayout>

namespace {
  const double MAX_FRACTION_SCREEN_WIDTH = 0.4;
}

using Tellico::GUI::ChoiceFieldWidget;

ChoiceFieldWidget::ChoiceFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_), m_comboBox(nullptr), m_checkableComboBox(nullptr) {

  if(field_->hasFlag(Data::Field::AllowMultiple)) {
    initCheckableComboBox();
  } else {
    initComboBox();
  }
  initCommon(field_);

  registerWidget();
}

void ChoiceFieldWidget::initComboBox() {
  m_comboBox = new QComboBox(this);
  void (QComboBox::* indexChanged)(int) = &QComboBox::currentIndexChanged;
  connect(m_comboBox, indexChanged, this, &ChoiceFieldWidget::checkModified);
}

void ChoiceFieldWidget::initCheckableComboBox() {
  m_checkableComboBox = new CheckableComboBox(this);
  m_checkableComboBox->setSeparator(FieldFormat::delimiterString());
  connect(m_checkableComboBox, &CheckableComboBox::checkedItemsChanged, this, &ChoiceFieldWidget::checkModified);
}

void ChoiceFieldWidget::initCommon(Tellico::Data::FieldPtr field_) {
  m_maxTextWidth = MAX_FRACTION_SCREEN_WIDTH * QGuiApplication::primaryScreen()->size().width();

  QStringList values = field_->allowed();
  // always have empty choice, but don't show two empty values
  values.removeAll(QString());

  const QFontMetrics& fm = fontMetrics();
  auto cb = comboBox(); // everything here applies to either type of combobox;
  cb->addItem(QString(), QString());
  foreach(const QString& value, values) {
    cb->addItem(fm.elidedText(value, Qt::ElideMiddle, m_maxTextWidth), value);
  }
  cb->setMinimumWidth(5*fm.maxWidth());
}

QString ChoiceFieldWidget::text() const {
  return isMultiSelect() ? comboBox()->currentText()
                         : comboBox()->currentData().toString();
}

void ChoiceFieldWidget::setTextImpl(const QString& text_) {
  auto cb = comboBox();
  if(isMultiSelect()) {
    // first uncheck all the boxes
    m_checkableComboBox->setAllCheckState(Qt::Unchecked);
    const auto values = FieldFormat::splitValue(text_, FieldFormat::StringSplit);
    for(const auto& value : qAsConst(values)) {
      int idx = cb->findData(value);
      if(idx < 0) {
        m_checkableComboBox->addItem(fontMetrics().elidedText(text_, Qt::ElideMiddle, m_maxTextWidth), text_);
        idx = m_checkableComboBox->count()-1;
      }
      m_checkableComboBox->setItemData(idx, Qt::Checked, Qt::CheckStateRole);
    }
  } else {
    int idx = cb->findData(text_);
    if(idx < 0) {
      m_comboBox->addItem(fontMetrics().elidedText(text_, Qt::ElideMiddle, m_maxTextWidth), text_);
      m_comboBox->setCurrentIndex(m_comboBox->count()-1);
    } else {
      m_comboBox->setCurrentIndex(idx);
    }
  }
}

void ChoiceFieldWidget::clearImpl() {
  comboBox()->setCurrentIndex(0); // first item is empty
  editMultiple(false);
}

void ChoiceFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  bool wasMultiSelect = isMultiSelect();
  bool nowMultiSelect = newField_->hasFlag(Data::Field::AllowMultiple);

  const QString oldValue = text();

  const int widgetIndex = layout()->indexOf(widget());
  Q_ASSERT(widgetIndex > -1);

  if(wasMultiSelect && !nowMultiSelect) {
    layout()->removeWidget(m_checkableComboBox);
    delete m_checkableComboBox;
    m_checkableComboBox = nullptr;
    initComboBox();
  } else if(!wasMultiSelect && nowMultiSelect) {
    layout()->removeWidget(m_comboBox);
    delete m_comboBox;
    m_comboBox = nullptr;
    initCheckableComboBox();
  } else {
    // remove existing values
    comboBox()->clear();
  }
  initCommon(newField_);

  static_cast<QBoxLayout*>(layout())->insertWidget(widgetIndex, widget(), 1 /*stretch*/);
  widget()->show();

  setTextImpl(oldValue); // set back to previous value
}

QComboBox* ChoiceFieldWidget::comboBox() const {
  return isMultiSelect() ? m_checkableComboBox : m_comboBox;
}

QWidget* ChoiceFieldWidget::widget() {
  return comboBox();
}
