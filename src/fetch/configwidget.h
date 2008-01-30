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

#include "../datavectors.h"

#include <qwidget.h>
#include <qdict.h>
#include <qcheckbox.h>

class KConfigGroup;
class QStringList;

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
  virtual void saveConfig(KConfigGroup& config) = 0;
  /**
   * Called when a fetcher data source is removed. Useful for any cleanup work necessary.
   * The ExecExternalFetcher might need to remove the script, for example.
   * Because of the way the ConfigDialog is setup, easier to have that in the ConfigWidget
   * class than in the Fetcher class itself
   */
  virtual void removed() {}
  virtual QString preferredName() const = 0;

signals:
  void signalName(const QString& name);

public slots:
  void slotSetModified(bool modified_ = true) { m_modified = modified_; }

protected:
  QWidget* optionsWidget() { return m_optionsWidget; }
  void addFieldsWidget(const StringMap& customFields, const QStringList& fieldsToAdd);
  void saveFieldsConfig(KConfigGroup& config) const;

private:
  bool m_modified;
  bool m_accepted;
  QWidget* m_optionsWidget;
  QDict<QCheckBox> m_fields;
};

  }
}

#endif
