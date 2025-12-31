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

using Tellico::GUI::ParaFieldWidget;

ParaFieldWidget::ParaFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_) {

  m_textEdit = new KTextEdit(this);
  m_textEdit->setAcceptRichText(false);
  if(field_->property(QStringLiteral("spellcheck")) != QLatin1String("false")) {
    m_textEdit->setCheckSpellingEnabled(true);
  }
  // the default is to replace line feeds with HTML tags
  m_replaceLineFeeds = field_->property(QStringLiteral("replace-line-feeds")) != QLatin1String("false");

  void (KTextEdit::* textChanged)() = &KTextEdit::textChanged;
  connect(m_textEdit, textChanged, this, &ParaFieldWidget::checkModified);

  m_brRx = QRegularExpression(QStringLiteral("<br/?>"), QRegularExpression::CaseInsensitiveOption);

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
  if(m_replaceLineFeeds) {
    text.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
  }
  return text;
}

void ParaFieldWidget::setTextImpl(const QString& text_) {
  QString s = text_;
  if(m_replaceLineFeeds) {
    s.replace(m_brRx, QStringLiteral("\n"));
  }
  m_textEdit->setPlainText(s);
}

void ParaFieldWidget::clearImpl() {
  m_textEdit->clear();
  editMultiple(false);
}

QWidget* ParaFieldWidget::widget() {
  return m_textEdit;
}

void ParaFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  m_textEdit->setCheckSpellingEnabled(newField_->property(QStringLiteral("spellcheck")) != QLatin1String("false"));
  m_replaceLineFeeds = newField_->property(QStringLiteral("replace-line-feeds")) != QLatin1String("false");
}
