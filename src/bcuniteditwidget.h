/***************************************************************************
                             bcuniteditwidget.h
                             -------------------
    begin                : Wed Sep 26 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BCUNITEDITWIDGET_H
#define BCUNITEDITWIDGET_H

class BCUnit;
class BCCollection;
class BCTabControl;
class BookcaseDoc;

class QListViewItem;
class QPushButton;

#include <klineedit.h>
#include <kcombobox.h>

#include <qtextedit.h>
#include <qcheckbox.h>
#include <qdict.h>

/**
 * The BCUnitEditWidget classes allows for the editing of the unit information. It is a
 * control box, and translates the unit attribute types into combo boxes, checkboxes,
 * edit controls and so on.
 *
 * @author Robby Stephenson
 * @version $Id: bcuniteditwidget.h,v 1.29 2003/03/10 02:50:52 robby Exp $
 */
class BCUnitEditWidget : public QWidget  {
Q_OBJECT

public: 
  /**
   * The constructor doesn't do much, except added a tabbed control. Most of the layout
   * happens in @ref slotSetLayout().
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCUnitEditWidget(QWidget* parent, const char* name=0);

  bool queryModified();

public slots:
  void slotSetLayout(BCCollection* coll);
  void slotSetCollection(BCCollection* coll);
  /**
   * Resets the widget, deleting all of its contents
   */
  void slotReset();
  /**
   * Sets the contents of the input controls to match the contents of a unit.
   *
   * @param unit A pointer to the unit
   */
  void slotSetContents(BCUnit* unit);
  /**
   * Updates the completion objects in the edit boxes to include values
   * contained in a certain unit.
   *
   * @param unit A pointer to the unit
   */
  void slotUpdateCompletions(BCUnit* unit);
  /**
   * Handles clicking the Save button. All the values in the entry widgets are
   * copied into the unit object. @ref signalDoUnitSave is made. The widget is cleared,
   * and the first tab is shown.
   */
  void slotHandleSave();

protected:
  /**
   * Used to nullify all pointers, delete the child objects, and fully reset the
   * data contained in the class.
   */
//  void clearWidgets();

protected slots:
  /**
   * Handles clicking the New button. The old unit pointer is destroyed and a
   * new one is created, but not added to any collection.
   */
  void slotHandleNew();
  /**
   * Handles clicking the Copy button. A copy of the current unit is made and saved
   * to the parent collection.
   */
  void slotHandleCopy();
  /**
   * Clears all of the input controls in the widget. The pointer to the
   * current unit is nullified, but not the pointer to the current collection.
   */
  void slotHandleClear();
  /**
   * Handles clicking the Delete button. If the parent collection of the current unit
   * includes the unit object, then @ref signalDoUnitDelete is made. The widget is then
   * cleared.
   */
  void slotHandleDelete();
  /**
   * Switches the focus to the first text box in the visible widget page.
   *
   * @param tabNum The number of the page shown
   */
  void slotSwitchFocus(int tabNum);
  /**
   * This slot is called whenever anything is modified.
   */
  void slotSetModified();

signals:
  /**
   * Signals a desire on the part of the user to save a unit to a collection.
   *
   * @param unit A pointer to the unit to be saved
   */
  void signalSaveUnit(BCUnit* unit);
  /**
   * Signals a desire on the part of the user to delete a unit from a collection.
   *
   * @param unit A pointer to the unit to be deleted
   */
  void signalDeleteUnit(BCUnit* unit);

private:
  BCCollection* m_currColl;
  BCUnit* m_currUnit;
  BCTabControl* m_tabs;
  QDict<KLineEdit> m_editDict;
  QDict<QTextEdit> m_multiDict;
  QDict<KComboBox> m_comboDict;
  QDict<QCheckBox> m_checkDict;
  QPushButton* m_new;
  QPushButton* m_copy;
  QPushButton* m_save;
  QPushButton* m_delete;
  QPushButton* m_clear;

  bool m_modified;
};

#endif
