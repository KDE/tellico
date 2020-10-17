/***************************************************************************
    Copyright (C) 2005-2020 Robby Stephenson <robby@periapsis.org>
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

#include <KTextEdit>

#include <QRegularExpression>

using Tellico::GUI::ParaFieldWidget;

ParaFieldWidget::ParaFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_textEdit = new KTextEdit(this);
  m_textEdit->setAcceptRichText(false);
  if(field_->property(QStringLiteral("spellcheck")) != QLatin1String("false")) {
    m_textEdit->setCheckSpellingEnabled(true);
  }
  void (KTextEdit::* textChanged)() = &KTextEdit::textChanged;
  connect(m_textEdit, textChanged, this, &ParaFieldWidget::checkModified);

  registerWidget();
}

ParaFieldWidget::~ParaFieldWidget() {
  // saw a crash when closing Tellico and the KTextEdit d'tor called ~QSyntaxHighlighter()
  // when ultimately signaled a valueChange for some reason, so disconnect
  void (KTextEdit::* textChanged)() = &KTextEdit::textChanged;
  disconnect(m_textEdit, textChanged, this, &ParaFieldWidget::checkModified);
}

QString ParaFieldWidget::text() const {
  QString text = m_textEdit->toPlainText();
  text.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
  return text;
}

void ParaFieldWidget::setTextImpl(const QString& text_) {
  QRegularExpression rx(QLatin1String("<br/?>"), QRegularExpression::CaseInsensitiveOption);
  QString s = text_;
  s.replace(rx, QStringLiteral("\n"));
  m_textEdit->setPlainText(s);
}

void ParaFieldWidget::clearImpl() {
  m_textEdit->clear();
  editMultiple(false);
}

QWidget* ParaFieldWidget::widget() {
  return m_textEdit;
}
