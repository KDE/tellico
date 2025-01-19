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

#include "tablefieldwidget.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QMenu>
#include <QTableWidget>
#include <QMouseEvent>
#include <QEvent>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>

namespace {
  static const int MIN_TABLE_ROWS = 5;
  static const int MAX_TABLE_COLS = 10;
}

using Tellico::GUI::TableFieldWidget;

TableFieldWidget::TableFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_), m_row(-1), m_col(-1) {

  bool ok;
  m_columns = Tellico::toUInt(field_->property(QStringLiteral("columns")), &ok);
  if(!ok) {
    m_columns = 1;
  } else {
    m_columns = qMin(m_columns, MAX_TABLE_COLS);
  }

  m_table = new QTableWidget(MIN_TABLE_ROWS, m_columns, this);
  labelColumns(field());

  m_table->setDragEnabled(false);

  m_table->horizontalHeader()->setSectionResizeMode(m_columns-1, QHeaderView::Interactive);
  m_table->resizeColumnToContents(m_columns-1);
  m_table->setSelectionMode(QAbstractItemView::NoSelection);
//  m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // capture all the context menu events
  m_table->setContextMenuPolicy(Qt::CustomContextMenu);
  m_table->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
  m_table->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(m_table, &QTableWidget::itemChanged, this, &TableFieldWidget::checkModified);
  connect(m_table, &QTableWidget::itemChanged, this, &TableFieldWidget::slotResizeColumn);
  connect(m_table, &QTableWidget::currentCellChanged, this, &TableFieldWidget::slotCheckRows);
  connect(m_table, &QWidget::customContextMenuRequested, this, &TableFieldWidget::tableContextMenu);
  connect(m_table->horizontalHeader(), &QWidget::customContextMenuRequested, this, &TableFieldWidget::horizontalHeaderContextMenu);
  connect(m_table->verticalHeader(), &QWidget::customContextMenuRequested, this, &TableFieldWidget::verticalHeaderContextMenu);

  registerWidget();
}

QString TableFieldWidget::text() const {
  QString text, str, rstack, cstack, rowStr;
  for(int row = 0; row < m_table->rowCount(); ++row) {
    rowStr.clear();
    cstack.clear();
    for(int col = 0; col < m_table->columnCount(); ++col) {
      QTableWidgetItem* item = m_table->item(row, col);
      str = item ? item->text().simplified() : QString();
      if(str.isEmpty()) {
        cstack += FieldFormat::columnDelimiterString();
      } else {
        rowStr += cstack + str + FieldFormat::columnDelimiterString();
        cstack.clear();
      }
    }
    if(rowStr.isEmpty()) {
      rstack += FieldFormat::rowDelimiterString();
    } else {
      rowStr.truncate(rowStr.length()-FieldFormat::columnDelimiterString().length()); // remove last delimiter
      text += rstack + rowStr + FieldFormat::rowDelimiterString();
      rstack.clear();
    }
  }
  if(!text.isEmpty()) {
    text.truncate(text.length()-FieldFormat::rowDelimiterString().length()); // remove last delimiter
  }
  return text;
}

void TableFieldWidget::setTextImpl(const QString& text_) {
  const QStringList rows = FieldFormat::splitTable(text_);
  if(rows.count() != m_table->rowCount()) {
    m_table->setRowCount(qMax(rows.count(), MIN_TABLE_ROWS));
  }
  for(int row = 0; row < rows.count(); ++row) {
    QStringList columnValues = FieldFormat::splitRow(rows.at(row));
    const int ncols = m_table->columnCount();
    if(ncols < columnValues.count()) {
      // need to combine all the last values, from ncols-1 to end
      QString lastValue = QStringList(columnValues.mid(ncols-1)).join(FieldFormat::columnDelimiterString());
      columnValues = columnValues.mid(0, ncols);
      columnValues.replace(ncols-1, lastValue);
    }
    for(int col = 0; col < ncols; ++col) {
      QString value = col < columnValues.count() ? columnValues.at(col) : QString();
      QTableWidgetItem* item = new QTableWidgetItem(value);
      m_table->setItem(row, col, item);
    }
  }
  // adjust all columns
  for(int col = 0; col < m_table->columnCount(); ++col) {
    m_table->resizeColumnToContents(col);
  }
}

void TableFieldWidget::clearImpl() {
  m_table->clear();
  m_table->setRowCount(MIN_TABLE_ROWS);
  labelColumns(field());
  editMultiple(false);
  checkModified();
}

QWidget* TableFieldWidget::widget() {
  return m_table;
}

void TableFieldWidget::slotCheckRows(int row_, int) {
  if(row_ == m_table->rowCount()-1 && !emptyRow(row_)) { // if is last row and row above is not empty
    m_table->insertRow(m_table->rowCount());
  }
}

void TableFieldWidget::slotResizeColumn(QTableWidgetItem* item_) {
  m_table->resizeColumnToContents(item_->column());
}

void TableFieldWidget::slotRenameColumn() {
  if(m_col < 0 || m_col >= m_columns) {
    return;
  }
  QString name = m_table->horizontalHeaderItem(m_col)->text();
  bool ok;
  QString newName = QInputDialog::getText(this, i18n("Rename Column"), i18n("New column name:"),
                                          QLineEdit::Normal, name, &ok);
  if(ok) {
    renameColumn(newName);
  }
}

bool TableFieldWidget::emptyRow(int row_) const {
  for(int col = 0; col < m_table->columnCount(); ++col) {
    QTableWidgetItem* item = m_table->item(row_, col);
    if(item && !item->text().isEmpty()) {
      return false;
    }
  }
  return true;
}

void TableFieldWidget::labelColumns(Tellico::Data::FieldPtr field_) {
  QStringList labels;
  for(int col = 0; col < m_columns; ++col) {
    QString s = field_->property(QStringLiteral("column%1").arg(col+1));
    if(s.isEmpty()) {
      s = i18n("Column %1", col+1);
    }
    labels += s;
  }
  m_table->setHorizontalHeaderLabels(labels);
}

void TableFieldWidget::renameColumn(const QString& newName_) {
  Q_ASSERT(m_col >= 0);
  Q_ASSERT(m_col < m_columns);
  Q_ASSERT(!newName_.isEmpty());

  Data::FieldPtr newField(new Data::Field(*field()));
  newField->setProperty(QStringLiteral("column%1").arg(m_col+1), newName_);
  Q_EMIT fieldChanged(newField);
  setField(newField);
  labelColumns(newField);
}

void TableFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  bool ok;
  m_columns = Tellico::toUInt(newField_->property(QStringLiteral("columns")), &ok);
  if(!ok) {
    m_columns = 1;
  } else {
    m_columns = qMin(m_columns, MAX_TABLE_COLS); // max of 5 columns
  }
  if(m_columns != m_table->columnCount()) {
    m_table->setColumnCount(m_columns);
  }
  labelColumns(newField_);
}

void TableFieldWidget::tableContextMenu(QPoint point_) {
  if(point_.isNull()) {
    return;
  }
  m_row = m_table->rowAt(point_.y());
  m_col = m_table->columnAt(point_.x());
  makeRowContextMenu(m_table->mapToGlobal(point_));
}

void TableFieldWidget::horizontalHeaderContextMenu(QPoint point_) {
  int col = m_table->horizontalHeader()->logicalIndexAt(point_.x());
  if(col < 0 || col >= m_columns) {
    return;
  }
  m_row = -1;
  m_col = col;

  QMenu menu(this);
  menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("Rename Column..."),
                 this, &TableFieldWidget::slotRenameColumn);
  menu.addAction(QIcon::fromTheme(QStringLiteral("edit-clear")), i18n("Clear Table"),
                 this, &TableFieldWidget::clearImpl);
  menu.exec(m_table->horizontalHeader()->mapToGlobal(point_));
}

void TableFieldWidget::verticalHeaderContextMenu(QPoint point_) {
  int row = m_table->verticalHeader()->logicalIndexAt(point_.y());
  if(row < 0 || row >= m_table->rowCount()) {
    return;
  }
  m_row = row;
  m_col = -1;
  makeRowContextMenu(m_table->verticalHeader()->mapToGlobal(point_));
 }

void TableFieldWidget::makeRowContextMenu(QPoint point_) {
  QMenu menu(this);
  menu.addAction(QIcon::fromTheme(QStringLiteral("edit-table-insert-row-below")), i18n("Insert Row"),
                 this, &TableFieldWidget::slotInsertRow);
  menu.addAction(QIcon::fromTheme(QStringLiteral("edit-table-delete-row")), i18n("Remove Row"),
                 this, &TableFieldWidget::slotRemoveRow);
  QAction* act = menu.addAction(QIcon::fromTheme(QStringLiteral("arrow-up")), i18n("Move Row Up"),
                                this, &TableFieldWidget::slotMoveRowUp);
  if(m_row < 1) {
    act->setEnabled(false);
  }
  act = menu.addAction(QIcon::fromTheme(QStringLiteral("arrow-down")), i18n("Move Row Down"),
                       this, &TableFieldWidget::slotMoveRowDown);
  if(m_row < 0 || m_row > m_table->rowCount()-1) {
    act->setEnabled(false);
  }
  menu.addSeparator();
  act = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18n("Rename Column..."),
                       this, &TableFieldWidget::slotRenameColumn);
  if(m_col < 0 || m_col > m_columns-1) {
    act->setEnabled(false);
  }
  menu.addSeparator();
  menu.addAction(QIcon::fromTheme(QStringLiteral("edit-clear")), i18n("Clear Table"),
                 this, &TableFieldWidget::slotClear);

  menu.exec(point_);
}

void TableFieldWidget::slotInsertRow() {
  if(m_row > -1) {
    m_table->insertRow(m_row);
    checkModified();
  }
}

void TableFieldWidget::slotRemoveRow() {
  if(m_row > -1) {
    m_table->removeRow(m_row);
    checkModified();
  }
}

void TableFieldWidget::slotMoveRowUp() {
  if(m_row < 1 || m_row > m_table->rowCount()-1) {
    return;
  }
  m_table->blockSignals(true);
  for(int col = 0; col < m_table->columnCount(); ++col) {
    QTableWidgetItem* item1 = m_table->takeItem(m_row-1, col);
    QTableWidgetItem* item2 = m_table->takeItem(m_row  , col);
    if(item1) {
      m_table->setItem(m_row  , col, item1);
    }
    if(item2) {
      m_table->setItem(m_row-1, col, item2);
    }
  }
  m_table->blockSignals(false);
  checkModified();
}

void TableFieldWidget::slotMoveRowDown() {
  if(m_row < 0 || m_row > m_table->rowCount()-2) {
    return;
  }
  m_table->blockSignals(true);
  for(int col = 0; col < m_table->columnCount(); ++col) {
    QTableWidgetItem* item1 = m_table->takeItem(m_row  , col);
    QTableWidgetItem* item2 = m_table->takeItem(m_row+1, col);
    if(item1) {
      m_table->setItem(m_row+1, col, item1);
    }
    if(item2) {
      m_table->setItem(m_row  , col, item2);
    }
  }
  m_table->blockSignals(false);
  checkModified();
}

void TableFieldWidget::slotClear() {
  clearImpl();
}
