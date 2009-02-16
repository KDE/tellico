/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "configwidget.h"

#include <klocale.h>
#include <kacceleratormanager.h>
#include <KConfigGroup>

#include <QGroupBox>
#include <QHBoxLayout>

using Tellico::Fetch::ConfigWidget;

ConfigWidget::ConfigWidget(QWidget* parent_) : QWidget(parent_), m_modified(false), m_accepted(false) {
  QHBoxLayout* boxLayout = new QHBoxLayout(this);
  boxLayout->setSpacing(10);
  boxLayout->setMargin(0);

  QGroupBox* gvbox = new QGroupBox(i18n("Source Options"), this);
  boxLayout->addWidget(gvbox, 10 /*stretch*/);

  QVBoxLayout* vbox = new QVBoxLayout();
  m_optionsWidget = new QWidget(gvbox);
  vbox->addWidget(m_optionsWidget);
  vbox->addStretch(1);
  gvbox->setLayout(vbox);
}

void ConfigWidget::addFieldsWidget(const Tellico::StringMap& customFields_, const QStringList& fieldsToAdd_) {
  if(customFields_.isEmpty()) {
    return;
  }

  QGroupBox* gbox = new QGroupBox(i18n("Available Fields"), this);
  static_cast<QBoxLayout*>(layout())->addWidget(gbox);

  QVBoxLayout* vbox = new QVBoxLayout();
  for(StringMap::ConstIterator it = customFields_.begin(); it != customFields_.end(); ++it) {
    QCheckBox* cb = new QCheckBox(it.value(), gbox);
    m_fields.insert(it.key(), cb);
    if(fieldsToAdd_.contains(it.key())) {
      cb->setChecked(true);
    }
    connect(cb, SIGNAL(clicked()), SLOT(slotSetModified()));
    vbox->addWidget(cb);
  }
  vbox->addStretch(1);
  gbox->setLayout(vbox);

  KAcceleratorManager::manage(this);
}

void ConfigWidget::saveFieldsConfig(KConfigGroup& config_) const {
  QStringList fields;
  QHash<QString, QCheckBox*>::const_iterator it = m_fields.constBegin();
  for( ; it != m_fields.constEnd(); ++it) {
    if(it.value()->isChecked()) {
      fields << it.key();
    }
  }
  config_.writeEntry(QString::fromLatin1("Custom Fields"), fields);
}

#include "configwidget.moc"
