/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef FETCHCONFIGWIDGET_H
#define FETCHCONFIGWIDGET_H

class KConfig;

#include "fetch.h"

#include <qwidget.h>

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 * @version $Id: configdialog.h 911 2004-10-06 00:53:40Z robby $
 */
class ConfigWidget : public QWidget {
Q_OBJECT

public:
  ConfigWidget(QWidget* parent_) : QWidget(parent_), m_modified(false), m_accepted(false) {}
  virtual ~ConfigWidget() {}

  void setAccepted(bool accepted_) { m_accepted = accepted_; }
  bool shouldSave() const { return m_modified && m_accepted; }
  /**
   * Read any configuration options. The config group must be
   * set before calling this function.
   *
   * @param config_ The KConfig pointer
   */
//  virtual void readConfig(KConfig* config_) = 0;
  /**
   * Saves any configuration options. The config group must be
   * set before calling this function.
   *
   * @param config_ The KConfig pointer
   */
  virtual void saveConfig(KConfig* config_) = 0;

public slots:
  void slotSetModified(bool modified_ = true) { m_modified = modified_; }

private:
  bool m_modified;
  bool m_accepted;
};

  }
}

#endif
