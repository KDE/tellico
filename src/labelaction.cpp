/***************************************************************************
    copyright            : (C) 2002-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "labelaction.h"

#include <ktoolbar.h>
#include <klineedit.h>

using Bookcase::LabelAction;
using Bookcase::LineEditAction;

#include <qlabel.h>
// KWidgetAction(QWidget *widget,
//               const QString &text,
//               const KShortcut &cut,
//               const QObject *receiver,
//               const char *slot,
//               KActionCollection *parent
//               const char *name)
LabelAction::LabelAction(const QString& text_, int accel_,
                         KActionCollection* parent_, const char* name_/*=0*/)
    : KWidgetAction(new QLabel(text_, 0, "kde toolbar widget"),
                    text_,
                    accel_,
                    0,
                    0,
                    parent_,
                    name_) {
  QLabel* label = static_cast<QLabel*>(widget());
  // asthetically, I want a space in front of and after the text
  label->setText(QString::fromLatin1(" ") + label->text() + QString::fromLatin1(" "));
  label->setBackgroundMode(Qt::PaletteButton);
  label->adjustSize();
}

// largely copied from KonqComboAction
LineEditAction::LineEditAction(const QString& text_, int accel_,
                               KActionCollection* parent_/*=0*/, const char* name_/*=0*/)
 : KAction(text_, accel_, parent_, name_), m_lineEdit(0L) {
}

int LineEditAction::plug(QWidget* widget_, int index_) {
  KToolBar* tb = static_cast<KToolBar *>(widget_);

  int id = KAction::getToolButtonID();

  m_lineEdit = new KLineEdit(tb);
  m_lineEdit->setFixedWidth(m_lineEdit->fontMetrics().maxWidth()*5);

  tb->insertWidget(id, m_lineEdit->width(), m_lineEdit, index_);

  connect(m_lineEdit, SIGNAL(textChanged(const QString&)),
          this, SIGNAL(textChanged(const QString&)));

  addContainer(tb, id);

  connect(tb, SIGNAL(destroyed()), this, SLOT(slotDestroyed()));

  emit plugged();
  return containerCount() - 1;
}

void LineEditAction::unplug(QWidget* widget_) {
  KToolBar* tb = static_cast<KToolBar *>(widget_);

  int idx = findContainer(widget_);

  if(idx != -1) {
    tb->removeItem(itemId(idx));
    removeContainer(idx);
  }

  m_lineEdit = 0L;
}

void LineEditAction::clear() {
  m_lineEdit->clear();
}
