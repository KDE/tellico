/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "fetcherconfigdialog.h"
#include "fetch/fetchmanager.h"

#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kiconloader.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qhgroupbox.h>
#include <qwidgetstack.h>
#include <qwhatsthis.h>
#include <qhbox.h>
#include <qvgroupbox.h>

namespace {
  static const int FETCHER_CONFIG_MIN_WIDTH = 600;
}

using Tellico::FetcherConfigDialog;

FetcherConfigDialog::FetcherConfigDialog(QWidget* parent_)
    : KDialogBase(parent_, "fetcher dialog", true, i18n("Data Source Properties"),
               KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Help)
    , m_newSource(true)
    , m_configWidget(0) {
  init(Fetch::Unknown);
}

FetcherConfigDialog::FetcherConfigDialog(const QString& sourceName_, Fetch::Type type_, Fetch::ConfigWidget* configWidget_, QWidget* parent_)
    : KDialogBase(parent_, "fetcher dialog", true, i18n("Data Source Properties"),
               KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Help)
    , m_newSource(false)
    , m_configWidget(configWidget_) {
  init(type_);
  m_nameEdit->setText(sourceName_);
}

void FetcherConfigDialog::init(Fetch::Type type_) {
  setMinimumWidth(FETCHER_CONFIG_MIN_WIDTH);
  setHelp(QString::fromLatin1("data-sources-options"));

  QWidget* widget = new QWidget(this);
  QBoxLayout* topLayout = new QHBoxLayout(widget, KDialog::spacingHint());

  QBoxLayout* vlay1 = new QVBoxLayout(topLayout, KDialog::spacingHint());
  QLabel* icon = new QLabel(widget);
  icon->setPixmap(KGlobal::iconLoader()->loadIcon(QString::fromLatin1("network"), KIcon::Panel, 64));
  vlay1->addWidget(icon);
  vlay1->addStretch(1);

  QBoxLayout* vlay2 = new QVBoxLayout(topLayout, KDialog::spacingHint());

  QGridLayout* gl = new QGridLayout(vlay2, 2, 2, KDialog::spacingHint());

  QLabel* label = new QLabel(i18n("&Source name: "), widget);
  gl->addWidget(label, 0, 0);
  QString w = i18n("The name identifies the data source and should be unique and informative.");
  QWhatsThis::add(label, w);

  m_nameEdit = new KLineEdit(widget);
  gl->addWidget(m_nameEdit, 0, 1);
  m_nameEdit->setFocus();
  QWhatsThis::add(m_nameEdit, w);
  label->setBuddy(m_nameEdit);

  if(m_newSource) {
    label = new QLabel(i18n("Source &type: "), widget);
    gl->addWidget(label, 1, 0);
  } else {
    // since the label doesn't have a buddy, we don't want an accel,
    // but also want to reuse string we already have
    label = new QLabel(i18n("Source &type: ").remove('&'), widget);
  }
  gl->addWidget(label, 1, 0);
  w = i18n("Tellico supports several different data sources.");
  QWhatsThis::add(label, w);

  if(m_newSource) {
    m_typeCombo = new KComboBox(widget);
    gl->addWidget(m_typeCombo, 1, 1);
    QWhatsThis::add(m_typeCombo, w);
    label->setBuddy(m_typeCombo);
  } else {
    m_typeCombo = 0;
    QLabel* lab = new QLabel(Fetch::Manager::sourceMap()[type_], widget);
    gl->addWidget(lab, 1, 1);
    QWhatsThis::add(lab, w);
  }

  if(m_newSource) {
    m_stack = new QWidgetStack(widget);
    vlay2->addWidget(m_stack);
    connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotNewSourceSelected(int)));

    const Fetch::FetchMap fetchMap = Fetch::Manager::sourceMap();
    for(Fetch::FetchMap::ConstIterator it = fetchMap.begin(); it != fetchMap.end(); ++it) {
      m_typeCombo->insertItem(it.data());
    }
    // make sure first widget gets initialized
    m_typeCombo->setCurrentItem(0);
    slotNewSourceSelected(m_typeCombo->currentItem());
  } else {
    m_stack = 0;
    // just add config widget and reparent
    m_configWidget->reparent(widget, QPoint());
    vlay2->addWidget(m_configWidget);
  }

  setMainWidget(widget);
}

QString FetcherConfigDialog::sourceName() const {
  return m_nameEdit->text();
}

Tellico::Fetch::ConfigWidget* FetcherConfigDialog::configWidget() const {
  if(m_newSource) {
    return dynamic_cast<Fetch::ConfigWidget*>(m_stack->visibleWidget());
  }
  kdWarning() << "FetcherConfigDialog::configWidget() called for modifying existing fetcher!" << endl;
  return m_configWidget;
}

Tellico::Fetch::Type FetcherConfigDialog::sourceType() const {
  if(!m_newSource) {
    kdWarning() << "FetcherConfigDialog::sourceType() called for modifying existing fetcher!" << endl;
    return Fetch::Unknown;
  }

  const Fetch::FetchMap fetchMap = Fetch::Manager::sourceMap();
  for(Fetch::FetchMap::ConstIterator it = fetchMap.begin(); it != fetchMap.end(); ++it) {
    if(it.data() == m_typeCombo->currentText()) {
      return it.key();
    }
  }
  return Fetch::Unknown;
}

void FetcherConfigDialog::slotNewSourceSelected(int idx_) {
  if(!m_newSource) {
    return;
  }
  static int oldIndex = -1;
  QString s = m_nameEdit->text();
  if(s.isEmpty() || s == Fetch::Manager::defaultSourceName(m_typeCombo->text(oldIndex))) {
    m_nameEdit->setText(Fetch::Manager::defaultSourceName(m_typeCombo->currentText()));
  }
  oldIndex = idx_;

  Fetch::ConfigWidget* cw = m_configWidgets[idx_];
  if(cw) {
    m_stack->raiseWidget(cw);
    return;
  }

  QString sourceType = m_typeCombo->currentText();

  Fetch::Type type = Fetch::Unknown;
  Fetch::FetchMap fetchMap = Fetch::Manager::sourceMap();
  for(Fetch::FetchMap::ConstIterator it = fetchMap.begin(); it != fetchMap.end(); ++it) {
    if(it.data() == sourceType) {
      type = it.key();
      break;
    }
  }
  if(type == Fetch::Unknown) {
    kdWarning() << "FetcherConfigDialog::slotNewSourceSelected() - unknown source type - " << sourceType << endl;
    return;
  }
  cw = Fetch::Manager::configWidget(type, m_stack);
  if(!cw) {
    // bad bad bad!
    kdWarning() << "FetcherConfigDialog::slotNewSourceSelected() - no config widget found for " << sourceType << endl;
    m_typeCombo->setCurrentItem(oldIndex);
    slotNewSourceSelected(oldIndex);
    return;
  }
  m_configWidgets.insert(idx_, cw);
  m_stack->addWidget(cw);
  m_stack->raiseWidget(cw);
}

#include "fetcherconfigdialog.moc"
