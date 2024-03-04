/***************************************************************************
    Copyright (C) 2002-2011 Robby Stephenson <robby@periapsis.org>
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

#include "viewstack.h"
#include "detailedlistview.h"
#include "entryiconview.h"
#include "config/tellico_config.h"

#include <KLocalizedString>

#include <QIcon>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedWidget>
#include <QSlider>
#include <QButtonGroup>

namespace {
  static const int MIN_ENTRY_ICON_SIZE = 64;
  static const int MAX_ENTRY_ICON_SIZE = 512;
  static const int SMALL_INCREMENT_ICON_SIZE = 1;
  static const int LARGE_INCREMENT_ICON_SIZE = 8;
}

using Tellico::ViewStack;

ViewStack::ViewStack(QWidget* parent_) : QWidget(parent_)
    , m_listView(new DetailedListView(this))
    , m_iconView(new EntryIconView(this)) {
  QBoxLayout* lay = new QVBoxLayout();
  lay->setContentsMargins(0, 0, 0, 0);
  lay->setSpacing(0);

  QBoxLayout* hlay = new QHBoxLayout();
  lay->addLayout(hlay);
  hlay->setContentsMargins(0, 0, 0, 0);
  hlay->setSpacing(0);

  m_listButton = new QToolButton(this);
  m_listButton->setCheckable(true);
  m_listButton->setIcon(QIcon::fromTheme(QStringLiteral("view-list-details")));
  connect(m_listButton, &QAbstractButton::clicked, this, &ViewStack::showListView);

  m_iconButton = new QToolButton(this);
  m_iconButton->setCheckable(true);
  m_iconButton->setIcon(QIcon::fromTheme(QStringLiteral("view-list-icons")));
  connect(m_iconButton, &QAbstractButton::clicked, this, &ViewStack::showIconView);

  QButtonGroup* bg = new QButtonGroup(this);
  bg->addButton(m_listButton);
  bg->addButton(m_iconButton);

  hlay->addWidget(m_listButton);
  hlay->addWidget(m_iconButton);
  hlay->addStretch(10);

  m_decreaseIconSizeButton = new QToolButton(this);
  m_decreaseIconSizeButton->setIcon(QIcon::fromTheme(QStringLiteral("zoom-out")));
  m_decreaseIconSizeButton->setToolTip(i18n("Decrease the maximum icon size in the icon list view"));
  hlay->addWidget(m_decreaseIconSizeButton);
  connect(m_decreaseIconSizeButton, &QAbstractButton::clicked, this, &ViewStack::slotDecreaseIconSizeButtonClicked);

  m_iconSizeSlider = new QSlider(Qt::Horizontal, this);
  m_iconSizeSlider->setMinimum(MIN_ENTRY_ICON_SIZE);
  m_iconSizeSlider->setMaximum(MAX_ENTRY_ICON_SIZE);
  m_iconSizeSlider->setSingleStep(SMALL_INCREMENT_ICON_SIZE);
  m_iconSizeSlider->setPageStep(LARGE_INCREMENT_ICON_SIZE);
  m_iconSizeSlider->setValue(Config::maxIconSize());
  m_iconSizeSlider->setTracking(true);
  m_iconSizeSlider->setToolTip(i18n("The current maximum icon size is %1.\nMove the slider to change it.", Config::maxIconSize()));
  hlay->addWidget(m_iconSizeSlider);
  connect(m_iconSizeSlider, &QAbstractSlider::valueChanged, this, &ViewStack::slotIconSizeSliderChanged);

  m_increaseIconSizeButton = new QToolButton(this);
  m_increaseIconSizeButton->setIcon(QIcon::fromTheme(QStringLiteral("zoom-in")));
  m_increaseIconSizeButton->setToolTip(i18n("Increase the maximum icon size in the icon list view"));
  hlay->addWidget(m_increaseIconSizeButton, 0);
  connect(m_increaseIconSizeButton, &QAbstractButton::clicked, this, &ViewStack::slotIncreaseIconSizeButtonClicked);

  setIconSizeInterfaceVisible(false);

  m_stack = new QStackedWidget(this);
  lay->addWidget(m_stack);
  m_stack->addWidget(m_listView);
  m_stack->addWidget(m_iconView);

  setLayout(lay);
}

int ViewStack::currentWidget() const {
  if(m_stack->currentWidget() == m_listView) {
    return Config::ListView;
  } else {
    return Config::IconView;
  }
}

void ViewStack::setCurrentWidget(int widget_) {
  switch(widget_) {
    case Config::ListView:
      m_listButton->setChecked(true);
      showListView();
      break;
    case Config::IconView:
      m_iconButton->setChecked(true);
      showIconView();
      break;
  }
}

void ViewStack::showListView() {
  setIconSizeInterfaceVisible(false);
  m_stack->setCurrentWidget(m_listView);
}

void ViewStack::showIconView() {
  m_stack->setCurrentWidget(m_iconView);
  setIconSizeInterfaceVisible(true);
}

void ViewStack::slotDecreaseIconSizeButtonClicked() {
  m_iconSizeSlider->setValue(m_iconSizeSlider->value() - LARGE_INCREMENT_ICON_SIZE);
}

void ViewStack::slotIncreaseIconSizeButtonClicked() {
  m_iconSizeSlider->setValue(m_iconSizeSlider->value() + LARGE_INCREMENT_ICON_SIZE);
}

void ViewStack::slotIconSizeSliderChanged(int size) {
  m_decreaseIconSizeButton->setEnabled(size > MIN_ENTRY_ICON_SIZE);
  m_increaseIconSizeButton->setEnabled(size < MAX_ENTRY_ICON_SIZE);
  m_iconSizeSlider->setToolTip(i18n("The current maximum icon size is %1.\nMove the slider to change it.", size));
  Config::setMaxIconSize(size);
  m_iconView->setMaxAllowedIconWidth(size);
}

void ViewStack::setIconSizeInterfaceVisible(bool visible) {
  m_decreaseIconSizeButton->setVisible(visible);
  m_increaseIconSizeButton->setVisible(visible);
  m_iconSizeSlider->setVisible(visible);
}
