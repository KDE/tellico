/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_GUI_PROGRESS_H
#define TELLICO_GUI_PROGRESS_H

#include <QProgressBar>

namespace Tellico {
  namespace GUI {

/**
 * @author Robby Stephenson
 */
class Progress : public QProgressBar {
Q_OBJECT

public:
  Progress(QWidget* parent);
  Progress(int totalSteps, QWidget* parent);

  bool isDone() const;
  void setDone();
};

  } // end namespace
} // end namespace

#endif
