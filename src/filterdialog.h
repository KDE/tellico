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

#ifndef FILTERDIALOG_H
#define FILTERDIALOG_H

class KComboBox;
class KLineEdit;
class KPushButton;

class QRadioButton;
class QDialog;

// kwidgetlister is copied from kdepim/libkdenetwork cvs
#include "kwidgetlister.h"

#include <kdialogbase.h>

#include <qhbox.h>
#include <qstring.h>
#include <qstringlist.h>

namespace Bookcase {
  class MainWindow;
  class Filter;
  class FilterRule;
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
 * @version $Id: filterdialog.h 578 2004-03-27 01:28:16Z robby $
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
  void setFilter(const Filter* filter);

public slots:
  void reset();

protected:
  virtual void clearWidget(QWidget* widget);
  virtual QWidget* createWidget(QWidget* parent);
};

/**
 * @author Robby Stephenson
 * @version $Id: filterdialog.h 578 2004-03-27 01:28:16Z robby $
 */
class FilterDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  FilterDialog(MainWindow* parent, const char* name=0);

  void setFilter(const Filter* filter);

  // These are just wrappers so that FilterRuleWidget doesn't have to know
  // anything about collections or documents
  QStringList fieldTitles() const;
  QString fieldNameByTitle(const QString& title) const;
  QString fieldTitleByName(const QString& name) const;

public slots:
  void slotClear();

protected slots:
  virtual void slotOk();
  virtual void slotApply();
  void slotShrink();

signals:
  void signalUpdateFilter(Bookcase::Filter*);

private:
  MainWindow* m_bookcase;

  QRadioButton* m_matchAll;
  QRadioButton* m_matchAny;
  FilterRuleWidgetLister* m_ruleLister;
};

} // end namespace
#endif
