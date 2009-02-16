/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_COLLECTIONFIELDSDIALOG_H
#define TELLICO_COLLECTIONFIELDSDIALOG_H

#include "datavectors.h"

#include <kdialog.h>

class KComboBox;
class KLineEdit;
class KPushButton;
class KListWidget;

class QRadioButton;
class QCheckBox;
class QPainter;

namespace Tellico {
  namespace Data {
    class Collection;
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

private:
  void applyChanges();
  void updateField();
  void updateTitle(const QString& title);
  bool checkValues();
  FieldListItem* findItem(Data::FieldPtr field);
  QStringList newTypesAllowed(int type);

  Data::CollPtr m_coll;
  Data::CollPtr m_defaultCollection;
  Data::FieldList m_copiedFields;
  Data::FieldList m_newFields;
  Data::FieldPtr m_currentField;
  bool m_modified;
  bool m_updatingValues;
  bool m_reordered;
  int m_oldIndex;

  KListWidget* m_fieldsWidget;
  KPushButton* m_btnNew;
  KPushButton* m_btnDelete;
  KPushButton* m_btnUp;
  KPushButton* m_btnDown;

  KLineEdit* m_titleEdit;
  KComboBox* m_typeCombo;
  KLineEdit* m_allowEdit;
  KLineEdit* m_defaultEdit;
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
