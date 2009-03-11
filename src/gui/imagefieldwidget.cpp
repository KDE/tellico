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

#include "imagefieldwidget.h"
#include "imagewidget.h"
#include "../field.h"

using Tellico::GUI::ImageFieldWidget;

ImageFieldWidget::ImageFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_widget = new ImageWidget(this);
  m_widget->setLinkOnlyChecked(field_->property(QLatin1String("link")) == QLatin1String("true"));
  connect(m_widget, SIGNAL(signalModified()), SLOT(checkModified()));

  registerWidget();
}

QString ImageFieldWidget::text() const {
  return m_widget->id();
}

void ImageFieldWidget::setTextImpl(const QString& text_) {
  m_widget->setImage(text_);
}

void ImageFieldWidget::clearImpl() {
  m_widget->slotClear();
  editMultiple(false);
}

QWidget* ImageFieldWidget::widget() {
  return m_widget;
}

#include "imagefieldwidget.moc"
