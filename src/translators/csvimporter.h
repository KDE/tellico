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

#ifndef TELLICO_CSVIMPORTER_H
#define TELLICO_CSVIMPORTER_H

#include "textimporter.h"
#include "../datavectors.h"

class CSVImporterWidget;

class KComboBox;

class QSpinBox;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QRadioButton;
class QTableWidget;
class CsvTest;

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

friend class ::CsvTest;

public:
  /**
   */
  CSVImporter(const QUrl& url);
  ~CSVImporter();

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection() override;
  /**
   * The CSVImporter can import any type known to Tellico.
   */
  virtual bool canImport(int type) const override { Q_UNUSED(type); return true; }
  /**
   */
  virtual QWidget* widget(QWidget* parent) override;

  virtual bool validImport() const override;

  void setCollectionType(int collType);
  void setImportColumns(const QList<int>& columns, const QStringList& fieldNames);
  void setDelimiter(const QString& delimeter);
  void setColumnDelimiter(const QString& delimeter);
  void setRowDelimiter(const QString& delimeter);

public Q_SLOTS:
  void slotActionChanged(int action) override;
  void slotCancel() override;

private Q_SLOTS:
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
  void updateHeader();
  void createCollection();
  void updateFieldCombo();

  int m_collType;
  Data::CollPtr m_coll;
  Data::CollPtr m_existingCollection; // used to grab fields from current collection in window
  QList<int> m_columnsToImport;
  QStringList m_fieldsToImport;
  bool m_firstRowHeader;
  QString m_delimiter;
  QString m_colDelimiter;
  QString m_rowDelimiter;
  bool m_cancelled;

  QWidget* m_widget;
  GUI::CollectionTypeCombo* m_comboColl;
  QCheckBox* m_checkFirstRowHeader;
  QRadioButton* m_radioComma;
  QRadioButton* m_radioSemicolon;
  QRadioButton* m_radioTab;
  QRadioButton* m_radioOther;
  QLineEdit* m_editOther;
  QLineEdit* m_editColDelimiter;
  QLineEdit* m_editRowDelimiter;
  QTableWidget* m_table;
  QSpinBox* m_colSpinBox;
  KComboBox* m_comboField;
  QPushButton* m_setColumnBtn;
  bool m_hasAssignedFields;
  bool m_isLibraryThing;

  CSVParser* m_parser;
};

  } // end namespace
} // end namespace
#endif
