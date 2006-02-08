/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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
#include "../latin1literal.h"

#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>

using Tellico::GUI::BoolFieldWidget;

BoolFieldWidget::BoolFieldWidget(Data::FieldPtr field_, QWidget* parent_, const char* name_/*=0*/)
    : FieldWidget(field_, parent_, name_) {

  m_checkBox = new QCheckBox(this);
  connect(m_checkBox, SIGNAL(clicked()), SIGNAL(modified()));
  registerWidget();
}

QString BoolFieldWidget::text() const {
  if(m_checkBox->isChecked()) {
    return QString::fromLatin1("true");
  }

  return QString();
}

void BoolFieldWidget::setText(const QString& text_) {
  blockSignals(true);

  m_checkBox->blockSignals(true);
  // be lax, don't have to check for "1" or "true"
  // just check for a non-empty string
  m_checkBox->setChecked(!text_.isEmpty());
  m_checkBox->blockSignals(false);

  blockSignals(false);
}

void BoolFieldWidget::clear() {
  m_checkBox->setChecked(false);
  editMultiple(false);
}

QWidget* BoolFieldWidget::widget() {
  return m_checkBox;
}

#include "boolfieldwidget.moc"
