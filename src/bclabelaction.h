/***************************************************************************
                              bclabelaction.h
                             -------------------
    begin                : Sat Nov 9 2002
    copyright            : (C) 2002 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BCLABELACTION_H
#define BCLABELACTION_H

#include <kaction.h>

/**
 * KDE 3.0.x didn't include an easy way to insert a label using the XML-GUI.
 * BCLabelAction is pretty much a copy of the old KonqLabelAction from KDE 3.0.x.
 * It should be superceded by @ref KWidgetAction in KDE 3.1.
 *
 * @see KWidgetAction
 *
 * @author Robby Stephenson
 * @version $Id: bclabelaction.h,v 1.3 2002/11/11 03:47:06 robby Exp $
 */
class BCLabelAction : public KAction {
Q_OBJECT

public:
  BCLabelAction(const QString& text, int accel,
                QObject* parent = 0, const char* name = 0);
  
  int plug(QWidget* widget, int index = -1);
  void unplug(QWidget* widget);
  
private:
  class ToolBarLabel;
  ToolBarLabel* m_label;
};

#endif
