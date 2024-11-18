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

#include "boolfieldwidget.h"
#include "../field.h"

#include <QCheckBox>

using Tellico::GUI::BoolFieldWidget;

BoolFieldWidget::BoolFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {
  m_checkBox = new QCheckBox(this);
#if (QT_VERSION < QT_VERSION_CHECK(6, 7, 0))
  connect(m_checkBox, &QCheckBox::stateChanged, this, &BoolFieldWidget::checkModified);
#else
  connect(m_checkBox, &QCheckBox::checkStateChanged, this, &BoolFieldWidget::checkModified);
#endif
  registerWidget();
}

QString BoolFieldWidget::text() const {
  if(m_checkBox->isChecked()) {
    return QStringLiteral("true");
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
