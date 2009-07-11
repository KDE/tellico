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

#include "fieldwidget.h"
#include "linefieldwidget.h"
#include "parafieldwidget.h"
#include "boolfieldwidget.h"
#include "choicefieldwidget.h"
#include "numberfieldwidget.h"
#include "urlfieldwidget.h"
#include "imagefieldwidget.h"
#include "datefieldwidget.h"
#include "tablefieldwidget.h"
#include "ratingfieldwidget.h"
#include "../field.h"
#include "../tellico_debug.h"

#include <kdebug.h>
#include <kurllabel.h>
#include <klocale.h>

#include <QWhatsThis>
#include <QRegExp>
#include <QLabel>
#include <QCheckBox>
#include <QStyle>
#include <QTimer>
#include <QHBoxLayout>

namespace {
  // if you change this, update numberfieldwidget, too
  const int FIELD_EDIT_WIDGET_INDEX = 2;
}

using Tellico::GUI::FieldWidget;

const QRegExp FieldWidget::s_semiColon = QRegExp(QLatin1String("\\s*;\\s*"));

FieldWidget* FieldWidget::create(Tellico::Data::FieldPtr field_, QWidget* parent_) {
  switch (field_->type()) {
    case Data::Field::Line:
      return new GUI::LineFieldWidget(field_, parent_);

    case Data::Field::Para:
      return new GUI::ParaFieldWidget(field_, parent_);

    case Data::Field::Bool:
      return new GUI::BoolFieldWidget(field_, parent_);

    case Data::Field::Number:
      return new GUI::NumberFieldWidget(field_, parent_);

    case Data::Field::Choice:
      return new GUI::ChoiceFieldWidget(field_, parent_);

    case Data::Field::Table:
    case Data::Field::Table2:
      return new GUI::TableFieldWidget(field_, parent_);

    case Data::Field::Date:
      return new GUI::DateFieldWidget(field_, parent_);

    case Data::Field::URL:
      return new GUI::URLFieldWidget(field_, parent_);

    case Data::Field::Image:
      return new GUI::ImageFieldWidget(field_, parent_);

    case Data::Field::Rating:
      return new GUI::RatingFieldWidget(field_, parent_);

    case Data::Field::ReadOnly:
    case Data::Field::Dependent:
      myWarning() << "read-only/dependent field, this shouldn't have been called";
      return 0;

    default:
      myWarning() << "unknown field type = " << field_->type();
      return 0;
  }
}

FieldWidget::FieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : QWidget(parent_), m_field(field_), m_settingText(false) {
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(2);
  l->setSpacing(2);
  l->addSpacing(4); // add some more space in the columns between widgets

  Data::Field::Type type = field_->type();
  QString s = i18nc("Edit Label", "%1:", field_->title());
  if(type == Data::Field::URL) {
    // set URL to empty for now
    m_label = new KUrlLabel(QString(), s, this);
  } else {
    m_label = new QLabel(s, this);
  }
  m_label->setFixedWidth(m_label->sizeHint().width());
  l->addWidget(m_label);

  // expands indicates if the edit widget should expand to full width of widget
  m_expands = (type == Data::Field::Line
               || type == Data::Field::Para
               || type == Data::Field::Number
               || type == Data::Field::URL
               || type == Data::Field::Table
               || type == Data::Field::Table2
               || type == Data::Field::Image
               || type == Data::Field::Date);

  m_editMultiple = new QCheckBox(this);
  m_editMultiple->setChecked(true);
  m_editMultiple->setFixedWidth(m_editMultiple->sizeHint().width()); // don't let it have any extra space
  connect(m_editMultiple, SIGNAL(toggled(bool)), SLOT(setEnabled(bool)));
  l->addWidget(m_editMultiple);

  setWhatsThis(field_->description());
  // after letting the subclass get created, insert default value
  QTimer::singleShot(0, this, SLOT(insertDefault()));
}

void FieldWidget::insertDefault() {
  setText(m_field->defaultValue());
}

bool FieldWidget::isEnabled() {
  return widget()->isEnabled();
}

void FieldWidget::setEnabled(bool enabled_) {
  if(enabled_ == isEnabled()) {
    return;
  }

  widget()->setEnabled(enabled_);
  m_editMultiple->setChecked(enabled_);
}

void FieldWidget::setText(const QString& text_) {
  m_oldValue = text_;
  m_settingText = true;
  setTextImpl(text_);
  m_settingText = false;
  // now check to see if the widget modified the text
  checkModified();
}

void FieldWidget::clear() {
  m_oldValue.clear();
  clearImpl();
}

int FieldWidget::labelWidth() const {
  return m_label->sizeHint().width();
}

void FieldWidget::setLabelWidth(int width_) {
  m_label->setFixedWidth(width_);
}

bool FieldWidget::expands() const {
  return m_expands;
}

void FieldWidget::editMultiple(bool show_) {
  if(show_ == !m_editMultiple->isHidden()) {
    return;
  }

  // FIXME: maybe valueChanged should only be signaled when the button is toggled on
  if(show_) {
    m_editMultiple->show();
    connect(m_editMultiple, SIGNAL(clicked()), this, SIGNAL(valueChanged()));
  } else {
    m_editMultiple->hide();
    disconnect(m_editMultiple, SIGNAL(clicked()), this, SIGNAL(valueChanged()));
  }
  // the widget length needs to be updated since it gets shorter
  widget()->updateGeometry();
}

void FieldWidget::registerWidget() {
  QWidget* w = widget();
  m_label->setBuddy(w);
  if(w->focusPolicy() != Qt::NoFocus) {
    setFocusProxy(w);
  }

  QHBoxLayout* l = static_cast<QHBoxLayout*>(layout());
  l->insertWidget(FIELD_EDIT_WIDGET_INDEX, w, m_expands ? 1 : 0 /*stretch*/);
  if(!m_expands) {
    l->insertStretch(FIELD_EDIT_WIDGET_INDEX+1, 1 /*stretch*/);
  }
  updateGeometry();
}

void FieldWidget::updateField(Tellico::Data::FieldPtr oldField_, Tellico::Data::FieldPtr newField_) {
  m_field = newField_;
  m_label->setText(i18nc("Edit Label", "%1:", newField_->title()));
  updateGeometry();
  setWhatsThis(newField_->description());
  updateFieldHook(oldField_, newField_);
}

void FieldWidget::checkModified() {
  // some of the widgets have signals that fire when the text is modified
  // in particular, the tablewidget used to prune empty rows when text() was called
  // just to guard against that in the future, don't emit anything if
  // we're in the process of setting the text
  if(m_settingText) {
    return;
  }
  const QString value = text();
  if(value != m_oldValue) {
//    myDebug() << "old value:" << m_oldValue << "| new value:" << value;
    m_oldValue = value;
    emit valueChanged();
  }
}

#include "fieldwidget.moc"
