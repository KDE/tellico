/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
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

#include "observer.h"
#include "datavectors.h"
#include "gui/fieldwidget.h"

#include <kdialogbase.h>

#include <qdict.h>

namespace Tellico {
  namespace GUI {
    class TabControl;
    class FieldWidget;
  }

/**
 * @author Robby Stephenson
 */
class EntryEditDialog : public KDialogBase, public Observer {
Q_OBJECT

// needed for completion object support
friend class GUI::FieldWidget;

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
   * Sets the contents of the input controls to match the contents of a list of entries.
   *
   * @param list A list of the entries. The data in the first one will be inserted in the controls, and
   * the widgets will be enabled or not, depending on whether the rest of the entries match the first one.
   */
  void setContents(Data::EntryVec entries);
  /**
   * Clears all of the input controls in the widget. The pointer to the
   * current entry is nullified, but not the pointer to the current collection.
   */
  void clear();

  virtual void    addEntry(Data::Entry* entry) { updateCompletions(entry); }
  virtual void modifyEntry(Data::Entry* entry) { updateCompletions(entry); }

  virtual void    addField(Data::Collection* coll, Data::Field*) { setLayout(coll); }
  /**
   * Updates a widget when its field has been modified. The category may have changed, completions may have
   * been added or removed, or what-have-you.
   *
   * @param coll A pointer to the parent collection
   * @param oldField A pointer to the old field, which should have the same name as the new one
   * @param newField A pointer to the new field
   */
  virtual void modifyField(Data::Collection* coll, Data::Field* oldField, Data::Field* newField);
  /**
   * Removes a field from the editor.
   *
   * @param field The field to be removed
   */
  virtual void removeField(Data::Collection*, Data::Field* field);

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

private:
  /**
   * Sets the contents of the input controls to match the contents of a entry.
   *
   * @param entry A pointer to the entry
   * @param highlight An optional string to highlight
   */
  void setContents(Data::Entry* entry);
  /**
   * Updates the completion objects in the edit boxes to include values
   * contained in a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void updateCompletions(Data::Entry* entry);

  KSharedPtr<Data::Collection> m_currColl;
  Data::EntryVec m_currEntries;
  GUI::TabControl* m_tabs;
  QDict<GUI::FieldWidget> m_widgetDict;
  QPushButton* m_new;
  QPushButton* m_save;

  bool m_modified;
  bool m_isOrphan;
  bool m_isWorking;
};

} // end namespace
#endif
