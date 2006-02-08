/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
class QCheckBox;

#include "fetch.h"
#include "../datavectors.h"

#include <qwidget.h>
#include <qdict.h>
#include <qcheckbox.h>

namespace Tellico {
  namespace Fetch {

/**
 * @author Robby Stephenson
 */
class ConfigWidget : public QWidget {
Q_OBJECT

public:
  ConfigWidget(QWidget* parent);
  virtual ~ConfigWidget() {}

  void setAccepted(bool accepted_) { m_accepted = accepted_; }
  bool shouldSave() const { return m_modified && m_accepted; }
  /**
   * Saves any configuration options. The config group must be
   * set before calling this function.
   *
   * @param config_ The KConfig pointer
   */
  virtual void saveConfig(KConfig* config) = 0;

public slots:
  void slotSetModified(bool modified_ = true) { m_modified = modified_; }

protected:
  QWidget* optionsWidget() { return m_optionsWidget; }
  void addFieldsWidget(const StringMap& customFields, const QStringList& fieldsToAdd);
  void saveFieldsConfig(KConfig* config) const;

private:
  bool m_modified;
  bool m_accepted;
  QWidget* m_optionsWidget;
  QDict<QCheckBox> m_fields;
};

  }
}

#endif
