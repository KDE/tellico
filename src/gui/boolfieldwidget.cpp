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

#include "boolfieldwidget.h"
#include "../field.h"

#include <QCheckBox>

using Tellico::GUI::BoolFieldWidget;

BoolFieldWidget::BoolFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_checkBox = new QCheckBox(this);
  connect(m_checkBox, SIGNAL(clicked()), SLOT(checkModified()));
  registerWidget();
}

QString BoolFieldWidget::text() const {
  if(m_checkBox->isChecked()) {
    return QLatin1String("true");
  }

  return QString();
}

void BoolFieldWidget::setTextImpl(const QString& text_) {
  // be lax, don't have to check for "1" or "true"
  // just check for a non-empty string
  m_checkBox->setChecked(!text_.isEmpty());
}

void BoolFieldWidget::clearImpl() {
  m_checkBox->setChecked(false);
  editMultiple(false);
}

QWidget* BoolFieldWidget::widget() {
  return m_checkBox;
}

#include "boolfieldwidget.moc"
