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
#include "gui/combobox.h"

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
#include <qcheckbox.h>

namespace {
  static const int FETCHER_CONFIG_MIN_WIDTH = 600;
}

using Tellico::FetcherConfigDialog;

FetcherConfigDialog::FetcherConfigDialog(QWidget* parent_)
    : KDialogBase(parent_, "fetcher dialog", true, i18n("Data Source Properties"),
               KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Help)
    , m_newSource(true)
    , m_useDefaultName(true)
    , m_configWidget(0) {
  init(Fetch::Unknown);
}

FetcherConfigDialog::FetcherConfigDialog(const QString& sourceName_, Fetch::Type type_, bool updateOverwrite_,
                                         Fetch::ConfigWidget* configWidget_, QWidget* parent_)
    : KDialogBase(parent_, "fetcher dialog", true, i18n("Data Source Properties"),
               KDialogBase::Ok | KDialogBase::Cancel | KDialogBase::Help)
    , m_newSource(false)
    , m_useDefaultName(false)
    , m_configWidget(configWidget_) {
  init(type_);
  m_nameEdit->setText(sourceName_);
  m_cbOverwrite->setChecked(updateOverwrite_);
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
  int row = -1;

  QLabel* label = new QLabel(i18n("&Source name: "), widget);
  gl->addWidget(label, ++row, 0);
  QString w = i18n("The name identifies the data source and should be unique and informative.");
  QWhatsThis::add(label, w);

  m_nameEdit = new KLineEdit(widget);
  gl->addWidget(m_nameEdit, row, 1);
  m_nameEdit->setFocus();
  QWhatsThis::add(m_nameEdit, w);
  label->setBuddy(m_nameEdit);
  connect(m_nameEdit, SIGNAL(textChanged(const QString&)), SLOT(slotNameChanged(const QString&)));

  if(m_newSource) {
    label = new QLabel(i18n("Source &type: "), widget);
  } else {
    // since the label doesn't have a buddy, we don't want an accel,
    // but also want to reuse string we already have
    label = new QLabel(i18n("Source &type: ").remove('&'), widget);
  }
  gl->addWidget(label, ++row, 0);
  w = i18n("Tellico supports several different data sources.");
  QWhatsThis::add(label, w);

  if(m_newSource) {
    m_typeCombo = new GUI::ComboBox(widget);
    gl->addWidget(m_typeCombo, row, 1);
    QWhatsThis::add(m_typeCombo, w);
    label->setBuddy(m_typeCombo);
  } else {
    m_typeCombo = 0;
    QLabel* lab = new QLabel(Fetch::Manager::typeName(type_), widget);
    gl->addWidget(lab, row, 1);
    QWhatsThis::add(lab, w);
  }
  m_cbOverwrite = new QCheckBox(i18n("Updating from source should overwrite user data"), widget);
  ++row;
  gl->addMultiCellWidget(m_cbOverwrite, row, row, 0, 1);
  w = i18n("If checked, updating entries will overwrite any existing information.");
  QWhatsThis::add(m_cbOverwrite, w);

  if(m_newSource) {
    m_stack = new QWidgetStack(widget);
    vlay2->addWidget(m_stack);
    connect(m_typeCombo, SIGNAL(activated(int)), SLOT(slotNewSourceSelected(int)));

    int z3950_idx = 0;
    const Fetch::TypePairList typeList = Fetch::Manager::self()->typeList();
    for(Fetch::TypePairList::ConstIterator it = typeList.begin(); it != typeList.end(); ++it) {
      const Fetch::TypePair& type = *it;
      m_typeCombo->insertItem(type.index(), type.value());
      if(type.value() == Fetch::Z3950) {
        z3950_idx = m_typeCombo->count()-1;
      }
    }
    // make sure first widget gets initialized
    // I'd like it to be the z39.50 widget
    m_typeCombo->setCurrentItem(z3950_idx);
    slotNewSourceSelected(z3950_idx);
  } else {
    m_stack = 0;
    // just add config widget and reparent
    m_configWidget->reparent(widget, QPoint());
    vlay2->addWidget(m_configWidget);
    connect(m_configWidget, SIGNAL(signalName(const QString&)), SLOT(slotPossibleNewName(const QString&)));
  }

  setMainWidget(widget);
}

QString FetcherConfigDialog::sourceName() const {
  return m_nameEdit->text();
}

bool FetcherConfigDialog::updateOverwrite() const {
  return m_cbOverwrite->isChecked();
}

Tellico::Fetch::ConfigWidget* FetcherConfigDialog::configWidget() const {
  if(m_newSource) {
    return dynamic_cast<Fetch::ConfigWidget*>(m_stack->visibleWidget());
  }
  kdWarning() << "FetcherConfigDialog::configWidget() called for modifying existing fetcher!" << endl;
  return m_configWidget;
}

Tellico::Fetch::Type FetcherConfigDialog::sourceType() const {
  if(!m_newSource || m_typeCombo->count() == 0) {
    kdWarning() << "FetcherConfigDialog::sourceType() called for modifying existing fetcher!" << endl;
    return Fetch::Unknown;
  }
  return static_cast<Fetch::Type>(m_typeCombo->currentData().toInt());
}

void FetcherConfigDialog::slotNewSourceSelected(int idx_) {
  if(!m_newSource) {
    return;
  }

  // always change to default name
  m_useDefaultName = true;

  Fetch::ConfigWidget* cw = m_configWidgets[idx_];
  if(cw) {
    m_stack->raiseWidget(cw);
    slotPossibleNewName(cw->preferredName());
    return;
  }

  Fetch::Type type = sourceType();
  if(type == Fetch::Unknown) {
    kdWarning() << "FetcherConfigDialog::slotNewSourceSelected() - unknown source type" << endl;
    return;
  }
  cw = Fetch::Manager::self()->configWidget(m_stack, type, m_typeCombo->currentText());
  if(!cw) {
    // bad bad bad!
    kdWarning() << "FetcherConfigDialog::slotNewSourceSelected() - no config widget found for type " << type << endl;
    m_typeCombo->setCurrentItem(0);
    slotNewSourceSelected(0);
    return;
  }
  connect(cw, SIGNAL(signalName(const QString&)), SLOT(slotPossibleNewName(const QString&)));
  m_configWidgets.insert(idx_, cw);
  m_stack->addWidget(cw);
  m_stack->raiseWidget(cw);
  slotPossibleNewName(cw->preferredName());
}

void FetcherConfigDialog::slotNameChanged(const QString&) {
  m_useDefaultName = false;
}

void FetcherConfigDialog::slotPossibleNewName(const QString& name_) {
  if(name_.isEmpty()) {
    return;
  }
  Fetch::ConfigWidget* cw = m_stack ? static_cast<Fetch::ConfigWidget*>(m_stack->visibleWidget()) : m_configWidget;
  if(m_useDefaultName || (cw && m_nameEdit->text() == cw->preferredName())) {
    m_nameEdit->setText(name_);
    m_useDefaultName = true; // it gets reset in slotNameChanged()
  }
}

#include "fetcherconfigdialog.moc"
