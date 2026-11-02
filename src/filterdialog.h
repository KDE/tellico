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

#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

#include "datavectors.h"

#include <QDialog>
#include <QString>
#include <QStringList>

class QRadioButton;
class QPushButton;
class QLineEdit;

namespace Tellico {
  class FilterRuleWidgetLister;

/**
 * @author Robby Stephenson
 */
class FilterDialog : public QDialog {
Q_OBJECT

public:
  enum Mode {
    CreateFilter,
    ModifyFilter
  };

  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   */
  FilterDialog(Mode mode, QWidget* parent);

  Filter currentFilter(bool alwaysCreateNew=false);
  void setFilter(const Filter& filter);

public Q_SLOTS:
  void slotClear();

protected Q_SLOTS:
  void slotOk();
  void slotApply();
  void slotHelp();
  void slotShrink();
  void slotFilterChanged();
  void slotSaveFilter();

Q_SIGNALS:
  void signalUpdateFilter(const Tellico::Filter&);
  void signalCollectionModified();

private:
  Filter m_filter;
  const Mode m_mode;
  QRadioButton* m_matchAll;
  QRadioButton* m_matchAny;
  FilterRuleWidgetLister* m_ruleLister;
  QLineEdit* m_filterName;
  QPushButton* m_saveFilter;
  QPushButton* m_okButton;
  QPushButton* m_applyButton;
};

} // end namespace
#endif
