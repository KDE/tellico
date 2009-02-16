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

#ifndef TELLICO_CSVIMPORTER_H
#define TELLICO_CSVIMPORTER_H

#include "textimporter.h"
#include "../datavectors.h"

class CSVImporterWidget;

class KLineEdit;
class KComboBox;
class KIntSpinBox;
class KPushButton;

class QGroupBox;
class QCheckBox;
class QRadioButton;
class QTableWidget;

namespace Tellico {
  namespace GUI {
    class CollectionTypeCombo;
  }
  class CSVParser;
  namespace Import {

/**
 * @author Robby Stephenson
 */
class CSVImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  CSVImporter(const KUrl& url);
  ~CSVImporter();

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget* parent);

  virtual bool validImport() const;

public slots:
  void slotActionChanged(int action);
  void slotCancel();

private slots:
  void slotTypeChanged();
  void slotFieldChanged(int idx);
  void slotFirstRowHeader(bool b);
  void slotDelimiter();
  void slotCurrentChanged(int row, int col);
  void slotHeaderClicked(int col);
  void slotSelectColumn(int col);
  void slotSetColumnTitle();

private:
  void fillTable();
  void updateHeader(bool force);

  Data::CollPtr m_coll;
  Data::CollPtr m_existingCollection; // used to grab fields from current collection in window
  bool m_firstRowHeader;
  QString m_delimiter;
  bool m_cancelled;

  QWidget* m_widget;
  GUI::CollectionTypeCombo* m_comboColl;
  QCheckBox* m_checkFirstRowHeader;
  QGroupBox* m_delimiterGroup;
  QRadioButton* m_radioComma;
  QRadioButton* m_radioSemicolon;
  QRadioButton* m_radioTab;
  QRadioButton* m_radioOther;
  KLineEdit* m_editOther;
  QTableWidget* m_table;
  KIntSpinBox* m_colSpinBox;
  KComboBox* m_comboField;
  KPushButton* m_setColumnBtn;
  bool m_hasAssignedFields;

  CSVParser* m_parser;
};

  } // end namespace
} // end namespace
#endif
