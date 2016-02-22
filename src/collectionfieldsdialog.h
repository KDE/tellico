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

#ifndef TELLICO_COLLECTIONFIELDSDIALOG_H
#define TELLICO_COLLECTIONFIELDSDIALOG_H

#include "datavectors.h"

#include <KDialog>

class KComboBox;

class QPushButton;
class QLineEdit;
class QListWidget;
class QRadioButton;
class QCheckBox;

namespace Tellico {
  namespace Data {
    class Collection;
  }
  namespace GUI {
    class ComboBox;
  }

class FieldListItem;

/**
 * @author Robby Stephenson
 */
class CollectionFieldsDialog : public KDialog {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param coll A pointer to the collection parent of all the attributes
   * @param parent A pointer to the parent widget
   */
  CollectionFieldsDialog(Data::CollPtr coll, QWidget* parent);
  ~CollectionFieldsDialog();

  void setNotifyKernel(bool notify);

signals:
  void signalCollectionModified();

protected slots:
  virtual void slotOk();
  virtual void slotApply();
  virtual void slotDefault();

private slots:
  void slotNew();
  void slotDelete();
  void slotMoveUp();
  void slotMoveDown();
  void slotTypeChanged(const QString& type);
  void slotHighlightedChanged(int index);
  void slotModified();
  bool slotShowExtendedProperties();
  void slotSelectInitial();
  void slotDerivedChecked(bool checked);
  void resetToCurrent();

private:
  void applyChanges();
  void updateField();
  void updateTitle(const QString& title);
  bool checkValues();
  FieldListItem* findItem(Data::FieldPtr field);
  QStringList newTypesAllowed(int type);
  void populate(Data::FieldPtr field);

  Data::CollPtr m_coll;
  Data::CollPtr m_defaultCollection;
  Data::FieldList m_copiedFields;
  Data::FieldList m_newFields;
  Data::FieldPtr m_currentField;
  bool m_modified;
  bool m_updatingValues;
  bool m_reordered;
  int m_oldIndex;
  enum NotifyMode { NotifyKernel, NoNotification };
  NotifyMode m_notifyMode;

  QListWidget* m_fieldsWidget;
  QPushButton* m_btnNew;
  QPushButton* m_btnDelete;
  QPushButton* m_btnUp;
  QPushButton* m_btnDown;

  QLineEdit* m_titleEdit;
  KComboBox* m_typeCombo;
  KComboBox* m_catCombo;
  QLineEdit* m_descEdit;
  QLineEdit* m_derivedEdit;
  QLineEdit* m_defaultEdit;
  QCheckBox* m_derived;
  QLineEdit* m_allowEdit;
  QPushButton* m_btnExtended;

  GUI::ComboBox* m_formatCombo;
  QCheckBox* m_complete;
  QCheckBox* m_multiple;
  QCheckBox* m_grouped;
};

} // end namespace
#endif
