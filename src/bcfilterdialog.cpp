/***************************************************************************
                             bcfilterdialog.cpp
                             -------------------
    begin                : Fri Apr 11 2003
    copyright            : (C) 2003 by Robby Stephenson
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

#include "bcfilterdialog.h"
#include "bookcase.h"
#include "bookcasedoc.h"
#include "bccollection.h"
#include "bcfilter.h"
#include "bcdetailedlistview.h"
#include "bcutils.h"

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
#include <qbuttongroup.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>

BCFilterRuleWidget::BCFilterRuleWidget(BCFilterRule* rule_,
                                       QWidget* parent_, const char* name_/*=0*/)
    : QHBox(parent_, name_), m_editRegExpDialog(0) {
  initLists();
  initWidget();

  if(rule_) {
    setRule(rule_);
  } else {
    reset();
  }
}

void BCFilterRuleWidget::initLists() {
  //---------- initialize list of filter fields
  if(m_ruleAttributeList.isEmpty()) {
    m_ruleAttributeList.append(i18n("Any Field"));
    BCFilterDialog* dlg = BCFilterDialogAncestor(parent());
    m_ruleAttributeList += dlg->attributeTitles();
  }

  //---------- initialize list of filter operators
  if(m_ruleFuncList.isEmpty()) {
    // also see BCFilterRule::matches() and BCFilterRule::Function
    // if you change the following strings!
    m_ruleFuncList.append(i18n("contains"));
    m_ruleFuncList.append(i18n("does not contain"));
    m_ruleFuncList.append(i18n("equals"));
    m_ruleFuncList.append(i18n("does not equal"));
    m_ruleFuncList.append(i18n("matches regexp"));
    m_ruleFuncList.append(i18n("does not match regexp"));
  }
}

void BCFilterRuleWidget::initWidget() {
  setSpacing(4);

  m_ruleAttribute = new KComboBox(this);
  m_ruleFunc = new KComboBox(this);
  m_ruleValue = new KLineEdit(this);

  if(!KTrader::self()->query(QString::fromLatin1("KRegExpEditor/KRegExpEditor")).isEmpty()) {
    m_editRegExp = new KPushButton(i18n("Edit..."), this);
    connect(m_editRegExp, SIGNAL(clicked()), this, SLOT(slotEditRegExp()));
    connect(m_ruleFunc, SIGNAL(activated(int)), this, SLOT(slotRuleFunctionChanged(int)));
    slotRuleFunctionChanged(m_ruleFunc->currentItem());
  }

  m_ruleAttribute->insertStringList(m_ruleAttributeList);
  // don't show sliders when popping up this menu
//  m_ruleAttribute->setSizeLimit(m_ruleAttribute->count());
  m_ruleAttribute->adjustSize();
  
  m_ruleFunc->insertStringList(m_ruleFuncList);
  m_ruleFunc->adjustSize();

//  connect(m_ruleAttribute, SIGNAL(textChanged(const QString &)),
//          this, SIGNAL(fieldChanged(const QString &)));
//  connect(m_ruleValue, SIGNAL(textChanged(const QString &)),
//          this, SIGNAL(contentsChanged(const QString &)));
}

void BCFilterRuleWidget::slotEditRegExp() {
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

void BCFilterRuleWidget::slotRuleFunctionChanged(int which_) {
  // The 5th and 6th functions are for regexps
  m_editRegExp->setEnabled(which_ == 4 || which_ == 5);
}

void BCFilterRuleWidget::setRule(const BCFilterRule* rule_) {
  if(!rule_) {
    reset();
    return;
  }

  blockSignals(true);

  BCFilterDialog* dlg = BCFilterDialogAncestor(parent());

  if(rule_->attribute().isEmpty()) {
    m_ruleAttribute->setCurrentItem(0); // "All Fields"
  } else {
    m_ruleAttribute->setCurrentText(dlg->attributeTitleByName(rule_->attribute()));
  }
  
  //--------------set function and contents
  m_ruleFunc->setCurrentItem(static_cast<int>(rule_->function()));
  m_ruleValue->setText(rule_->pattern());

  if(m_editRegExp) {
    slotRuleFunctionChanged(static_cast<int>(rule_->function()));
  }

  blockSignals(false);
}

BCFilterRule* BCFilterRuleWidget::rule() const {
  QString att;
  if(m_ruleAttribute->currentItem() > 0) { // 0 is "All Fields", attribute is empty
    BCFilterDialog* dlg = BCFilterDialogAncestor(parent());
    att = dlg->attributeNameByTitle(m_ruleAttribute->currentText());
  }

  BCFilterRule* r = new BCFilterRule(att, m_ruleValue->text().stripWhiteSpace(),
                                     static_cast<BCFilterRule::Function>(m_ruleFunc->currentItem()));

  return r;
}

void BCFilterRuleWidget::reset() {
//  kdDebug() << "BCFilterRuleWidget::reset()" << endl;
  blockSignals(true);

  m_ruleAttribute->setCurrentItem(0);
  m_ruleFunc->setCurrentItem(0);
  m_ruleValue->clear();
  
  if(m_editRegExp) {
    m_editRegExp->setEnabled(false);
  }

  blockSignals(false);
}


/***************************************************************/

static const int FILTER_MIN_RULE_WIDGETS = 2;

BCFilterRuleWidgetLister::BCFilterRuleWidgetLister(QWidget* parent_, const char* name_)
    : KWidgetLister(FILTER_MIN_RULE_WIDGETS, FILTER_MAX_RULES, parent_, name_) {
//  slotClear();
}

void BCFilterRuleWidgetLister::setFilter(const BCFilter* filter_) {
  if(mWidgetList.first()) { // move this below next 'if'?
    mWidgetList.first()->blockSignals(true);
  }

  if(filter_->isEmpty()) {
    slotClear();
    mWidgetList.first()->blockSignals(false);
    return;
  }

  if(filter_->count() > mMaxWidgets) {
    kdDebug() << "BCFilterRuleWidgetLister::setFilter() - more rules than allowed!" << endl;
  }

  // set the right number of widgets
  setNumberOfShownWidgetsTo(QMAX(filter_->count(), mMinWidgets));

  // load the actions into the widgets
  QPtrListIterator<BCFilterRule> rIt(*filter_);
  QPtrListIterator<QWidget> wIt(mWidgetList);
  for( ; rIt.current() && wIt.current(); ++rIt, ++wIt) {
    static_cast<BCFilterRuleWidget*>(*wIt)->setRule(*rIt);
  }
  for( ; wIt.current(); ++wIt) { // clear any remaining
    static_cast<BCFilterRuleWidget*>(*wIt)->reset();
  }

  mWidgetList.first()->blockSignals(false);
}

void BCFilterRuleWidgetLister::reset() {
  slotClear();
}

QWidget* BCFilterRuleWidgetLister::createWidget(QWidget* parent_) {
  return new BCFilterRuleWidget(static_cast<BCFilterRule*>(0), parent_);
}

void BCFilterRuleWidgetLister::clearWidget(QWidget* widget_) {
  if(widget_) {
    static_cast<BCFilterRuleWidget*>(widget_)->reset();
  }
}

const QPtrList<QWidget>& BCFilterRuleWidgetLister::widgetList() const {
  return mWidgetList;
}

/***************************************************************/

static const int FILTER_MIN_WIDTH = 600;

// make the Apply button the default, so the user can see if the filter is good
BCFilterDialog::BCFilterDialog(BCDetailedListView* view_, Bookcase* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("Advanced Filter"), Ok|Apply|Cancel, Ok,
                  false), m_bookcase(parent_), m_view(view_) {
  QWidget* page = new QWidget(this);
  setMainWidget(page);
  QVBoxLayout* topLayout = new QVBoxLayout(page, 0, KDialog::spacingHint());

  m_matchGroup = new QGroupBox(1, Qt::Horizontal, i18n("Filter Criteria"), page);
  topLayout->addWidget(m_matchGroup);

  m_matchAll = new QRadioButton(i18n("Match a&ll of the following"), m_matchGroup);
  m_matchAny = new QRadioButton(i18n("Match an&y of the following"), m_matchGroup);

  m_matchAll->setChecked(true);
  m_matchAny->setChecked(false);

  QButtonGroup* bg = new QButtonGroup(m_matchGroup);
  bg->hide();
  bg->insert(m_matchAll, static_cast<int>(BCFilter::MatchAll));
  bg->insert(m_matchAny, static_cast<int>(BCFilter::MatchAny));

  m_ruleLister = new BCFilterRuleWidgetLister(m_matchGroup);
  connect(m_ruleLister, SIGNAL(widgetRemoved()), SLOT(slotShrink()));

  setMinimumWidth(FILTER_MIN_WIDTH);
}

void BCFilterDialog::setFilter(const BCFilter* filter_) {
  if(!filter_) {
    slotClear();
    return;
  }

  if(filter_->op() == BCFilter::MatchAll) {
    m_matchAll->setChecked(true);
  } else {
    m_matchAny->setChecked(true);
  }

  m_ruleLister->setFilter(filter_);
}

void BCFilterDialog::slotOk() {
  slotApply();
  accept();
}

void BCFilterDialog::slotApply() {
  BCFilter* filter;
  if(m_matchAny->isChecked()) {
    filter = new BCFilter(BCFilter::MatchAny);
  } else {
    filter = new BCFilter(BCFilter::MatchAll);
  }
  
  BCFilterRuleWidget* rw;
  QPtrListIterator<QWidget> it(m_ruleLister->widgetList());
  for( ; it.current(); ++it) {
    rw = static_cast<BCFilterRuleWidget*>(it.current());
    BCFilterRule* rule = rw->rule();
    if(rule && !rule->isEmpty()) {
      filter->append(rule);
    }
  }
  // TODO: the signal is emitted first because the Bookcase app clears
  // the quick filter on this signal
  // clearing it emits the textChanged signal which in turns also
  // applies another filter. Should be fixed someday
  emit filterApplied();

  // the view takes over ownership of the filter
  m_view->setFilter(filter);
}

void BCFilterDialog::slotClear() {
//  kdDebug() << "BCFilterDialog::slotClear()" << endl;
  m_matchAll->setChecked(true);
  m_ruleLister->reset();
}

void BCFilterDialog::slotShrink() {
  updateGeometry();
  QApplication::sendPostedEvents();
  resize(width(), sizeHint().height());
}

// TODO: fix when multiple collections supported
QStringList BCFilterDialog::attributeTitles() const {
  return m_bookcase->doc()->collectionById(0)->attributeTitles();
}

QString BCFilterDialog::attributeNameByTitle(const QString& title_) const {
  return m_bookcase->doc()->attributeNameByTitle(title_);
}

QString BCFilterDialog::attributeTitleByName(const QString& name_) const {
  return m_bookcase->doc()->attributeTitleByName(name_);
}
