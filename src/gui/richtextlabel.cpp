/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "richtextlabel.h"

#include <kdebug.h>

#include <qlayout.h>

using Tellico::GUI::RichTextLabel;

RichTextLabel::RichTextLabel(QWidget* parent) : QTextEdit(parent) {
  init();
};

RichTextLabel::RichTextLabel(const QString& text, QWidget* parent) : QTextEdit(text, QString::null, parent) {
  init();
};

QSize RichTextLabel::sizeHint() const {
  return minimumSizeHint();
}

void RichTextLabel::init() {
  setReadOnly(true);
  setTextFormat(Qt::RichText);

  setFrameShape(QFrame::NoFrame);
  viewport()->setMouseTracking(false);

  setPaper(colorGroup().background());

  setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));
  viewport()->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));
}

#include "richtextlabel.moc"
