/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "progress.h"
#include "../tellico_debug.h"

using Tellico::GUI::Progress;

Progress::Progress(QWidget* parent_) : KProgress(parent_) {
}

Progress::Progress(int totalSteps_, QWidget* parent_) : KProgress(totalSteps_, parent_) {
}

bool Progress::isDone() const {
  return progress() == totalSteps();
}

void Progress::setDone() {
  setProgress(totalSteps());
}

#include "progress.moc"
