/***************************************************************************
                           bccollectionpropdialog.h
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

#ifndef BCCOLLECTIONPROPDIALOG_H
#define BCCOLLECTIONPROPDIALOG_H

class Bookcase;
class BCCollection;

class KComboBox;
class KLineEdit;

class QRadioButton;
class QCheckBox;
class QListBox;
class QPushButton;

#include "bcattribute.h"

#include <kdialogbase.h>

#include <qmap.h>
#include <qdict.h>

/**
 * @author Robby Stephenson
 * @version $Id: bccollectionpropdialog.h,v 1.1 2003/05/02 06:50:28 robby Exp $
 */
class BCCollectionPropDialog : public KDialogBase  {
Q_OBJECT

public: 
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget, a Bookcase object
   * @param name The widget name
   */
  BCCollectionPropDialog(BCCollection* coll, Bookcase* parent, const char* name=0);

signals:
  void signalCollectionModified();

protected slots:
  virtual void slotOk();
  virtual void slotApply();
  void slotNew();
  void slotDelete();
  void slotTypeChanged(const QString& type);
  void slotUpdateValues(const QString& title);
  void slotModified();
  void slotUpdateTitle(const QString& title);

protected:
  void updateAttribute();

private:
  Bookcase* m_bookcase;
  BCCollection* m_coll;
  QMap<BCAttribute::AttributeType, QString> m_typeMap;
  QDict<BCAttribute> m_copiedAttributes;
  BCAttributeList m_newAttributes;
  BCAttribute* m_currentAttribute;
  int m_currentListItem;
  bool m_modified;
  bool m_updatingValues;
  
  QListBox* m_fieldsBox;
  QPushButton* m_btnNew;
  QPushButton* m_btnDelete;
  
  KLineEdit* m_titleEdit;
  KComboBox* m_typeCombo;
  KLineEdit* m_allowEdit;
  KComboBox* m_catCombo;
  KLineEdit* m_descEdit;

  QRadioButton* m_formatPlain;
  QRadioButton* m_formatTitle;
  QRadioButton* m_formatName;
  QCheckBox* m_complete;
  QCheckBox* m_multiple;
  QCheckBox* m_grouped;
};

#endif
