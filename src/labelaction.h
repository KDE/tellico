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

#ifndef LABELACTION_H
#define LABELACTION_H

#include <kaction.h>
#include <klineedit.h>

#include <qguardedptr.h>

namespace Bookcase {

/**
 * There isn't an easy way to insert a label using the XML-GUI in KDE 3.0.x.
 * LabelAction is pretty much a copy of the KonqLabelAction class from KDE 3.0.x.
 * It is superceded by @ref KWidgetAction in KDE 3.1.
 *
 * @see KWidgetAction
 *
 * @author Robby Stephenson
 * @version $Id: labelaction.h 626 2004-04-28 03:54:00Z robby $
 */
class LabelAction : public KWidgetAction {
Q_OBJECT

public:
  LabelAction(const QString& text, int accel,
                KActionCollection* parent = 0, const char* name = 0);
};

/**
 * There isn't an easy way to insert a line edit using the XML-GUI in KDE 3.0.x.
 *
 * @see KAction
 *
 * @author Robby Stephenson
 * @version $Id: labelaction.h 626 2004-04-28 03:54:00Z robby $
 */
class LineEditAction : public KAction {
Q_OBJECT

public:
  LineEditAction(const QString& text, int accel, KActionCollection* parent = 0, const char* name = 0);

  virtual int plug(QWidget* w, int index = -1);
  virtual void unplug(QWidget* w);

  QString text() const { return m_lineEdit ? m_lineEdit->text() : QString::null; }

public slots:
  void clear();

signals:
  void plugged();
  void textChanged(const QString& string);

private:
  QGuardedPtr<KLineEdit> m_lineEdit;
};

} // end namespace
#endif
