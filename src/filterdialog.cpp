/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

// The layout borrows heavily from kmsearchpatternedit.cpp in kmail
// which is authored by Marc Mutz <Marc@Mutz.com> under the GPL

#include "filterdialog.h"
#include "filter.h"
#include "tellico_kernel.h"

#include <klocale.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>

#include <qlayout.h>
#include <qgroupbox.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qapplication.h>

using Tellico::FilterRuleWidget;
using Tellico::FilterRuleWidgetLister;
using Tellico::FilterDialog;

FilterRuleWidget::FilterRuleWidget(FilterRule* rule_, QWidget* parent_, const char* name_/*=0*/)
    : QHBox(parent_, name_), m_editRegExp(0), m_editRegExpDialog(0) {
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
    m_ruleFieldList.append('<' + i18n("Any Field") + '>');
    m_ruleFieldList += Kernel::self()->fieldTitles();
  }

  //---------- initialize list of filter operators
  if(m_ruleFuncList.isEmpty()) {
    // also see FilterRule::matches() and FilterRule::Function
    // if you change the following strings!
    m_ruleFuncList.append(i18n("contains"));
    m_ruleFuncList.append(i18n("does not contain"));
    m_ruleFuncList.append(i18n("equals"));
    m_ruleFuncList.append(i18n("does not equal"));
    m_ruleFuncList.append(i18n("matches regexp"));
    m_ruleFuncList.append(i18n("does not match regexp"));
  }
}

void FilterRuleWidget::initWidget() {
  setSpacing(4);

  m_ruleField = new KComboBox(this);
  m_ruleFunc = new KComboBox(this);
  m_ruleValue = new KLineEdit(this);

  if(!KTrader::self()->query(QString::fromLatin1("KRegExpEditor/KRegExpEditor")).isEmpty()) {
    m_editRegExp = new KPushButton(i18n("Edit..."), this);
    connect(m_editRegExp, SIGNAL(clicked()), this, SLOT(slotEditRegExp()));
    connect(m_ruleFunc, SIGNAL(activated(int)), this, SLOT(slotRuleFunctionChanged(int)));
    slotRuleFunctionChanged(m_ruleFunc->currentItem());
  }

  m_ruleField->insertStringList(m_ruleFieldList);
  // don't show sliders when popping up this menu
//  m_ruleField->setSizeLimit(m_ruleField->count());
//  m_ruleField->adjustSize();

  m_ruleFunc->insertStringList(m_ruleFuncList);
//  m_ruleFunc->adjustSize();

//  connect(m_ruleField, SIGNAL(textChanged(const QString &)),
//          this, SIGNAL(fieldChanged(const QString &)));
//  connect(m_ruleValue, SIGNAL(textChanged(const QString &)),
//          this, SIGNAL(contentsChanged(const QString &)));
}

void FilterRuleWidget::slotEditRegExp() {
  if(m_editRegExpDialog == 0) {
    m_editRegExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>(QString::fromLatin1("KRegExpEditor/KRegExpEditor"),
                                                                                    QString::null, this);
  }

  KRegExpEditorInterface* iface = static_cast<KRegExpEditorInterface *>(m_editRegExpDialog->qt_cast(QString::fromLatin1("KRegExpEditorInterface")));
  if(iface) {
    iface->setRegExp(m_ruleValue->text());
    if(m_editRegExpDialog->exec() == QDialog::Accepted) {
      m_ruleValue->setText(iface->regExp());
    }
  }
}

void FilterRuleWidget::slotRuleFunctionChanged(int which_) {
  // The 5th and 6th functions are for regexps
  m_editRegExp->setEnabled(which_ == 4 || which_ == 5);
}

void FilterRuleWidget::setRule(const FilterRule* rule_) {
  if(!rule_) {
    reset();
    return;
  }

  blockSignals(true);

  if(rule_->fieldName().isEmpty()) {
    m_ruleField->setCurrentItem(0); // "All Fields"
  } else {
    m_ruleField->setCurrentText(Kernel::self()->fieldTitleByName(rule_->fieldName()));
  }

  //--------------set function and contents
  m_ruleFunc->setCurrentItem(static_cast<int>(rule_->function()));
  m_ruleValue->setText(rule_->pattern());

  if(m_editRegExp) {
    slotRuleFunctionChanged(static_cast<int>(rule_->function()));
  }

  blockSignals(false);
}

Tellico::FilterRule* FilterRuleWidget::rule() const {
  QString field; // empty string
  if(m_ruleField->currentItem() > 0) { // 0 is "All Fields", field is empty
    field = Kernel::self()->fieldNameByTitle(m_ruleField->currentText());
  }

  return new FilterRule(field, m_ruleValue->text().stripWhiteSpace(),
                        static_cast<FilterRule::Function>(m_ruleFunc->currentItem()));
}

void FilterRuleWidget::reset() {
//  kdDebug() << "FilterRuleWidget::reset()" << endl;
  blockSignals(true);

  m_ruleField->setCurrentItem(0);
  m_ruleFunc->setCurrentItem(0);
  m_ruleValue->clear();

  if(m_editRegExp) {
    m_editRegExp->setEnabled(false);
  }

  blockSignals(false);
}


/***************************************************************/

namespace {
  static const int FILTER_MIN_RULE_WIDGETS = 2;
  static const int FILTER_MAX_RULES = 8;
}

FilterRuleWidgetLister::FilterRuleWidgetLister(QWidget* parent_, const char* name_)
    : KWidgetLister(FILTER_MIN_RULE_WIDGETS, FILTER_MAX_RULES, parent_, name_) {
//  slotClear();
}

void FilterRuleWidgetLister::setFilter(const Filter* filter_) {
//  if(mWidgetList.first()) { // move this below next 'if'?
//    mWidgetList.first()->blockSignals(true);
//  }

  if(filter_->isEmpty()) {
    slotClear();
//    mWidgetList.first()->blockSignals(false);
    return;
  }

  if(static_cast<int>(filter_->count()) > mMaxWidgets) {
    kdDebug() << "FilterRuleWidgetLister::setFilter() - more rules than allowed!" << endl;
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo(KMAX(static_cast<int>(filter_->count()), mMinWidgets));

  // load the actions into the widgets
  QPtrListIterator<QWidget> wIt(mWidgetList);
  for(QPtrListIterator<FilterRule> rIt(*filter_); rIt.current() && wIt.current(); ++rIt, ++wIt) {
    static_cast<FilterRuleWidget*>(*wIt)->setRule(*rIt);
  }
  for( ; wIt.current(); ++wIt) { // clear any remaining
    static_cast<FilterRuleWidget*>(*wIt)->reset();
  }

//  mWidgetList.first()->blockSignals(false);
}

void FilterRuleWidgetLister::reset() {
  slotClear();
}

QWidget* FilterRuleWidgetLister::createWidget(QWidget* parent_) {
  return new FilterRuleWidget(static_cast<Tellico::FilterRule*>(0), parent_);
}

void FilterRuleWidgetLister::clearWidget(QWidget* widget_) {
  if(widget_) {
    static_cast<FilterRuleWidget*>(widget_)->reset();
  }
}

const QPtrList<QWidget>& FilterRuleWidgetLister::widgetList() const {
  return mWidgetList;
}

/***************************************************************/

namespace {
  static const int FILTER_MIN_WIDTH = 600;
}

// make the Apply button the default, so the user can see if the filter is good
FilterDialog::FilterDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("Advanced Filter"), Help|Ok|Apply|Cancel, Ok, false) {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
  QVBoxLayout* topLayout = new QVBoxLayout(page, 0, KDialog::spacingHint());

  QGroupBox* m_matchGroup = new QGroupBox(1, Qt::Horizontal, i18n("Filter Criteria"), page);
  topLayout->addWidget(m_matchGroup);

  QVButtonGroup* bg = new QVButtonGroup(m_matchGroup);
  bg->setFrameShape(QFrame::NoFrame);
  bg->setInsideMargin(0);
  m_matchAll = new QRadioButton(i18n("Match a&ll of the following"), bg);
  m_matchAny = new QRadioButton(i18n("Match an&y of the following"), bg);
  m_matchAll->setChecked(true);

  m_ruleLister = new FilterRuleWidgetLister(m_matchGroup);
  connect(m_ruleLister, SIGNAL(widgetRemoved()), SLOT(slotShrink()));

  setMinimumWidth(KMAX(minimumWidth(), FILTER_MIN_WIDTH));
  setHelp(QString::fromLatin1("filter-dialog"));
}

void FilterDialog::setFilter(const Filter* filter_) {
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
}

void FilterDialog::slotOk() {
  slotApply();
  accept();
}

void FilterDialog::slotApply() {
  Tellico::Filter* filter;
  if(m_matchAny->isChecked()) {
    filter = new Filter(Filter::MatchAny);
  } else {
    filter = new Filter(Filter::MatchAll);
  }

  for(QPtrListIterator<QWidget> it(m_ruleLister->widgetList()); it.current(); ++it) {
    FilterRuleWidget* rw = static_cast<FilterRuleWidget*>(it.current());
    FilterRule* rule = rw->rule();
    if(rule && !rule->isEmpty()) {
      filter->append(rule);
    }
  }

  emit signalUpdateFilter(filter);
}

void FilterDialog::slotClear() {
//  kdDebug() << "FilterDialog::slotClear()" << endl;
  m_matchAll->setChecked(true);
  m_ruleLister->reset();
}

void FilterDialog::slotShrink() {
  updateGeometry();
  QApplication::sendPostedEvents();
  resize(width(), sizeHint().height());
}

#include "filterdialog.moc"
