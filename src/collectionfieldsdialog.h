/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef COLLECTIONFIELDSDIALOG_H
#define COLLECTIONFIELDSDIALOG_H

class KComboBox;
class KLineEdit;
class KPushButton;

class QRadioButton;
class QCheckBox;
class QPainter;

#include "datavectors.h"
#include "gui/listboxtext.h"

#include <kdialogbase.h>

#include <qmap.h>
#include <qlistbox.h>

namespace Tellico {
  namespace Data {
    class Collection;
  }

class FieldListBox : public GUI::ListBoxText {
public:
  FieldListBox(QListBox* listbox, Data::FieldPtr field);
  FieldListBox(QListBox* listbox, Data::FieldPtr field, QListBoxItem* after);

  Data::FieldPtr field() const { return m_field; }
  void setField(Data::FieldPtr field) { m_field = field; }

private:
  Data::FieldPtr m_field;
};

/**
 * @author Robby Stephenson
 */
class CollectionFieldsDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param coll A pointer to the collection parent of all the attributes
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  CollectionFieldsDialog(Data::CollPtr coll, QWidget* parent, const char* name=0);
  ~CollectionFieldsDialog();

signals:
  void signalCollectionModified();

protected slots:
  virtual void slotOk();
  virtual void slotApply();
  virtual void slotDefault();
  void slotNew();
  void slotDelete();
  void slotMoveUp();
  void slotMoveDown();
  void slotTypeChanged(const QString& type);
  void slotHighlightedChanged(int index);
  void slotModified();
  void slotUpdateTitle(const QString& title);
  bool slotShowExtendedProperties();

protected:
  void updateField();
  bool checkValues();
  FieldListBox* findItem(const QListBox* box, Data::FieldPtr field);
  QStringList newTypesAllowed(int type);

private slots:
  void slotSelectInitial();

private:
  Data::CollPtr m_coll;
  Data::CollPtr m_defaultCollection;
  Data::FieldVec m_copiedFields;
  Data::FieldVec m_newFields;
  Data::FieldPtr m_currentField;
  bool m_modified;
  bool m_updatingValues;
  bool m_reordered;
  int m_oldIndex;

  QListBox* m_fieldsBox;
  KPushButton* m_btnNew;
  KPushButton* m_btnDelete;
  KPushButton* m_btnUp;
  KPushButton* m_btnDown;

  KLineEdit* m_titleEdit;
  KComboBox* m_typeCombo;
  KLineEdit* m_allowEdit;
  KComboBox* m_catCombo;
  KLineEdit* m_descEdit;
  KPushButton* m_btnExtended;

  QRadioButton* m_formatNone;
  QRadioButton* m_formatPlain;
  QRadioButton* m_formatTitle;
  QRadioButton* m_formatName;
  QCheckBox* m_complete;
  QCheckBox* m_multiple;
  QCheckBox* m_grouped;
};

} // end namespace
#endif
