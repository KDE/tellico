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

class KLineEdit;

#include <kaction.h>

#include <qguardedptr.h>

/**
 * There isn't an easy way to insert a label using the XML-GUI in KDE 3.0.x.
 * BCLabelAction is pretty much a copy of the KonqLabelAction class from KDE 3.0.x.
 * It is superceded by @ref KWidgetAction in KDE 3.1.
 *
 * @see KWidgetAction
 *
 * @author Robby Stephenson
 * @version $Id: bclabelaction.h,v 1.4 2003/05/03 05:54:43 robby Exp $
 */
class BCLabelAction : public KAction {
Q_OBJECT

public:
  BCLabelAction(const QString& text, int accel,
                QObject* parent = 0, const char* name = 0);
  
  virtual int plug(QWidget* widget, int index = -1);
  virtual void unplug(QWidget* widget);
  
private:
  class ToolBarLabel;
  ToolBarLabel* m_label;
};

/**
 * There isn't an easy way to insert a line edit using the XML-GUI in KDE 3.0.x.
 * BCLabelAction is pretty much a modified copy of the KonqComboAction class from KDE 3.0.x.
 * It is superceded by @ref KWidgetAction in KDE 3.1.
 *
 * @see KWidgetAction
 *
 * @author Robby Stephenson
 * @version $Id: bclabelaction.h,v 1.4 2003/05/03 05:54:43 robby Exp $
 */
class BCLineEditAction : public KAction {
Q_OBJECT

public:
  BCLineEditAction(const QString& text, int accel,
                   QObject* parent = 0, const char* name = 0);

  virtual int plug(QWidget* w, int index = -1);
  virtual void unplug(QWidget* w);

public slots:
  void clear();

signals:
  void plugged();
  void textChanged(const QString& string);

private:
  QGuardedPtr<KLineEdit> m_lineEdit;
};

#endif
