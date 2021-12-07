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

#ifndef FILTERRULEWIDGET_H
#define FILTERRULEWIDGET_H

#include <QWidget>

#include "../datavectors.h"

class KComboBox;
class KLineEdit;
class KDateComboBox;

class QRadioButton;
class QDialog;
class QStackedWidget;
class QLineEdit;
class QPushButton;

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
class FilterRuleWidget : public QWidget {
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

Q_SIGNALS:
  void signalModified();

public Q_SLOTS:
  void setFocus();

protected Q_SLOTS:
  void slotEditRegExp();
  void slotRuleFieldChanged(int which);
  void slotRuleFunctionChanged(int which);

private:
  static QString anyFieldString();
  void initLists();
  void initWidget();
  void updateFunctionList();

  KComboBox* m_ruleField;
  GUI::ComboBox* m_ruleFunc;
  QStackedWidget* m_valueStack;
  KLineEdit* m_ruleValue;
  KDateComboBox* m_ruleDate;
  QPushButton* m_editRegExp;
  QDialog* m_editRegExpDialog;
  QStringList m_ruleFieldList;
  enum RuleType {
    General,
    Date,
    Number,
    Image,
    Bool
  };
  RuleType m_ruleType;
};

} // end namespace
#endif
