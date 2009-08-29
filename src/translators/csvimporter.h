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

class KLineEdit;
class KComboBox;
class KIntSpinBox;
class KPushButton;

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
