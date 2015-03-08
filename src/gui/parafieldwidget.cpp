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

#include "parafieldwidget.h"
#include "../field.h"

#include <ktextedit.h>

using Tellico::GUI::ParaFieldWidget;

ParaFieldWidget::ParaFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_textEdit = new KTextEdit(this);
  m_textEdit->setAcceptRichText(false);
  if(field_->property(QLatin1String("spellcheck")) != QLatin1String("false")) {
    m_textEdit->setCheckSpellingEnabled(true);
  }
  connect(m_textEdit, SIGNAL(textChanged()), SLOT(checkModified()));

  registerWidget();
}

QString ParaFieldWidget::text() const {
  QString text = m_textEdit->toPlainText();
  text.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
  return text;
}

void ParaFieldWidget::setTextImpl(const QString& text_) {
  QRegExp rx(QLatin1String("<br/?>"), Qt::CaseInsensitive);
  QString s = text_;
  s.replace(rx, QLatin1String("\n"));
  m_textEdit->setPlainText(s);
}

void ParaFieldWidget::clearImpl() {
  m_textEdit->clear();
  editMultiple(false);
}

QWidget* ParaFieldWidget::widget() {
  return m_textEdit;
}

