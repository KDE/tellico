/***************************************************************************
                           bookcasecontroller.cpp
                             -------------------
    begin                : Thu May 29 2003
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

#ifndef BOOKCASECONTROLLER_H
#define BOOKCASECONTROLLER_H

class Bookcase;
class BCGroupView;
class BCDetailedListView;
class BCUnitEditWidget;
class BCCollection;
class BCAttribute;

#include <qobject.h>

#include "bcunit.h" // needed for BCUnitList

/**
 * @author Robby Stephenson
 * @version $Id: bookcasecontroller.h 223 2003-10-24 14:07:32Z robby $
 */
class BookcaseController : public QObject {
Q_OBJECT

public:
  BookcaseController(Bookcase* parent, const char* name);

  void setWidgets(BCGroupView* groupView, BCDetailedListView* detailedView,
                  BCUnitEditWidget* editWidget);

public slots:
  /**
   * When a collection is added to the document, certain actions need to be taken
   * by the parent app. The collection toolbar is updated, the unit count is set, and
   * the collection's modified signal is connected to the @ref BCGroupView widget.
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionAdded(BCCollection* coll);
  /**
   * Removes a collection from all the widgets
   *
   * @param coll A pointer to the collection being added
   */
  void slotCollectionDeleted(BCCollection* coll);
  void slotCollectionRenamed(const QString& name);
  void slotUnitAdded(BCUnit* unit);
  void slotUnitModified(BCUnit* unit);
  void slotUnitDeleted(BCUnit* unit);
  void slotAttributeAdded(BCCollection* coll, BCAttribute* att);
  void slotAttributeDeleted(BCCollection* coll, BCAttribute* att);
  void slotAttributeModified(BCCollection* coll, BCAttribute* oldAtt, BCAttribute* newAtt);
  void slotAttributesReordered(BCCollection* coll);
  /**
   * Updates the widgets when a unit(s) is selected.
   *
   * param widget A pointer to the widget where the units were selected
   * @param list The list of selected units
   */
  void slotUpdateSelection(QWidget* widget, const BCUnitList& list=BCUnitList());
  void slotUpdateSelection(BCUnit* unit, const QString& highlight);
  void slotDeleteSelectedUnits();

private:
  Bookcase* m_bookcase;
  BCGroupView*  m_groupView;
  BCDetailedListView* m_detailedView;
  BCUnitEditWidget* m_editWidget;

  /**
   * Keep track of the selected units so that a top-level delete has something for reference
   */
  BCUnitList m_selectedUnits;
};

#endif
