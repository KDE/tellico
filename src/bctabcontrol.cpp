/* *************************************************************************
                             bctabcontrol.cpp
                             -------------------
    begin                : Sun Jan 6 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#include "bctabcontrol.h"

BCTabControl::BCTabControl(QWidget* parent_, const char* name_/*=0*/)
 : KTabCtl(parent_, name_) {
}

BCTabControl::~BCTabControl() {
}

void BCTabControl::showTab(int i) {
  KTabCtl::showTab(i);
}
