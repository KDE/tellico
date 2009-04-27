/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_FETCHERCONFIGDIALOG_H
#define TELLICO_FETCHERCONFIGDIALOG_H

#include "fetch/fetch.h"
#include "fetch/configwidget.h"

#include <kdialog.h>

#include <QHash>
#include <QLabel>

class KLineEdit;
class QCheckBox;
class QStackedWidget;

namespace Tellico {
  namespace GUI {
    class ComboBox;
  }

/**
 * @author Robby Stephenson
 */
class FetcherConfigDialog : public KDialog {
Q_OBJECT

public:
  FetcherConfigDialog(QWidget* parent);
  FetcherConfigDialog(const QString& sourceName, Fetch::Type type, bool updateOverwrite,
                      Fetch::ConfigWidget* configWidget, QWidget* parent);
  virtual ~FetcherConfigDialog() {}

  QString sourceName() const;
  Fetch::Type sourceType() const;
  bool updateOverwrite() const;
  Fetch::ConfigWidget* configWidget() const;

private slots:
  void slotNewSourceSelected(int idx);
  void slotNameChanged(const QString& name);
  void slotPossibleNewName(const QString& name);

private:
  void init(Fetch::Type type);

  bool m_newSource : 1;
  bool m_useDefaultName : 1;
  Fetch::ConfigWidget* m_configWidget;
  QLabel* m_iconLabel;
  KLineEdit* m_nameEdit;
  GUI::ComboBox* m_typeCombo;
  QCheckBox* m_cbOverwrite;
  QStackedWidget* m_stack;
  QHash<int, Fetch::ConfigWidget*> m_configWidgets;
};

} // end namespace
#endif
