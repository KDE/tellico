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

#include "imagefieldwidget.h"
#include "imagewidget.h"
#include "../field.h"

using Tellico::GUI::ImageFieldWidget;

ImageFieldWidget::ImageFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_widget = new ImageWidget(this);
  m_widget->setLinkOnlyChecked(field_->property(QStringLiteral("link")) == QLatin1String("true"));
  connect(m_widget, &ImageWidget::signalModified, this, &ImageFieldWidget::checkModified);

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

void ImageFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  m_widget->setLinkOnlyChecked(newField_->property(QStringLiteral("link")) == QLatin1String("true"));
}
