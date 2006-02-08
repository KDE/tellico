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

#include "configwidget.h"

#include <kconfig.h>
#include <klocale.h>
#include <kaccelmanager.h>

#include <qvgroupbox.h>
#include <qlayout.h>

using Tellico::Fetch::ConfigWidget;

ConfigWidget::ConfigWidget(QWidget* parent_) : QWidget(parent_), m_modified(false), m_accepted(false) {
  QHBoxLayout* boxLayout = new QHBoxLayout(this);
  boxLayout->setSpacing(10);

  QGroupBox* vbox = new QVGroupBox(i18n("Source Options"), this);
  boxLayout->addWidget(vbox, 10 /*stretch*/);

  m_optionsWidget = new QWidget(vbox);
}

void ConfigWidget::addFieldsWidget(const StringMap& customFields_, const QStringList& fieldsToAdd_) {
  if(customFields_.isEmpty()) {
    return;
  }

  QVGroupBox* box = new QVGroupBox(i18n("Available Fields"), this);
  static_cast<QBoxLayout*>(layout())->addWidget(box);
  for(StringMap::ConstIterator it = customFields_.begin(); it != customFields_.end(); ++it) {
    QCheckBox* cb = new QCheckBox(it.data(), box);
    m_fields.insert(it.key(), cb);
    if(fieldsToAdd_.contains(it.key())) {
      cb->setChecked(true);
    }
    connect(cb, SIGNAL(clicked()), SLOT(slotSetModified()));
  }

  KAcceleratorManager::manage(this);

  return;
}

void ConfigWidget::saveFieldsConfig(KConfig* config_) const {
  QStringList fields;
  for(QDictIterator<QCheckBox> it(m_fields); it.current(); ++it) {
    if(it.current()->isChecked()) {
      fields << it.currentKey();
    }
  }
  config_->writeEntry(QString::fromLatin1("Custom Fields"), fields);
}

#include "configwidget.moc"
