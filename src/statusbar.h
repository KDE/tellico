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

// much of this code is adapted from amarok
// which is GPL licensed, Copyright (C) 2005 by Max Howell

#ifndef TELLICO_STATUSBAR_H
#define TELLICO_STATUSBAR_H

#include <kstatusbar.h>

class QLabel;
class KPushButton;

namespace Tellico {
  namespace GUI {
    class Progress;
  }
  class MainWindow;

/**
 * @author Robby Stephenson
 */
class StatusBar : public KStatusBar {
Q_OBJECT

public:
  void clearStatus();
  void setStatus(const QString& status);
  void setCount(const QString& count);

  static StatusBar* self() { return s_self; }

  virtual void ensurePolished() const;

private slots:
  void slotProgress(qulonglong progress);
  void slotUpdate();

private:
  static StatusBar* s_self;

  friend class MainWindow;

  StatusBar(QWidget* parent);

  QLabel* m_mainLabel;
  QLabel* m_countLabel;
  GUI::Progress* m_progress;
  KPushButton* m_cancelButton;
};

}

#endif
