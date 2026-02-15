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

#include "fetcherconfigdialog.h"
#include "fetch/fetchmanager.h"
#include "gui/combobox.h"
#include "utils/string_utils.h"
#include "tellico_debug.h"

#include <KLocalizedString>
#include <KComboBox>
#include <KIconLoader>
#include <KHelpClient>

#include <QLineEdit>
#include <QLabel>
#include <QLayout>
#include <QStackedWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

namespace {
  static const int FETCHER_CONFIG_MIN_WIDTH = 600;
}

using Tellico::FetcherConfigDialog;

FetcherConfigDialog::FetcherConfigDialog(QWidget* parent_)
    : QDialog(parent_)
    , m_newSource(true)
    , m_useDefaultName(true)
    , m_configWidget(nullptr) {
  init(Fetch::Unknown);
}

FetcherConfigDialog::FetcherConfigDialog(const QString& sourceName_, Tellico::Fetch::Type type_, bool updateOverwrite_,
                                         Tellico::Fetch::ConfigWidget* configWidget_, QWidget* parent_)
    : QDialog(parent_)
    , m_newSource(false)
    , m_useDefaultName(false)
    , m_configWidget(configWidget_) {
  init(type_);
  m_nameEdit->setText(sourceName_);
  m_cbOverwrite->setChecked(updateOverwrite_);
}

void FetcherConfigDialog::init(Tellico::Fetch::Type type_) {
  setModal(true);
  setWindowTitle(i18n("Data Source Properties"));

  QVBoxLayout* mainLayout = new QVBoxLayout();
  setLayout(mainLayout);

  setMinimumWidth(FETCHER_CONFIG_MIN_WIDTH);

  QWidget* widget = new QWidget(this);
  QBoxLayout* topLayout = new QHBoxLayout(widget);
  widget->setLayout(topLayout);

  QBoxLayout* vlay1 = new QVBoxLayout();
  topLayout->addLayout(vlay1);
  m_iconLabel = new QLabel(widget);
  if(type_ == Fetch::Unknown) {
    m_iconLabel->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("network-wired"), KIconLoader::Dialog, 64));
  } else {
    m_iconLabel->setPixmap(Fetch::Manager::self()->fetcherIcon(type_, KIconLoader::Dialog, 64));
  }
  vlay1->addWidget(m_iconLabel);
  vlay1->addStretch(1);

  QBoxLayout* vlay2 = new QVBoxLayout();
  topLayout->addLayout(vlay2);

  QGridLayout* gl = new QGridLayout();
  vlay2->addLayout(gl);
  int row = -1;

  QLabel* label = new QLabel(i18n("&Source name: "), widget);
  gl->addWidget(label, ++row, 0);
  QString w = i18n("The name identifies the data source and should be unique and informative.");
  label->setWhatsThis(w);

  m_nameEdit = new QLineEdit(widget);
  gl->addWidget(m_nameEdit, row, 1);
  m_nameEdit->setFocus();
  m_nameEdit->setWhatsThis(w);
  label->setBuddy(m_nameEdit);
  connect(m_nameEdit, &QLineEdit::textChanged, this, &FetcherConfigDialog::slotNameChanged);

  const QString labelText(i18n("Source &type: "));
  if(m_newSource) {
    label = new QLabel(labelText, widget);
  } else {
    // since the label doesn't have a buddy, we don't want an accel,
    // but also want to reuse string we already have
    label = new QLabel(KLocalizedString::removeAcceleratorMarker(labelText), widget);
  }
  gl->addWidget(label, ++row, 0);
  w = i18n("Tellico supports several different data sources.");
  label->setWhatsThis(w);

  if(m_newSource) {
    m_typeCombo = new GUI::ComboBox(widget);
    gl->addWidget(m_typeCombo, row, 1);
    m_typeCombo->setWhatsThis(w);
    label->setBuddy(m_typeCombo);
  } else {
    m_typeCombo = nullptr;
    QLabel* lab = new QLabel(Fetch::Manager::typeName(type_), widget);
    gl->addWidget(lab, row, 1);
    lab->setWhatsThis(w);
  }
  m_cbOverwrite = new QCheckBox(i18n("Updating from source should overwrite user data"), widget);
  ++row;
  gl->addWidget(m_cbOverwrite, row, 0, 1, 2);
  w = i18n("If checked, updating entries will overwrite any existing information.");
  m_cbOverwrite->setWhatsThis(w);

  if(m_newSource) {
    m_stack = new QStackedWidget(widget);
    vlay2->addWidget(m_stack);
    void (QComboBox::* activatedInt)(int) = &QComboBox::activated;
    connect(m_typeCombo, activatedInt, this, &FetcherConfigDialog::slotNewSourceSelected);

    int z3950_idx = 0;
    const auto typeByName = Fetch::Manager::self()->nameTypeHash();
    auto typeKeys = typeByName.keys();
    typeKeys.sort(Qt::CaseInsensitive);
    // key is the fetcher name, value is the fetcher type
    for(auto it = typeKeys.constBegin(); it != typeKeys.constEnd(); ++it) {
      const auto typeValue = typeByName.value(*it);
      m_typeCombo->addItem(Fetch::Manager::self()->fetcherIcon(typeValue), *it, typeValue);
      if(typeValue == Fetch::Z3950) {
        z3950_idx = m_typeCombo->count()-1;
      }
    }
    // make sure first widget gets initialized
    // I'd like it to be the z39.50 widget
    m_typeCombo->setCurrentIndex(z3950_idx);
    slotNewSourceSelected(z3950_idx);
  } else {
    m_stack = nullptr;
    // just add config widget and reparent
    m_configWidget->setParent(widget);
    m_configWidget->show();
    vlay2->addWidget(m_configWidget);
    connect(m_configWidget, &Fetch::ConfigWidget::signalName, this, &FetcherConfigDialog::slotPossibleNewName);
  }

  mainLayout->addWidget(widget);

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|
                                                     QDialogButtonBox::Cancel|
                                                     QDialogButtonBox::Help);
  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(buttonBox, &QDialogButtonBox::helpRequested, this, &FetcherConfigDialog::slotHelp);
  mainLayout->addWidget(buttonBox);
}

QString FetcherConfigDialog::sourceName() const {
  return m_nameEdit->text();
}

bool FetcherConfigDialog::updateOverwrite() const {
  return m_cbOverwrite->isChecked();
}

Tellico::Fetch::ConfigWidget* FetcherConfigDialog::configWidget() const {
  if(m_newSource) {
    return dynamic_cast<Fetch::ConfigWidget*>(m_stack->currentWidget());
  }
  myWarning() << "called for modifying existing fetcher!";
  return m_configWidget;
}

Tellico::Fetch::Type FetcherConfigDialog::sourceType() const {
  if(!m_newSource || m_typeCombo->count() == 0) {
    myWarning() << "called for modifying existing fetcher!";
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
    m_stack->setCurrentWidget(cw);
    slotPossibleNewName(cw->preferredName());
    return;
  }

  const Fetch::Type type = sourceType();
  if(type == Fetch::Unknown) {
    myWarning() << "unknown source type";
    return;
  }
  m_iconLabel->setPixmap(Fetch::Manager::self()->fetcherIcon(type, KIconLoader::Dialog, 64));
  cw = Fetch::Manager::self()->configWidget(m_stack, type, m_typeCombo->currentText());
  if(!cw) {
    // bad bad bad!
    myWarning() << "No config widget found for type" << type;
    m_typeCombo->setCurrentIndex(0);
    slotNewSourceSelected(0);
    return;
  }
  connect(cw, &Fetch::ConfigWidget::signalName, this, &FetcherConfigDialog::slotPossibleNewName);
  m_configWidgets.insert(idx_, cw);
  m_stack->addWidget(cw);
  m_stack->setCurrentWidget(cw);
  slotPossibleNewName(cw->preferredName());
}

void FetcherConfigDialog::slotNameChanged(const QString&) {
  m_useDefaultName = false;
}

void FetcherConfigDialog::slotPossibleNewName(const QString& name_) {
  if(name_.isEmpty()) {
    return;
  }
  Fetch::ConfigWidget* cw = m_stack ? static_cast<Fetch::ConfigWidget*>(m_stack->currentWidget()) : m_configWidget;
  if(m_useDefaultName || (cw && m_nameEdit->text() == cw->preferredName())) {
    m_nameEdit->setText(name_);
    m_useDefaultName = true; // it gets reset in slotNameChanged()
  }
}

void FetcherConfigDialog::slotHelp() {
  KHelpClient::invokeHelp(QStringLiteral("data-sources-options"));
}
