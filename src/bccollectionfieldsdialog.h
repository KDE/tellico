/***************************************************************************
                          bccollectionfieldsdialog.h
                             -------------------
    begin                : Thu Apr 3 2003
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

#ifndef BCCOLLECTIONFIELDSDIALOG_H
#define BCCOLLECTIONFIELDSDIALOG_H

class BCCollection;

class KComboBox;
class KLineEdit;
class KPushButton;

class QRadioButton;
class QCheckBox;
class QPainter;

#include "bcattribute.h"

#include <kdialogbase.h>

#include <qmap.h>
#include <qlistbox.h>

/**
 * BCListBoxText subclasses QListBoxText so that @ref setText() can be made public,
 * and the font color can be changed
 *
 * @author Robby Stephenson
 * @version $Id: bccollectionfieldsdialog.h 284 2003-11-10 02:05:34Z robby $
 */
class BCListBoxText : public QListBoxText {
public:
  BCListBoxText(QListBox* listbox, BCAttribute* att);
  BCListBoxText(QListBox* listbox, BCAttribute* att, QListBoxItem* after);

  BCAttribute* attribute() const { return m_attribute; }
  void setAttribute(BCAttribute* att) { m_attribute = att; }
  void setColored(bool colored);
  void setText(const QString& text);

protected:
  virtual void paint(QPainter* painter);

private:
  BCAttribute* m_attribute;
  bool m_colored;
};

/**
 * @author Robby Stephenson
 * @version $Id: bccollectionfieldsdialog.h 284 2003-11-10 02:05:34Z robby $
 */
class BCCollectionFieldsDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * The constructor sets up the dialog.
   *
   * @param coll A pointer to the collection parent of all the attributes
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCCollectionFieldsDialog(BCCollection* coll, QWidget* parent, const char* name=0);
  ~BCCollectionFieldsDialog();

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

protected:
  void updateAttribute();
  BCListBoxText* findItem(const QListBox* box, const BCAttribute* att);

private:
  BCCollection* m_coll;
  BCCollection* m_defaultCollection;
  QMap<BCAttribute::AttributeType, QString> m_typeMap;
  BCAttributeList m_copiedAttributes;
  BCAttributeList m_newAttributes;
  BCAttribute* m_currentAttribute;
  int m_currentListItem;
  bool m_modified;
  bool m_updatingValues;
  bool m_reordered;
  
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
  KLineEdit* m_bibtexEdit;

  QRadioButton* m_formatNone;
  QRadioButton* m_formatPlain;
  QRadioButton* m_formatTitle;
  QRadioButton* m_formatName;
  QCheckBox* m_complete;
  QCheckBox* m_multiple;
  QCheckBox* m_grouped;
};

#endif
