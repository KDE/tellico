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

#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

class KComboBox;
class KLineEdit;
class KPushButton;

class QRadioButton;
class QDialog;

// kwidgetlister is copied from kdepim/libkdenetwork cvs
#include "gui/kwidgetlister.h"
#include "filter.h"
#include "datavectors.h"

#include <kdialogbase.h>

#include <qhbox.h>
#include <qstring.h>
#include <qstringlist.h>

namespace Tellico {
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
class FilterRuleWidget : public QHBox {
Q_OBJECT

public:
  /**
   * Constructor. You give a @ref FilterRule as parameter, which will
   * be used to initialize the widget.
   */
  FilterRuleWidget(FilterRule* rule, QWidget* parent, const char* name=0);

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
  void slotRuleFunctionChanged(int which);

private:
  void initLists();
  void initWidget();

  KComboBox* m_ruleField;
  KComboBox* m_ruleFunc;
  KLineEdit* m_ruleValue;
  KPushButton* m_editRegExp;
  QDialog* m_editRegExpDialog;
  QStringList m_ruleFieldList;
  QStringList m_ruleFuncList;
};

class FilterRuleWidgetLister : public KWidgetLister {
Q_OBJECT

public:
  FilterRuleWidgetLister(QWidget* parent, const char* name=0);

  const QPtrList<QWidget>& widgetList() const;
  void setFilter(Filter::Ptr filter);

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
class FilterDialog : public KDialogBase {
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
   * @param name The widget name
   */
  FilterDialog(Mode mode, QWidget* parent, const char* name=0);

  FilterPtr currentFilter();
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
