/***************************************************************************
                                csvimporter.h
                             -------------------
    begin                : Wed Sep 24 2003
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

#ifndef CSVIMPORTER_H
#define CSVIMPORTER_H

class CSVImporterWidget;

class KLineEdit;
class KComboBox;
class KIntSpinBox;
class KPushButton;

class QButtonGroup;
class QCheckBox;
class QRadioButton;
class QTable;

#include "textimporter.h"
#include "../bccollectionfactory.h" // needed for CollectionNameMap

#include <qobject.h>

/**
 * @author Robby Stephenson
 * @version $Id: csvimporter.h 267 2003-11-08 09:18:46Z robby $
 */
class CSVImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  CSVImporter(const KURL& url);

  /**
   * @return A pointer to a @ref BCCollection, or 0 if none can be created.
   */
  virtual BCCollection* collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent, const char* name=0);

private slots:
  void slotTypeChanged(const QString& name);
  void slotFieldChanged(int idx);
  void slotFirstRowHeader(bool b);
  void slotDelimiter();
  void slotCurrentChanged(int row, int col);
  void slotHeaderClicked(int col);
  void slotSelectColumn(int col);
  void slotSetColumnTitle();

private:
  QStringList splitLine(const QString& line);
  void fillTable();
  void updateHeader(bool force);

  BCCollection* m_coll;
  CollectionNameMap m_typeMap;
  bool m_firstRowHeader;
  QString m_delimiter;

  QWidget* m_widget;
  KComboBox* m_comboType;
  QCheckBox* m_checkFirstRowHeader;
  QButtonGroup* m_delimiterGroup;
  QRadioButton* m_radioComma;
  QRadioButton* m_radioSemicolon;
  QRadioButton* m_radioTab;
  QRadioButton* m_radioOther;
  KLineEdit* m_editOther;
  QTable* m_table;
  KIntSpinBox* m_colSpinBox;
  KComboBox* m_comboField;
  KPushButton* m_setColumnBtn;

  static const QChar s_quote;
};
#endif
