/***************************************************************************
                              bcfilterdialog.h
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

#ifndef BCFILTERDIALOG_H
#define BCFILTERDIALOG_H

class Bookcase;
class BCDetailedListView;
class BCFilter;
class BCFilterRule;
class BCFilterDialog; // forward declare for BCFilterRuleWidget

class KComboBox;
class KLineEdit;
class KPushButton;

class QGroupBox;
class QRadioButton;
class QDialog;

// kwidgetlister is copied from kdepim/libkdenetwork cvs
#include "kwidgetlister.h"

#include <kdialogbase.h>

#include <qhbox.h>
#include <qstring.h>
#include <qstringlist.h>

/**
 * A widget to edit a single BCFilterRule.
 * It consists of a read-only @ref KComboBox for the field,
 * a read-only @ref KComboBox for the function and
 * a @ref KLineEdit for the content or the pattern (in case of regexps).
 *
 * This class borrows heavily from KMSearchRule in kmail by Marc Mutz
 *
 * @author Robby Stephenson
 * @version $Id: bcfilterdialog.h,v 1.1 2003/05/02 06:04:21 robby Exp $
 */
class BCFilterRuleWidget : public QHBox {
Q_OBJECT

public:
  /**
   * Constructor. You give a @ref BCFilterRule as parameter, which will
   * be used to initialize the widget.
   */
  BCFilterRuleWidget(BCFilterRule* rule, QWidget* parent, const char* name=0);

  /**
   * Set the rule. The rule is accepted regardless of the return
   * value of @ref BCFilterRule::isEmpty. This widget makes a shallow
   * copy of @p rule and operates directly on it. If @p rule is
   * 0, the widget resets itself, takes user input, but does essentially
   * nothing. If you pass 0, you should probably disable it.
   */
  void setRule(const BCFilterRule* rule);
  /**
   * Return a reference to the currently worked-on @ref BCFilterRule.
   */
  BCFilterRule* rule() const;
  /**
   * Resets the rule currently worked on and updates the widget
   * accordingly.
   */
  void reset();

protected slots:
  void slotEditRegExp();
  void slotRuleFunctionChanged(int which);

private:
  void initLists();
  void initWidget();

  KComboBox* m_ruleAttribute;
  KComboBox* m_ruleFunc;
  KLineEdit* m_ruleValue;
  KPushButton* m_editRegExp;
  QDialog* m_editRegExpDialog;
  QStringList m_ruleAttributeList;
  QStringList m_ruleFuncList;
};

class BCFilterRuleWidgetLister : public KWidgetLister {
Q_OBJECT

public:
  BCFilterRuleWidgetLister(QWidget* parent=0, const char* name=0);

//  virtual ~KMSearchRuleWidgetLister();

  const QPtrList<QWidget>& widgetList() const;
  void setFilter(const BCFilter* filter);

public slots:
  void reset();

protected:
  virtual void clearWidget(QWidget* widget);
  virtual QWidget* createWidget(QWidget* parent);

private:
//  void regenerateRuleListFromWidgets();
//  QPtrList<BCFilterRule>* m_ruleList;
};

/**
 * @author Robby Stephenson
 * @version $Id: bcfilterdialog.h,v 1.1 2003/05/02 06:04:21 robby Exp $
 */
class BCFilterDialog : public KDialogBase  {
Q_OBJECT

public: 
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget, a Bookcase object
   * @param name The widget name
   */
  BCFilterDialog(BCDetailedListView* view, Bookcase* parent, const char* name=0);

  void setFilter(const BCFilter* filter);
  
  // These are just wrappers so that BCFilterRuleWidget doesn't have to know
  // anything about collections or documents
  QStringList attributeTitles() const;
  QString attributeNameByTitle(const QString& title) const;
  QString attributeTitleByName(const QString& name) const;

public slots:
  void slotClear();

protected slots:
  virtual void slotOk();
  virtual void slotApply();
  void slotShrink();

signals:
  void filterApplied();

private:
  Bookcase* m_bookcase;
  BCDetailedListView* m_view;

  QGroupBox* m_matchGroup;
  QRadioButton* m_matchAll;
  QRadioButton* m_matchAny;
  BCFilterRuleWidgetLister* m_ruleLister;
};

#endif
