/* *************************************************************************
                             searchdialog.cpp
                             -------------------
    begin                : Wed Feb 27 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@periapsis.org
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#include "searchdialog.h"

#include <klocale.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qlabel.h>

SearchDialog::SearchDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(Plain, i18n("Find"), User1|Cancel, User1,
                  parent_, name_, false, false, i18n("&Find")) {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
  QVBoxLayout* topLayout = new QVBoxLayout(page, 0, spacingHint());

  QLabel* label = new QLabel(i18n("Find:"), page);
  topLayout->addWidget(label);

//  lineedit = new QLineEdit(urltext, page, "lineedit");
//  lineedit->setMinimumWidth(fontMetrics().maxWidth()*20);
//  topLayout->addWidget(lineedit);

  topLayout->addStretch(1);
}

void SearchDialog::slotUser1() {
}
