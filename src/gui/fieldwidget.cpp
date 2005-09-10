/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
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

#include <kdebug.h>
#include <kurllabel.h>

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qstyle.h>

namespace {
  // if you change this, update numberfieldwidget, too
  const int FIELD_EDIT_WIDGET_INDEX = 2;
}

using Tellico::GUI::FieldWidget;

const QRegExp FieldWidget::s_semiColon = QRegExp(QString::fromLatin1("\\s*;\\s*"));
const QRegExp FieldWidget::s_comma = QRegExp(QString::fromLatin1("\\s*,\\s*"));

FieldWidget* FieldWidget::create(const Data::Field* field_, QWidget* parent_, const char* name_) {
  switch (field_->type()) {
    case Data::Field::Line:
      return new GUI::LineFieldWidget(field_, parent_, name_);

    case Data::Field::Para:
      return new GUI::ParaFieldWidget(field_, parent_, name_);

    case Data::Field::Bool:
      return new GUI::BoolFieldWidget(field_, parent_, name_);

    case Data::Field::Number:
      return new GUI::NumberFieldWidget(field_, parent_, name_);

    case Data::Field::Choice:
      return new GUI::ChoiceFieldWidget(field_, parent_, name_);

    case Data::Field::Table:
    case Data::Field::Table2:
      return new GUI::TableFieldWidget(field_, parent_, name_);

    case Data::Field::Date:
      return new GUI::DateFieldWidget(field_, parent_, name_);

    case Data::Field::URL:
      return new GUI::URLFieldWidget(field_, parent_, name_);

    case Data::Field::Image:
      return new GUI::ImageFieldWidget(field_, parent_, name_);

    case Data::Field::Rating:
      return new GUI::RatingFieldWidget(field_, parent_, name_);

    case Data::Field::ReadOnly:
    case Data::Field::Dependent:
      kdWarning() << "FieldWidget::create() - read-only/dependent field, this shouldn't have been called" << endl;
      return 0;

    default:
      kdWarning() << "FieldWidget::create() - unknown field type = " << field_->type() << endl;
      return 0;
  }
}

FieldWidget::FieldWidget(const Data::Field* field_, QWidget* parent_, const char* name_/*=0*/)
    : QWidget(parent_, name_) {
  QHBoxLayout* l = new QHBoxLayout(this, 2, 2); // parent, margin, spacing
  l->addSpacing(4); // add some more space in the columns between widgets
  if(QCString(style().name()).lower().find("keramik", 0, false) > -1) {
    l->setMargin(1);
  }

  Data::Field::Type type = field_->type();
  if(type == Data::Field::URL) {
    // set URL to null for now
    m_label = new KURLLabel(QString::null, field_->title() + QChar(':'), this);
  } else {
    m_label = new QLabel(field_->title() + QChar(':'), this);
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

  QWhatsThis::add(this, field_->description());
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
  if(show_ == m_editMultiple->isShown()) {
    return;
  }

  // FIXME: maybe modified should only be signaled when the button is toggle on
  if(show_) {
    m_editMultiple->show();
    connect(m_editMultiple, SIGNAL(clicked()), this, SIGNAL(modified()));
  } else {
    m_editMultiple->hide();
    disconnect(m_editMultiple, SIGNAL(clicked()), this, SIGNAL(modified()));
  }
  // the widget length needs to be updated since it gets shorter
  widget()->updateGeometry();
}

void FieldWidget::registerWidget() {
  m_label->setBuddy(widget());

  QHBoxLayout* l = static_cast<QHBoxLayout*>(layout());
  l->insertWidget(FIELD_EDIT_WIDGET_INDEX, widget(), m_expands ? 1 : 0 /*stretch*/);
  if(!m_expands) {
    l->insertStretch(FIELD_EDIT_WIDGET_INDEX+1, 1 /*stretch*/);
  }
  updateGeometry();
}

void FieldWidget::updateField(Data::Field* oldField_, Data::Field* newField_) {
  m_label->setText(newField_->title() + QString::fromLatin1(":"));
  updateGeometry();
  QWhatsThis::add(this, newField_->description());
  updateFieldHook(oldField_, newField_);
}

#include "fieldwidget.moc"
