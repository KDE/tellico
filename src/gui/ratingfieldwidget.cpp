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

#include "ratingfieldwidget.h"
#include "ratingwidget.h"
#include "../field.h"
#include "../tellico_debug.h"

using Tellico::GUI::RatingFieldWidget;

RatingFieldWidget::RatingFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {
  m_rating = new RatingWidget(field_, this);
  connect(m_rating, &RatingWidget::signalModified, this, &RatingFieldWidget::checkModified);

  registerWidget();
}

QString RatingFieldWidget::text() const {
  return m_rating->text();
}

void RatingFieldWidget::setTextImpl(const QString& text_) {
  m_rating->setText(text_);
}

void RatingFieldWidget::clearImpl() {
  m_rating->clear();
  editMultiple(false);
}

void RatingFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  m_rating->updateField(newField_);
}

QWidget* RatingFieldWidget::widget() {
  return m_rating;
}
