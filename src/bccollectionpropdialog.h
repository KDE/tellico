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

class BCCollection;

class KComboBox;
class KLineEdit;

class QRadioButton;
class QCheckBox;
class QPushButton;
class QPainter;

#include "bcattribute.h"

#include <kdialogbase.h>

#include <qmap.h>
#include <qdict.h>
#include <qlistbox.h>

/**
 * BCListBoxText subclasses QListBoxText so that @ref setText() can be made public,
 * and the font color can be changed
 *
 * @author Robby Stephenson
 * @version $Id: bccollectionpropdialog.h,v 1.2.2.3 2003/07/23 01:31:12 robby Exp $
 */
class BCListBoxText : public QListBoxText {
public:
  BCListBoxText(QListBox* listbox, const QString& text);

  void setColored(bool colored);
  void setText(const QString& text);

protected:
  virtual void paint(QPainter* painter);

private:
  bool m_colored;
};

/**
 * @author Robby Stephenson
 * @version $Id: bccollectionpropdialog.h,v 1.2.2.3 2003/07/23 01:31:12 robby Exp $
 */
class BCCollectionPropDialog : public KDialogBase {
Q_OBJECT

public: 
  /**
   * The constructor sets up the dialog.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BCCollectionPropDialog(BCCollection* coll, QWidget* parent, const char* name=0);

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
