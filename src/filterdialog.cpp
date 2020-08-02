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

// The layout borrows heavily from kmsearchpatternedit.cpp in kmail
// which is authored by Marc Mutz <Marc@Mutz.com> under the GPL

#include "filterdialog.h"
#include "tellico_kernel.h"
#include "document.h"
#include "collection.h"
#include "fieldcompletion.h"
#include "gui/filterrulewidgetlister.h"
#include "gui/filterrulewidget.h"
#include "tellico_debug.h"

#include <KLocalizedString>
#include <KHelpClient>

#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QApplication>
#include <QFrame>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QDialogButtonBox>

using Tellico::FilterDialog;

namespace {
  static const int FILTER_MIN_WIDTH = 600;
}

// modal dialog so I don't have to worry about updating stuff
// don't show apply button if not saving, i.e. just modifying existing filter
FilterDialog::FilterDialog(Mode mode_, QWidget* parent_)
    : QDialog(parent_), m_filter(nullptr), m_mode(mode_), m_saveFilter(nullptr) {
  setModal(true);
  setWindowTitle(mode_ == CreateFilter ? i18n("Advanced Filter") : i18n("Modify Filter"));

  QVBoxLayout* topLayout = new QVBoxLayout();
  setLayout(topLayout);

  QDialogButtonBox* buttonBox;
  if(mode_ == CreateFilter) {
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Help|QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
  } else {
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Help|QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  }
  m_okButton = buttonBox->button(QDialogButtonBox::Ok);
  m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
  connect(m_okButton, &QAbstractButton::clicked, this, &FilterDialog::slotOk);
  if(m_applyButton) {
    connect(m_applyButton, &QAbstractButton::clicked, this, &FilterDialog::slotApply);
  }
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(buttonBox, &QDialogButtonBox::helpRequested, this, &FilterDialog::slotHelp);

  QGroupBox* m_matchGroup = new QGroupBox(i18n("Filter Criteria"), this);
  QVBoxLayout* vlay = new QVBoxLayout(m_matchGroup);
  topLayout->addWidget(m_matchGroup);
  m_matchGroup->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  m_matchAll = new QRadioButton(i18n("Match a&ll of the following"), m_matchGroup);
  m_matchAny = new QRadioButton(i18n("Match an&y of the following"), m_matchGroup);
  m_matchAll->setChecked(true);

  vlay->addWidget(m_matchAll);
  vlay->addWidget(m_matchAny);

  QButtonGroup* bg = new QButtonGroup(m_matchGroup);
  bg->addButton(m_matchAll);
  bg->addButton(m_matchAny);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  void (QButtonGroup::* buttonClicked)(int) = &QButtonGroup::buttonClicked;
  connect(bg, buttonClicked, this, &FilterDialog::slotFilterChanged);
#else
  connect(bg, &QButtonGroup::idClicked, this, &FilterDialog::slotFilterChanged);
#endif

  m_ruleLister = new FilterRuleWidgetLister(m_matchGroup);
  connect(m_ruleLister, &KWidgetLister::widgetRemoved, this, &FilterDialog::slotShrink);
  connect(m_ruleLister, &FilterRuleWidgetLister::signalModified, this, &FilterDialog::slotFilterChanged);
  m_ruleLister->setFocus();
  vlay->addWidget(m_ruleLister);

  QHBoxLayout* blay = new QHBoxLayout();
  topLayout->addLayout(blay);
  QLabel* lab = new QLabel(i18n("Filter name:"), this);
  blay->addWidget(lab);

  m_filterName = new QLineEdit(this);
  blay->addWidget(m_filterName);
  connect(m_filterName, &QLineEdit::textChanged, this, &FilterDialog::slotFilterChanged);

  // only when creating a new filter can it be saved
  if(m_mode == CreateFilter) {
    m_saveFilter = new QPushButton(QIcon::fromTheme(QStringLiteral("view-filter")), i18n("&Save Filter"), this);
    blay->addWidget(m_saveFilter);
    m_saveFilter->setEnabled(false);
    connect(m_saveFilter, &QAbstractButton::clicked, this, &FilterDialog::slotSaveFilter);
    m_applyButton->setEnabled(false);
  }
  m_okButton->setEnabled(false); // disable at start
  buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
  setMinimumWidth(qMax(minimumWidth(), FILTER_MIN_WIDTH));
  topLayout->addWidget(buttonBox);
}

Tellico::FilterPtr FilterDialog::currentFilter(bool alwaysCreateNew_) {
  FilterPtr newFilter(new Filter(Filter::MatchAny));

  if(m_matchAll->isChecked()) {
    newFilter->setMatch(Filter::MatchAll);
  } else {
    newFilter->setMatch(Filter::MatchAny);
  }

  foreach(QWidget* widget, m_ruleLister->widgetList()) {
    FilterRuleWidget* rw = static_cast<FilterRuleWidget*>(widget);
    FilterRule* rule = rw->rule();
    if(rule && !rule->isEmpty()) {
      newFilter->append(rule);
    } else {
      delete rule;
    }
  }
  newFilter->setName(m_filterName->text());
  if(!m_filter || !alwaysCreateNew_) {
    m_filter = newFilter;
  }
  return newFilter;
}

void FilterDialog::setFilter(Tellico::FilterPtr filter_) {
  if(!filter_) {
    slotClear();
    return;
  }

  if(filter_->op() == Filter::MatchAll) {
    m_matchAll->setChecked(true);
  } else {
    m_matchAny->setChecked(true);
  }

  m_ruleLister->setFilter(filter_);
  m_filterName->setText(filter_->name());
  m_filter = filter_;
}

void FilterDialog::slotOk() {
  slotApply();
  accept();
}

void FilterDialog::slotApply() {
  emit signalUpdateFilter(currentFilter());
}

void FilterDialog::slotHelp() {
  KHelpClient::invokeHelp(QStringLiteral("filter-dialog"));
}

void FilterDialog::slotClear() {
//  myDebug();
  m_matchAll->setChecked(true);
  m_ruleLister->reset();
  m_filterName->clear();
}

void FilterDialog::slotShrink() {
  updateGeometry();
  QApplication::sendPostedEvents();
  resize(width(), sizeHint().height());
}

void FilterDialog::slotFilterChanged() {
  const bool hadFilter = m_filter && !m_filter->isEmpty();
  const bool emptyFilter = currentFilter(true)->isEmpty();
  // an empty filter can be ok if the filter was originally not empty
  const bool enableOk = !currentFilter()->isEmpty() || hadFilter;
  if(m_saveFilter) {
    m_saveFilter->setEnabled(!m_filterName->text().isEmpty() && !emptyFilter);
    if(m_applyButton) {
      m_applyButton->setEnabled(!emptyFilter);
    }
  }
  if(m_applyButton) {
    m_applyButton->setEnabled(enableOk);
  }
  m_okButton->setEnabled(enableOk);
  m_okButton->setDefault(enableOk);
}

void FilterDialog::slotSaveFilter() {
  // non-op if editing an existing filter
  if(m_mode != CreateFilter) {
    return;
  }

  // in this case, currentFilter() either creates a new filter or
  // updates the current one. If creating a new one, then I want to copy it
  const bool wasEmpty = !m_filter;
  FilterPtr filter(new Filter(*currentFilter()));
  if(wasEmpty) {
    m_filter = filter;
  }
  // this keeps the saving completely decoupled from the filter setting in the detailed view
  if(filter->isEmpty()) {
    m_filter = FilterPtr();
    return;
  }
  Kernel::self()->addFilter(filter);
}
