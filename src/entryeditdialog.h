/***************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef ENTRYEDITDIALOG_H
#define ENTRYEDITDIALOG_H

class QPushButton;

#include "fieldwidget.h"
#include "entry.h" // needed for EntryList definition
#include "field.h"

#include <kdialogbase.h>

#include <qdict.h>

namespace Bookcase {
  class TabControl;

/**
 * @author Robby Stephenson
 * @version $Id: entryeditdialog.h 622 2004-04-22 05:17:09Z robby $
 */
class EntryEditDialog : public KDialogBase {
Q_OBJECT

// needed for completion object support
friend class FieldWidget;

public:
  EntryEditDialog(QWidget* parent, const char* name);

  /**
   * Checks to see if any data needs to be saved. Returns @p true if it's ok to continue with
   * saving or closing the widget.
   *
   * @return Returns @p true if either the data has not been modified or the user to save or discard the new data.
   */
  bool queryModified();
  /**
   * Deletes and resets the layout of the tabs.
   *
   * @param coll A pointer to the collection whose fields should be used for setting up the layout
   */
  void setLayout(Data::Collection* coll);
  /**
   * Sets the contents of the input controls to match the contents of a entry.
   *
   * @param entry A pointer to the entry
   * @param highlight An optional string to highlight
   */
  void setContents(Data::Entry* entry, const QString& highlight=QString::null);
  /**
   * Sets the contents of the input controls to match the contents of a list of entries.
   *
   * @param list A list of the entries. The data in the first one will be inserted in the controls, and
   * the widgets will be enabled or not, depending on whether the rest of the entries match the first one.
   */
  void setContents(const Data::EntryList& list);
  /**
   * Clears all of the input controls in the widget. The pointer to the
   * current entry is nullified, but not the pointer to the current collection.
   */
  void clear();
  /**
   * Removes a field from the editor.
   *
   * @param field The field to be removed
   */
  void removeField(Data::Field* field);

public slots:
  /**
   * Called when the Close button is clicked. It just hides the dialog.
   */
  virtual void slotClose();
  /**
   * Resets the widget, deleting all of its contents
   */
  void slotReset();
  /**
   * Updates the completion objects in the edit boxes to include values
   * contained in a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void slotUpdateCompletions(Bookcase::Data::Entry* entry);
  /**
   * Handles clicking the New button. The old entry pointer is destroyed and a
   * new one is created, but not added to any collection.
   */
  void slotHandleNew();
  /**
   * Handles clicking the Save button. All the values in the entry widgets are
   * copied into the entry object. @ref signalSaveEntry is made. The widget is cleared,
   * and the first tab is shown.
   */
  void slotHandleSave();
  /**
   * This slot is called whenever anything is modified. It's public so I can call it
   * from a @ref FieldEditWidget.
   */
  void slotSetModified(bool modified=true);
  /**
   * Updates a widget when its field has been modified. The category may have changed, completions may have
   * been added or removed, or what-have-you.
   *
   * @param coll A pointer to the parent collection
   * @param newField A pointer to the new field
   * @param oldField A pointer to the old field, which should have the same name as the new one
   */
  void slotUpdateField(Bookcase::Data::Collection* coll, Bookcase::Data::Field* newField, Bookcase::Data::Field* oldField);

signals:
  /**
   * Signals a desire on the part of the user to save a entry to a collection.
   *
   * @param entry A pointer to the entry to be saved
   */
  void signalSaveEntries(const Bookcase::Data::EntryList& entryList);
  /**
   * Signals a desire to clear the global selection
   *
   * @param widget The widget signalling
   */
  void signalClearSelection(QWidget* widget);

private:
  Data::Collection* m_currColl;
  Data::EntryList m_currEntries;
  TabControl* m_tabs;
  QDict<FieldWidget> m_widgetDict;
  QPushButton* m_new;
  QPushButton* m_save;

  bool m_modified;
  bool m_isOrphan;
  bool m_isSaving;
};

} // end namespace
#endif
