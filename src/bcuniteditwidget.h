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

class BCTabControl;
class BookcaseDoc;

class QPushButton;

#include "bcattributewidget.h"
#include "bcunit.h" // needed for BCUnitList definition
#include "bcattribute.h"
#include "bcunitgroup.h" // needed for linking for some reason

#include <qdict.h>

//#define SHOW_COPY_BTN

/**
 * The BCUnitEditWidget classes allows for the editing of the unit information. It is a
 * control box, and translates the unit attribute types into combo boxes, checkboxes,
 * edit controls and so on.
 *
 * @author Robby Stephenson
 * @version $Id: bcuniteditwidget.h,v 1.4 2003/05/02 06:04:21 robby Exp $
 */
class BCUnitEditWidget : public QWidget {
Q_OBJECT

// needed for completion object support
friend class BCAttributeWidget;

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
//  void keyReleaseEvent(QKeyEvent* ev);

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
   * @param highlight An optional string to highlight
   */
  void slotSetContents(BCUnit* unit, const QString& highlight=QString::null);
  void slotSetContents(const BCUnitList& list);
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
  /**
   * Clears all of the input controls in the widget. The pointer to the
   * current unit is nullified, but not the pointer to the current collection.
   */
  void slotHandleClear();
  /**
   * This slot is called whenever anything is modified. It's public so I can call it
   * from a @ref BCAttributeEditWidget.
   */
  void slotSetModified(bool modified=true);
  void slotUpdateAttribute(BCCollection* coll, BCAttribute* att);

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
   * Handles clicking the Delete button. If the parent collection of the current unit
   * includes the unit object, then @ref signalDoUnitDelete is made. The widget is then
   * cleared.
   */
  void slotHandleDelete();

  void slotHandleReturn();

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
  BCUnitList m_currUnits;
  BCTabControl* m_tabs;
  QDict<BCAttributeWidget> m_widgetDict;
  QPushButton* m_new;
#ifdef SHOW_COPY_BTN
  QPushButton* m_copy;
#endif
  QPushButton* m_save;
  QPushButton* m_delete;
//  QPushButton* m_clear;

  bool m_modified;
  bool m_completionActivated;
  bool m_isOrphan;
};

#endif
