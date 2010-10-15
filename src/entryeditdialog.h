/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_ENTRYEDITDIALOG_H
#define TELLICO_ENTRYEDITDIALOG_H

#include "observer.h"
#include "gui/fieldwidget.h"

#include <kdialog.h>

#include <QHash>

class KPushButton;

namespace Tellico {
  namespace GUI {
    class TabWidget;
  }

/**
 * @author Robby Stephenson
 */
class EntryEditDialog : public KDialog, public Observer {
Q_OBJECT

// needed for completion object support
friend class GUI::FieldWidget;

public:
  EntryEditDialog(QWidget* parent);

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
  void setLayout(Data::CollPtr coll);
  /**
   * Sets the contents of the input controls to match the contents of a list of entries.
   *
   * @param list A list of the entries. The data in the first one will be inserted in the controls, and
   * the widgets will be enabled or not, depending on whether the rest of the entries match the first one.
   */
  void setContents(Data::EntryList entries);
  /**
   * Clears all of the input controls in the widget. The pointer to the
   * current entry is nullified, but not the pointer to the current collection.
   */
  void clear();

  virtual void    addEntries(Data::EntryList entries);
  virtual void modifyEntries(Data::EntryList entries);

  virtual void    addField(Data::CollPtr coll, Data::FieldPtr field);
  /**
   * Updates a widget when its field has been modified. The category may have changed, completions may have
   * been added or removed, or what-have-you.
   *
   * @param coll A pointer to the parent collection
   * @param oldField A pointer to the old field, which should have the same name as the new one
   * @param newField A pointer to the new field
   */
  virtual void modifyField(Data::CollPtr coll, Data::FieldPtr oldField, Data::FieldPtr newField);
  /**
   * Removes a field from the editor.
   *
   * @param field The field to be removed
   */
  virtual void removeField(Data::CollPtr, Data::FieldPtr field);

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

private slots:
  void fieldValueChanged(Tellico::Data::FieldPtr field);

private:
  /**
   * Sets the contents of the input controls to match the contents of a entry.
   *
   * @param entry A pointer to the entry
   * @param highlight An optional string to highlight
   */
  void setContents(Data::EntryPtr entry);
  /**
   * Updates the completion objects in the edit boxes to include values
   * contained in a certain entry.
   *
   * @param entry A pointer to the entry
   */
  void updateCompletions(Data::EntryPtr entry);

  Data::CollPtr m_currColl;
  Data::EntryList m_currEntries;
  GUI::TabWidget* m_tabs;
  QHash<QString, GUI::FieldWidget*> m_widgetDict;

  ButtonCode m_saveBtn;
  ButtonCode m_newBtn;

  bool m_modified;
  Data::FieldList m_modifiedFields;
  bool m_isOrphan;
  bool m_isWorking;
  bool m_needReset;
};

} // end namespace
#endif
