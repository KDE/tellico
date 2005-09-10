/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "parafieldwidget.h"
#include "../field.h"

#include <qtextedit.h>

using Tellico::GUI::ParaFieldWidget;

ParaFieldWidget::ParaFieldWidget(const Data::Field* field_, QWidget* parent_, const char* name_/*=0*/)
    : FieldWidget(field_, parent_, name_) {

  m_textEdit = new QTextEdit(this);
  m_textEdit->setTextFormat(Qt::PlainText);
  connect(m_textEdit, SIGNAL(textChanged()), SIGNAL(modified()));

  registerWidget();
}

QString ParaFieldWidget::text() const {
  QString text = m_textEdit->text();
  text.replace('\n', QString::fromLatin1("<br/>"));
  return text;
}

void ParaFieldWidget::setText(const QString& text_) {
  blockSignals(true);
  m_textEdit->blockSignals(true);

  QRegExp rx(QString::fromLatin1("<br/?>"), false /*case-sensitive*/);
  QString s = text_;
  s.replace(rx, QChar('\n'));
  m_textEdit->setText(s);

  m_textEdit->blockSignals(false);
  blockSignals(false);
}

void ParaFieldWidget::clear() {
  m_textEdit->clear();
  editMultiple(false);
}

QWidget* ParaFieldWidget::widget() {
  return m_textEdit;
}

#include "parafieldwidget.moc"
