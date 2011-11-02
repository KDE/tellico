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

#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

// kwidgetlister is copied from kdepim/libkdenetwork cvs
#include "gui/kwidgetlister.h"
#include "filter.h"
#include "datavectors.h"

#include <kdialog.h>
#include <KHBox>

#include <QString>
#include <QStringList>

class KComboBox;
class KLineEdit;
class KPushButton;
class KDateComboBox;

class QRadioButton;
class QDialog;
class QStackedWidget;

namespace Tellico {
  namespace GUI {
    class ComboBox;
  }
  class FilterDialog;

/**
 * A widget to edit a single FilterRule.
 * It consists of a read-only @ref KComboBox for the field,
 * a read-only @ref KComboBox for the function and
 * a @ref KLineEdit for the content or the pattern (in case of regexps).
 *
 * This class borrows heavily from KMSearchRule in kmail by Marc Mutz
 *
 * @author Robby Stephenson
 */
class FilterRuleWidget : public KHBox {
Q_OBJECT

public:
  /**
   * Constructor. You give a @ref FilterRule as parameter, which will
   * be used to initialize the widget.
   */
  FilterRuleWidget(FilterRule* rule, QWidget* parent);

  /**
   * Set the rule. The rule is accepted regardless of the return
   * value of @ref FilterRule::isEmpty. This widget makes a shallow
   * copy of @p rule and operates directly on it. If @p rule is
   * 0, the widget resets itself, takes user input, but does essentially
   * nothing. If you pass 0, you should probably disable it.
   */
  void setRule(const FilterRule* rule);
  /**
   * Return a reference to the currently worked-on @ref FilterRule.
   */
  FilterRule* rule() const;
  /**
   * Resets the rule currently worked on and updates the widget accordingly.
   */
  void reset();

signals:
  void signalModified();

public slots:
  void setFocus();

protected slots:
  void slotEditRegExp();
  void slotRuleFieldChanged(int which);
  void slotRuleFunctionChanged(int which);

private:
  void initLists();
  void initWidget();
  void updateFunctionList();

  KComboBox* m_ruleField;
  GUI::ComboBox* m_ruleFunc;
  QStackedWidget* m_valueStack;
  KLineEdit* m_ruleValue;
  KDateComboBox* m_ruleDate;
  KPushButton* m_editRegExp;
  QDialog* m_editRegExpDialog;  //krazy:exclude=qclasses
  QStringList m_ruleFieldList;
  bool m_isDate;
};

class FilterRuleWidgetLister : public KWidgetLister {
Q_OBJECT

public:
  FilterRuleWidgetLister(QWidget* parent);

  QList<QWidget*> widgetList() const;
  void setFilter(FilterPtr filter);

public slots:
  void reset();
  virtual void setFocus();

signals:
  void signalModified();

protected:
  virtual void clearWidget(QWidget* widget);
  virtual QWidget* createWidget(QWidget* parent);
};

/**
 * @author Robby Stephenson
 */
class FilterDialog : public KDialog {
Q_OBJECT

public:
  enum Mode {
    CreateFilter,
    ModifyFilter
  };

  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   */
  FilterDialog(Mode mode, QWidget* parent);

  FilterPtr currentFilter(bool alwaysCreateNew=false);
  void setFilter(FilterPtr filter);

public slots:
  void slotClear();

protected slots:
  virtual void slotOk();
  virtual void slotApply();
  void slotShrink();
  void slotFilterChanged();
  void slotSaveFilter();

signals:
  void signalUpdateFilter(Tellico::FilterPtr);
  void signalCollectionModified();

private:
  void init();

  FilterPtr m_filter;
  const Mode m_mode;
  QRadioButton* m_matchAll;
  QRadioButton* m_matchAny;
  FilterRuleWidgetLister* m_ruleLister;
  KLineEdit* m_filterName;
  KPushButton* m_saveFilter;
};

} // end namespace
#endif
