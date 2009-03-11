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

#include "datefieldwidget.h"
#include "datewidget.h"
#include "../field.h"

using Tellico::GUI::DateFieldWidget;

DateFieldWidget::DateFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_widget = new DateWidget(this);
  connect(m_widget, SIGNAL(signalModified()), SLOT(checkModified()));

  registerWidget();
}

QString DateFieldWidget::text() const {
  return m_widget->text();
}

void DateFieldWidget::setTextImpl(const QString& text_) {
  m_widget->setDate(text_);
}

void DateFieldWidget::clearImpl() {
  m_widget->clear();
  editMultiple(false);
}

QWidget* DateFieldWidget::widget() {
  return m_widget;
}

#include "datefieldwidget.moc"
