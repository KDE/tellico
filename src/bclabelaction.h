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
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCLABELACTION_H
#define BCLABELACTION_H

#include <kaction.h>

/**
 * There isn't an easy way to insert a label using the XML-GUI in KDE 3.0.x.
 * BCLabelAction is pretty much a copy of the KonqLabelAction class from KDE 3.0.x.
 * It is superceded by @ref KWidgetAction in KDE 3.1.
 *
 * @see KWidgetAction
 *
 * @author Robby Stephenson
 * @version $Id: bclabelaction.h,v 1.5 2003/03/08 18:24:47 robby Exp $
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
