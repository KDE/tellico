/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_FETCHCONFIGWIDGET_H
#define TELLICO_FETCHCONFIGWIDGET_H

#include "../datavectors.h"

#include <QWidget>
#include <QCheckBox>
#include <QHash>

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
  QHash<QString, QCheckBox*> m_fields;
};

  }
}

#endif
