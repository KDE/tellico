/***************************************************************************
    Copyright (C) 2002-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_VIEWSTACK_H
#define TELLICO_VIEWSTACK_H

#include "datavectors.h"

#include <QWidget>

class QStackedWidget;
class QSlider;
class QToolButton;
class QxtFlowView;

namespace Tellico {
  class DetailedListView;
  class EntryIconView;

/**
 * @author Robby Stephenson
 */
class ViewStack : public QWidget {
Q_OBJECT

public:
  ViewStack(QWidget* parent);

  DetailedListView* listView() { return m_listView; }
  EntryIconView* iconView() { return m_iconView; }
  QxtFlowView* flowView() { return m_flowView; }

  int currentWidget() const;
  void setCurrentWidget(int widget);

private Q_SLOTS:
  void showListView();
  void showIconView();
  void showFlowView();
  /**
    * Called when the "Decrease Icon Size" button is clicked.
    */
  void slotDecreaseIconSizeButtonClicked();
  /**
    * Called when the "Increase Icon Size" button is clicked.
    */
  void slotIncreaseIconSizeButtonClicked();
  /**
    * Called when the "Icon Size" slider value changes.
    */
  void slotIconSizeSliderChanged(int);

private:
  /**
   * Sets the visibility of the icon size GUI controls
   */
  void setIconSizeInterfaceVisible(bool);

  DetailedListView* m_listView;
  EntryIconView* m_iconView;
  QxtFlowView* m_flowView;
  QStackedWidget* m_stack;
  QToolButton* m_listButton;
  QToolButton* m_iconButton;
  QToolButton* m_flowButton;
  QSlider* m_iconSizeSlider;
  QToolButton* m_increaseIconSizeButton;
  QToolButton* m_decreaseIconSizeButton;
};

} // end namespace
#endif
