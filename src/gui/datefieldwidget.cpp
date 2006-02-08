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

#include "datefieldwidget.h"
#include "datewidget.h"
#include "../field.h"

using Tellico::GUI::DateFieldWidget;

DateFieldWidget::DateFieldWidget(Data::FieldPtr field_, QWidget* parent_, const char* name_/*=0*/)
    : FieldWidget(field_, parent_, name_) {

  m_widget = new DateWidget(this);
  connect(m_widget, SIGNAL(signalModified()), SIGNAL(modified()));

  registerWidget();
}

QString DateFieldWidget::text() const {
  return m_widget->text();
}

void DateFieldWidget::setText(const QString& text_) {
  blockSignals(true);
  m_widget->blockSignals(true);

  m_widget->setDate(text_);

  m_widget->blockSignals(false);
  blockSignals(false);
}

void DateFieldWidget::clear() {
  m_widget->clear();
  editMultiple(false);
}

QWidget* DateFieldWidget::widget() {
  return m_widget;
}

#include "datefieldwidget.moc"
