/***************************************************************************
                              bclabelaction.h
                             -------------------
    begin                : Sat Nov 9 2002
    copyright            : (C) 2002, 2003 by Robby Stephenson
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
#include <kdeversion.h>

#include <qguardedptr.h>

// Do some trickery to fool moc. If compiling for KDE 3.0.x, BCLabelAction inherits KAction.
// Otherwise, it inherits KWidgetAction
#if KDE_VERSION > 309
typedef KWidgetAction ActionClass;
#else
typedef KAction ActionClass;
#endif

/**
 * There isn't an easy way to insert a label using the XML-GUI in KDE 3.0.x.
 * BCLabelAction is pretty much a copy of the KonqLabelAction class from KDE 3.0.x.
 * It is superceded by @ref KWidgetAction in KDE 3.1.
 *
 * @see KWidgetAction
 *
 * @author Robby Stephenson
 * @version $Id: bclabelaction.h 206 2003-10-22 00:11:36Z robby $
 */
class BCLabelAction : public ActionClass {
Q_OBJECT

public:
  BCLabelAction(const QString& text, int accel,
                KActionCollection* parent = 0, const char* name = 0);
  
#if KDE_VERSION > 309
#else
  virtual int plug(QWidget* widget, int index = -1);
  virtual void unplug(QWidget* widget);
  
private:
  class ToolBarLabel;
  ToolBarLabel* m_label;
#endif
};

/**
 * There isn't an easy way to insert a line edit using the XML-GUI in KDE 3.0.x.
 * BCLabelAction is pretty much a modified copy of the KonqComboAction class from KDE 3.0.x.
 * It is superceded by @ref KWidgetAction in KDE 3.1.
 *
 * @see KWidgetAction
 *
 * @author Robby Stephenson
 * @version $Id: bclabelaction.h 62 2003-07-26 00:49:23Z robby /home/robby/projects/bookcase/src/bccollectionpropdialog.$
 */
class BCLineEditAction : public KAction {
Q_OBJECT

public:
  BCLineEditAction(const QString& text, int accel,
                   KActionCollection* parent = 0, const char* name = 0);

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
