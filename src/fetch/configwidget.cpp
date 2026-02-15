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

#include "configwidget.h"

#include <KLocalizedString>
#include <KAcceleratorManager>
#include <KConfigGroup>

#include <QGroupBox>
#include <QHBoxLayout>

using Tellico::Fetch::ConfigWidget;

ConfigWidget::ConfigWidget(QWidget* parent_) : QWidget(parent_), m_modified(false), m_accepted(false) {
  QHBoxLayout* boxLayout = new QHBoxLayout(this);
  boxLayout->setSpacing(10);
  boxLayout->setContentsMargins(0, 0, 0, 0);

  QGroupBox* gvbox = new QGroupBox(i18n("Source Options"), this);
  boxLayout->addWidget(gvbox, 10 /*stretch*/);

  QVBoxLayout* vbox = new QVBoxLayout(gvbox);
  m_optionsWidget = new QWidget(gvbox);
  vbox->addWidget(m_optionsWidget);
  vbox->addStretch(1);
}

ConfigWidget::~ConfigWidget() = default;

bool ConfigWidget::shouldSave() const {
  return m_modified && m_accepted;
}

void ConfigWidget::setAccepted(bool accepted_) {
  m_accepted = accepted_;
}

void ConfigWidget::slotSetModified() {
  m_modified = true;
}

void ConfigWidget::addFieldsWidget(const Tellico::StringHash& customFields_, const QStringList& fieldsToAdd_) {
  if(customFields_.isEmpty()) {
    return;
  }

  QGroupBox* gbox = new QGroupBox(i18n("Available Fields"), this);
  static_cast<QBoxLayout*>(layout())->addWidget(gbox);

  QVBoxLayout* vbox = new QVBoxLayout(gbox);
  for(StringHash::ConstIterator it = customFields_.begin(); it != customFields_.end(); ++it) {
    QCheckBox* cb = new QCheckBox(it.value(), gbox);
    m_fields.insert(it.key(), cb);
    if(fieldsToAdd_.contains(it.key())) {
      cb->setChecked(true);
    }
    connect(cb, &QAbstractButton::clicked, this, &ConfigWidget::slotSetModified);
    vbox->addWidget(cb);
  }
  vbox->addStretch(1);

  KAcceleratorManager::manage(this);
}

void ConfigWidget::saveConfig(KConfigGroup& config_) {
  QStringList fields;
  QHash<QString, QCheckBox*>::const_iterator it = m_fields.constBegin();
  for( ; it != m_fields.constEnd(); ++it) {
    if(it.value()->isChecked()) {
      fields << it.key();
    }
  }
  config_.writeEntry(QStringLiteral("Custom Fields"), fields);
  saveConfigHook(config_);
  m_modified = false;
}

QWidget* ConfigWidget::optionsWidget() {
  return m_optionsWidget;
}
