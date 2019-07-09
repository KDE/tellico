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

#include "filterrulewidgetlister.h"
#include "filterrulewidget.h"
#include "../filter.h"
#include "../tellico_debug.h"

using Tellico::FilterRuleWidgetLister;

namespace {
  static const int FILTER_MIN_RULE_WIDGETS = 1;
  static const int FILTER_MAX_RULES = 8;
}

FilterRuleWidgetLister::FilterRuleWidgetLister(QWidget* parent_)
    : KWidgetLister(FILTER_MIN_RULE_WIDGETS, FILTER_MAX_RULES, parent_) {
//  slotClear();
  connect(this, &KWidgetLister::clearWidgets, this, &FilterRuleWidgetLister::signalModified);
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
  FilterRuleWidget* w = new FilterRuleWidget(static_cast<Tellico::FilterRule*>(nullptr), parent_);
  connect(w, &FilterRuleWidget::signalModified, this, &FilterRuleWidgetLister::signalModified);
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
