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
#include "gui/combobox.h"
#include "tellico_debug.h"

#include <KLocalizedString>
#include <kcombobox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kservicetypetrader.h>
#include <kregexpeditorinterface.h>
#include <kiconloader.h>
#include <kdatecombobox.h>

#include <QLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>

using Tellico::FilterRuleWidget;
using Tellico::FilterRuleWidgetLister;
using Tellico::FilterDialog;

FilterRuleWidget::FilterRuleWidget(Tellico::FilterRule* rule_, QWidget* parent_)
    : QWidget(parent_), m_ruleDate(0), m_editRegExp(0), m_editRegExpDialog(0), m_ruleType(General) {
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(0);
  l->setSizeConstraint(QLayout::SetFixedSize);

  initLists();
  initWidget();

  if(rule_) {
    setRule(rule_);
  } else {
    reset();
  }
}

void FilterRuleWidget::initLists() {
  //---------- initialize list of filter fields
  if(m_ruleFieldList.isEmpty()) {
    m_ruleFieldList.append(QLatin1Char('<') + i18n("Any Field") + QLatin1Char('>'));
    QStringList titles = Kernel::self()->fieldTitles();
    titles.sort();
    m_ruleFieldList += titles;
  }

}

void FilterRuleWidget::initWidget() {
  m_ruleField = new KComboBox(this);
  layout()->addWidget(m_ruleField);
  connect(m_ruleField, SIGNAL(activated(int)), SIGNAL(signalModified()));
  connect(m_ruleField, SIGNAL(activated(int)), SLOT(slotRuleFieldChanged(int)));

  m_ruleFunc = new GUI::ComboBox(this);
  layout()->addWidget(m_ruleFunc);
  connect(m_ruleFunc, SIGNAL(activated(int)), SIGNAL(signalModified()));
  connect(m_ruleFunc, SIGNAL(activated(int)), SLOT(slotRuleFunctionChanged(int)));

  m_valueStack = new QStackedWidget(this);
  layout()->addWidget(m_valueStack);

  m_ruleValue = new KLineEdit(m_valueStack);
  connect(m_ruleValue, SIGNAL(textChanged(const QString&)), SIGNAL(signalModified()));
  m_valueStack->addWidget(m_ruleValue);

  m_ruleDate = new KDateComboBox(m_valueStack);
  connect(m_ruleDate, SIGNAL(dateChanged(const QDate&)), SIGNAL(signalModified()));
  m_valueStack->addWidget(m_ruleDate);

  if(!KServiceTypeTrader::self()->query(QLatin1String("KRegExpEditor/KRegExpEditor")).isEmpty()) {
    m_editRegExp = new KPushButton(i18n("Edit..."), this);
    connect(m_editRegExp, SIGNAL(clicked()), this, SLOT(slotEditRegExp()));
  }

  m_ruleField->addItems(m_ruleFieldList);
  updateFunctionList();
  slotRuleFunctionChanged(m_ruleFunc->currentIndex());
}

void FilterRuleWidget::slotEditRegExp() {
  if(!m_editRegExpDialog) {
    m_editRegExpDialog = KServiceTypeTrader::createInstanceFromQuery<QDialog>(QLatin1String("KRegExpEditor/KRegExpEditor"),
                                                                              QString(), this);  //krazy:exclude=qclasses
  }

  if(!m_editRegExpDialog) {
    myWarning() << "no dialog";
    return;
  }

  KRegExpEditorInterface* iface = ::qobject_cast<KRegExpEditorInterface*>(m_editRegExpDialog);
  if(iface) {
    iface->setRegExp(m_ruleValue->text());
    if(m_editRegExpDialog->exec() == QDialog::Accepted) {
      m_ruleValue->setText(iface->regExp());
    }
  }
}

void FilterRuleWidget::slotRuleFieldChanged(int which_) {
  Q_UNUSED(which_);
  m_ruleType = General;
  QString fieldTitle = m_ruleField->currentText();
  if(fieldTitle.isEmpty() || fieldTitle[0] == QLatin1Char('<')) {
    m_ruleValue->setCompletionObject(0);
    updateFunctionList();
    return;
  }
  Data::FieldPtr field = Data::Document::self()->collection()->fieldByTitle(fieldTitle);
  if(field && field->hasFlag(Data::Field::AllowCompletion)) {
    FieldCompletion* completion = new FieldCompletion(field->hasFlag(Data::Field::AllowMultiple));
    completion->setItems(Kernel::self()->valuesByFieldName(field->name()));
    completion->setIgnoreCase(true);
    m_ruleValue->setCompletionObject(completion);
    m_ruleValue->setAutoDeleteCompletionObject(true);
  } else {
    m_ruleValue->setCompletionObject(0);
  }

  if(field) {
    if(field->type() == Data::Field::Date) {
      m_ruleType = Date;
    } else if(field->type() == Data::Field::Number || field->type() == Data::Field::Rating) {
      m_ruleType = Number;
    }
  }
  updateFunctionList();
}

void FilterRuleWidget::slotRuleFunctionChanged(int which_) {
  const QVariant data = m_ruleFunc->itemData(which_);
  if(m_editRegExp) {
    m_editRegExp->setEnabled(data == FilterRule::FuncRegExp ||
                             data == FilterRule::FuncNotRegExp);
  }

  // don't show the date picker if we're using regular expressions
  if(m_ruleType == Date && data != FilterRule::FuncRegExp && data != FilterRule::FuncNotRegExp) {
    m_valueStack->setCurrentWidget(m_ruleDate);
  } else {
    m_valueStack->setCurrentWidget(m_ruleValue);
    m_ruleValue->setClickMessage(QString());
    if(m_ruleType == Number) {
      m_ruleValue->setValidator(new QIntValidator(this));
    } else {
      m_ruleValue->setValidator(0);
    }
  }
}

void FilterRuleWidget::setRule(const Tellico::FilterRule* rule_) {
  if(!rule_) {
    reset();
    return;
  }

  blockSignals(true);

  m_ruleType = General;
  if(rule_->fieldName().isEmpty()) {
    m_ruleField->setCurrentIndex(0); // "All Fields"
  } else {
    Data::FieldPtr field = Data::Document::self()->collection()->fieldByName(rule_->fieldName());
    if(field && field->type() == Data::Field::Date) {
      m_ruleType = Date;
      const QDate date = QDate::fromString(rule_->pattern(), Qt::ISODate);
      if(date.isValid()) {
        m_ruleDate->setDate(date);
      }
    }
    const int idx = m_ruleField->findText(field ? field->title() : QString());
    m_ruleField->setCurrentIndex(idx);
  }

  // update the rulle fields first, before possible values
  slotRuleFieldChanged(m_ruleField->currentIndex());

  //--------------set function and contents
  m_ruleFunc->setCurrentData(rule_->function());
  m_ruleValue->setText(rule_->pattern());

  slotRuleFunctionChanged(m_ruleFunc->currentIndex());
  blockSignals(false);
}

Tellico::FilterRule* FilterRuleWidget::rule() const {
  QString fieldName; // empty string
  if(m_ruleField->currentIndex() > 0) { // 0 is "All Fields", field is empty
    fieldName = Kernel::self()->fieldNameByTitle(m_ruleField->currentText());
  }

  QString ruleValue;
  if(m_valueStack->currentWidget() == m_ruleDate) {
    ruleValue = m_ruleDate->date().toString(Qt::ISODate);
  } else {
    ruleValue = m_ruleValue->text().trimmed();
  }

  return new FilterRule(fieldName, ruleValue,
                        static_cast<FilterRule::Function>(m_ruleFunc->currentData().toInt()));
}

void FilterRuleWidget::reset() {
//  myDebug();
  blockSignals(true);

  m_ruleField->setCurrentIndex(0);
  m_ruleFunc->setCurrentIndex(0);
  m_ruleValue->clear();

  if(m_editRegExp) {
    m_editRegExp->setEnabled(false);
  }

  blockSignals(false);
}

void FilterRuleWidget::setFocus() {
  m_ruleValue->setFocus();
}

void FilterRuleWidget::updateFunctionList() {
  Q_ASSERT(m_ruleFunc);
  const QVariant data = m_ruleFunc->currentData();
  m_ruleFunc->clear();
  switch(m_ruleType) {
    case Date:
      m_ruleFunc->addItem(i18n("equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("does not equal"), FilterRule::FuncNotEquals);
      m_ruleFunc->addItem(i18n("matches regexp"), FilterRule::FuncRegExp);
      m_ruleFunc->addItem(i18n("does not match regexp"), FilterRule::FuncNotRegExp);
      m_ruleFunc->addItem(i18nc("is before a date", "is before"), FilterRule::FuncBefore);
      m_ruleFunc->addItem(i18nc("is after a date", "is after"), FilterRule::FuncAfter);
      break;
    case Number:
      m_ruleFunc->addItem(i18n("equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("does not equal"), FilterRule::FuncNotEquals);
      m_ruleFunc->addItem(i18n("matches regexp"), FilterRule::FuncRegExp);
      m_ruleFunc->addItem(i18n("does not match regexp"), FilterRule::FuncNotRegExp);
      m_ruleFunc->addItem(i18nc("is less than a number", "is less than"), FilterRule::FuncLess);
      m_ruleFunc->addItem(i18nc("is greater than a number", "is greater than"), FilterRule::FuncGreater);
      break;
    case General:
      m_ruleFunc->addItem(i18n("contains"), FilterRule::FuncContains);
      m_ruleFunc->addItem(i18n("does not contain"), FilterRule::FuncNotContains);
      m_ruleFunc->addItem(i18n("equals"), FilterRule::FuncEquals);
      m_ruleFunc->addItem(i18n("does not equal"), FilterRule::FuncNotEquals);
      m_ruleFunc->addItem(i18n("matches regexp"), FilterRule::FuncRegExp);
      m_ruleFunc->addItem(i18n("does not match regexp"), FilterRule::FuncNotRegExp);
      break;
  }
  m_ruleFunc->setCurrentData(data);
  slotRuleFunctionChanged(m_ruleFunc->currentIndex());
}

/***************************************************************/

namespace {
  static const int FILTER_MIN_RULE_WIDGETS = 1;
  static const int FILTER_MAX_RULES = 8;
}

FilterRuleWidgetLister::FilterRuleWidgetLister(QWidget* parent_)
    : KWidgetLister(FILTER_MIN_RULE_WIDGETS, FILTER_MAX_RULES, parent_) {
//  slotClear();
  connect(this, SIGNAL(clearWidgets()), SIGNAL(signalModified()));
}

void FilterRuleWidgetLister::setFilter(Tellico::FilterPtr filter_) {
//  if(mWidgetList.first()) { // move this below next 'if'?
//    mWidgetList.first()->blockSignals(true);
//  }

  if(filter_->isEmpty()) {
    slotClear();
//    mWidgetList.first()->blockSignals(false);
    return;
  }

  const int count = filter_->count();
  if(count > mMaxWidgets) {
    myDebug() << "more rules than allowed!";
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo(qMax(count, mMinWidgets));

  // load the actions into the widgets
  int i = 0;
  for( ; i < filter_->count(); ++i) {
    static_cast<FilterRuleWidget*>(mWidgetList.at(i))->setRule(filter_->at(i));
  }
  for( ; i < mWidgetList.count(); ++i) { // clear any remaining
    static_cast<FilterRuleWidget*>(mWidgetList.at(i))->reset();
  }

//  mWidgetList.first()->blockSignals(false);
}

void FilterRuleWidgetLister::reset() {
  slotClear();
}

void FilterRuleWidgetLister::setFocus() {
  if(!mWidgetList.isEmpty()) {
    mWidgetList.at(0)->setFocus();
  }
}

QWidget* FilterRuleWidgetLister::createWidget(QWidget* parent_) {
  QWidget* w = new FilterRuleWidget(static_cast<Tellico::FilterRule*>(0), parent_);
  connect(w, SIGNAL(signalModified()), SIGNAL(signalModified()));
  return w;
}

void FilterRuleWidgetLister::clearWidget(QWidget* widget_) {
  if(widget_) {
    static_cast<FilterRuleWidget*>(widget_)->reset();
  }
}

QList<QWidget*> FilterRuleWidgetLister::widgetList() const {
  return mWidgetList;
}

/***************************************************************/

namespace {
  static const int FILTER_MIN_WIDTH = 600;
}

// modal dialog so I don't have to worry about updating stuff
// don't show apply button if not saving, i.e. just modifying existing filter
FilterDialog::FilterDialog(Mode mode_, QWidget* parent_)
    : KDialog(parent_),
      m_filter(0), m_mode(mode_), m_saveFilter(0) {
  setModal(true);
  setCaption(mode_ == CreateFilter ? i18n("Advanced Filter") : i18n("Modify Filter"));
  setButtons(mode_ == CreateFilter ? Help|Ok|Apply|Cancel : Help|Ok|Cancel);
  setDefaultButton(Ok);
  showButtonSeparator(false);
  init();
}

void FilterDialog::init() {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
  QVBoxLayout* topLayout = new QVBoxLayout(page);

  QGroupBox* m_matchGroup = new QGroupBox(i18n("Filter Criteria"), page);
  QVBoxLayout* vlay = new QVBoxLayout(m_matchGroup);
  topLayout->addWidget(m_matchGroup);

  m_matchAll = new QRadioButton(i18n("Match a&ll of the following"), m_matchGroup);
  m_matchAny = new QRadioButton(i18n("Match an&y of the following"), m_matchGroup);
  m_matchAll->setChecked(true);

  vlay->addWidget(m_matchAll);
  vlay->addWidget(m_matchAny);

  QButtonGroup* bg = new QButtonGroup(m_matchGroup);
  bg->addButton(m_matchAll);
  bg->addButton(m_matchAny);
  connect(bg, SIGNAL(buttonClicked(int)), SLOT(slotFilterChanged()));

  m_ruleLister = new FilterRuleWidgetLister(m_matchGroup);
  connect(m_ruleLister, SIGNAL(widgetRemoved()), SLOT(slotShrink()));
  connect(m_ruleLister, SIGNAL(signalModified()), SLOT(slotFilterChanged()));
  m_ruleLister->setFocus();
  vlay->addWidget(m_ruleLister);

  QHBoxLayout* blay = new QHBoxLayout();
  topLayout->addLayout(blay);
  QLabel* lab = new QLabel(i18n("Filter name:"), page);
  blay->addWidget(lab);

  m_filterName = new KLineEdit(page);
  blay->addWidget(m_filterName);
  connect(m_filterName, SIGNAL(textChanged(const QString&)), SLOT(slotFilterChanged()));

  // only when creating a new filter can it be saved
  if(m_mode == CreateFilter) {
    m_saveFilter = new KPushButton(QIcon::fromTheme(QLatin1String("view-filter")), i18n("&Save Filter"), page);
    blay->addWidget(m_saveFilter);
    m_saveFilter->setEnabled(false);
    connect(m_saveFilter, SIGNAL(clicked()), SLOT(slotSaveFilter()));
    enableButtonApply(false);
  }
  enableButtonOk(false); // disable at start
  button(Help)->setDefault(false); // Help automatically becomes default when OK is disabled
  button(Cancel)->setDefault(true); // Help automatically becomes default when OK is disabled
  setMinimumWidth(qMax(minimumWidth(), FILTER_MIN_WIDTH));
  setHelp(QLatin1String("filter-dialog"));
  connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
  connect(this, SIGNAL(applyClicked()), SLOT(slotApply()));
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
    enableButtonApply(!emptyFilter);
  }
  enableButtonApply(enableOk);
  enableButtonOk(enableOk);
  button(Ok)->setDefault(enableOk);
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

