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
#include "core/tellico_config.h"

#include <KIcon>
#include <KLocale>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QStackedWidget>
#include <QSlider>

using Tellico::ViewStack;

ViewStack::ViewStack(QWidget* parent_) : QWidget(parent_),
                     m_listView(new DetailedListView(this)), m_iconView(new EntryIconView(this)) {
  QBoxLayout* lay = new QVBoxLayout();
  lay->setMargin(0);
  lay->setSpacing(0);

  QBoxLayout* hlay = new QHBoxLayout();
  lay->addLayout(hlay);
  hlay->setMargin(0);
  hlay->setSpacing(0);

  QToolButton* listBtn = new QToolButton(this);
  listBtn->setIcon(KIcon(QLatin1String("view-list-details")));
  connect(listBtn, SIGNAL(clicked(bool)), SLOT(showListView()));

  QToolButton* iconBtn = new QToolButton(this);
  iconBtn->setIcon(KIcon(QLatin1String("view-list-icons")));
  connect(iconBtn, SIGNAL(clicked(bool)), SLOT(showIconView()));

  hlay->addWidget(listBtn);
  hlay->addWidget(iconBtn);
  hlay->addStretch(10);

  m_decreaseIconSizeButton = new QToolButton(this);
  m_decreaseIconSizeButton->setIcon(KIcon(QLatin1String("zoom-out")));
  m_decreaseIconSizeButton->setToolTip(i18n("Decrease the maximum icon size in the icon list view"));
  hlay->addWidget(m_decreaseIconSizeButton);
  connect(m_decreaseIconSizeButton, SIGNAL(clicked(bool)), SLOT(slotDecreaseIconSizeButtonClicked()));

  m_iconSizeSlider = new QSlider(Qt::Horizontal, this);
  m_iconSizeSlider->setMinimum(MIN_ENTRY_ICON_SIZE);
  m_iconSizeSlider->setMaximum(MAX_ENTRY_ICON_SIZE);
  m_iconSizeSlider->setSingleStep(SMALL_INCREMENT_ICON_SIZE);
  m_iconSizeSlider->setPageStep(LARGE_INCREMENT_ICON_SIZE);
  m_iconSizeSlider->setValue(Config::maxIconSize());
  m_iconSizeSlider->setTracking(true);
  m_iconSizeSlider->setToolTip(i18n("The current maximum icon size is %1.\nMove the slider to change it.").arg(Config::maxIconSize()));
  hlay->addWidget(m_iconSizeSlider);
  connect(m_iconSizeSlider, SIGNAL(valueChanged(int)), SLOT(slotIconSizeSliderChanged(int)));

  m_increaseIconSizeButton = new QToolButton(this);
  m_increaseIconSizeButton->setIcon(KIcon(QLatin1String("zoom-in")));
  m_increaseIconSizeButton->setToolTip(i18n("Increase the maximum icon size in the icon list view"));
  hlay->addWidget(m_increaseIconSizeButton, 0);
  connect(m_increaseIconSizeButton, SIGNAL(clicked(bool)), SLOT(slotIncreaseIconSizeButtonClicked()));

  setIconSizeInterfaceVisible(false);

  m_stack = new QStackedWidget(this);
  lay->addWidget(m_stack);
  m_stack->addWidget(m_listView);
  m_stack->addWidget(m_iconView);

  setLayout(lay);
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
  m_iconSizeSlider->setToolTip(i18n("The current maximum icon size is %1.\nMove the slider to change it.").arg(size));
  Config::setMaxIconSize(size);
  m_iconView->setMaxAllowedIconWidth(size);
}

void ViewStack::setIconSizeInterfaceVisible(bool visible) {
  m_decreaseIconSizeButton->setVisible(visible);
  m_increaseIconSizeButton->setVisible(visible);
  m_iconSizeSlider->setVisible(visible);
}

#include "viewstack.moc"
