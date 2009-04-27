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
#include "../tellico_utils.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <kmenu.h>
#include <kicon.h>
#include <kinputdialog.h>

#include <QTableWidget>
#include <QMouseEvent>
#include <QEvent>
#include <QHeaderView>

namespace {
  static const int MIN_TABLE_ROWS = 5;
  static const int MAX_TABLE_COLS = 10;
}

using Tellico::GUI::TableFieldWidget;

TableFieldWidget::TableFieldWidget(Tellico::Data::FieldPtr field_, QWidget* parent_)
    : FieldWidget(field_, parent_), m_field(field_), m_row(-1), m_col(-1) {

  bool ok;
  m_columns = Tellico::toUInt(field_->property(QLatin1String("columns")), &ok);
  if(!ok) {
    m_columns = 1;
  } else {
    m_columns = qMin(m_columns, MAX_TABLE_COLS);
  }

  m_table = new QTableWidget(MIN_TABLE_ROWS, m_columns, this);
  labelColumns(m_field);

  m_table->setDragEnabled(false);

  m_table->horizontalHeader()->setResizeMode(m_columns-1, QHeaderView::Interactive);
  m_table->resizeColumnToContents(m_columns-1);
  m_table->setSelectionMode(QAbstractItemView::NoSelection);
  m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // capture all the context menu events
  m_table->setContextMenuPolicy(Qt::CustomContextMenu);
  m_table->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
  m_table->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(m_table, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(checkModified()));
  connect(m_table, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(slotResizeColumn(QTableWidgetItem*)));
  connect(m_table, SIGNAL(currentCellChanged(int, int, int, int)), SLOT(slotCheckRows(int, int)));
  connect(m_table, SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(tableContextMenu(const QPoint&)));
  connect(m_table->horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(horizontalHeaderContextMenu(const QPoint&)));
  connect(m_table->verticalHeader(), SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(verticalHeaderContextMenu(const QPoint&)));

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
        cstack += QLatin1String("::");
      } else {
        rowStr += cstack + str + QLatin1String("::");
        cstack.clear();
      }
    }
    if(rowStr.isEmpty()) {
      rstack += QLatin1String("; ");
    } else {
      rowStr.truncate(rowStr.length()-2); // remove last semi-colon and space
      text += rstack + rowStr + QLatin1String("; ");
      rstack.clear();
    }
  }
  if(!text.isEmpty()) {
    text.truncate(text.length()-2); // remove last semi-colon and space
  }

  // now reduce number of rows if necessary
  bool loop = true;
  for(int row = m_table->rowCount()-1; loop && row > MIN_TABLE_ROWS; --row) {
    if(emptyRow(row)) {
      m_table->removeRow(row);
    } else {
      loop = false;
    }
  }
  return text;
}

void TableFieldWidget::setTextImpl(const QString& text_) {
  QStringList list = Data::Field::split(text_, true);
  if(list.count() != m_table->rowCount()) {
    m_table->setRowCount(qMax(list.count(), MIN_TABLE_ROWS));
  }
  for(int row = 0; row < list.count(); ++row) {
    for(int col = 0; col < m_table->columnCount(); ++col) {
      QString value = list[row].section(QLatin1String("::"), col, col);
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
  labelColumns(m_field);
  editMultiple(false);
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
  QString newName = KInputDialog::getText(i18n("Rename Column"), i18n("New column name:"),
                                          name, &ok, this);
  if(ok && !newName.isEmpty()) {
    Data::FieldPtr newField(new Data::Field(*m_field));
    newField->setProperty(QString::fromLatin1("column%1").arg(m_col+1), newName);
    if(Kernel::self()->modifyField(newField)) {
      m_field = newField;
      labelColumns(m_field);
    }
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
    QString s = field_->property(QString::fromLatin1("column%1").arg(col+1));
    if(s.isEmpty()) {
      s = i18n("Column %1", col+1);
    }
    labels += s;
//    m_table->horizontalHeaderItem(col)->setText(s);
  }
  m_table->setHorizontalHeaderLabels(labels);
}

void TableFieldWidget::updateFieldHook(Tellico::Data::FieldPtr, Tellico::Data::FieldPtr newField_) {
  bool ok;
  m_columns = Tellico::toUInt(newField_->property(QLatin1String("columns")), &ok);
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

void TableFieldWidget::tableContextMenu(const QPoint& point_) {
  if(point_.isNull()) {
    return;
  }
  m_row = m_table->rowAt(point_.y());
  m_col = m_table->columnAt(point_.x());
  makeRowContextMenu(m_table->mapToGlobal(point_));
}

void TableFieldWidget::horizontalHeaderContextMenu(const QPoint& point_) {
  int col = m_table->horizontalHeader()->logicalIndexAt(point_.x());
  if(col < 0 || col >= m_columns) {
    return;
  }
  m_row = -1;
  m_col = col;

  KMenu menu(this);
  menu.addAction(KIcon(QLatin1String("edit-rename")), i18n("Rename Column..."),
                 this, SLOT(slotRenameColumn()));
  menu.addAction(KIcon(QLatin1String("edit-clear")), i18n("Clear Table"),
                 this, SLOT(clear()));
  menu.exec(m_table->horizontalHeader()->mapToGlobal(point_));
}

void TableFieldWidget::verticalHeaderContextMenu(const QPoint& point_) {
  int row = m_table->verticalHeader()->logicalIndexAt(point_.y());
  if(row < 0 || row >= m_table->rowCount()) {
    return;
  }
  m_row = row;
  m_col = -1;
  makeRowContextMenu(m_table->verticalHeader()->mapToGlobal(point_));
 }

void TableFieldWidget::makeRowContextMenu(const QPoint& point_) {
  KMenu menu(this);
  menu.addAction(KIcon(QLatin1String("insrow")), i18n("Insert Row"),
                 this, SLOT(slotInsertRow()));
  menu.addAction(KIcon(QLatin1String("remrow")), i18n("Remove Row"),
                 this, SLOT(slotRemoveRow()));
  QAction* act = menu.addAction(KIcon(QLatin1String("arrow-up")), i18n("Move Row Up"),
                                this, SLOT(slotMoveRowUp()));
  if(m_row < 1) {
    act->setEnabled(false);
  }
  act = menu.addAction(KIcon(QLatin1String("arrow-down")), i18n("Move Row Down"),
                       this, SLOT(slotMoveRowDown()));
  if(m_row < 0 || m_row > m_table->rowCount()-1) {
    act->setEnabled(false);
  }
  menu.addSeparator();
  act = menu.addAction(KIcon(QLatin1String("edit-rename")), i18n("Rename Column..."),
                       this, SLOT(slotRenameColumn()));
  if(m_col < 0 || m_col > m_columns-1) {
    act->setEnabled(false);
  }
  menu.addSeparator();
  menu.addAction(KIcon(QLatin1String("edit-clear")), i18n("Clear Table"),
                 this, SLOT(slotClear()));

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
  for(int col = 0; col < m_table->columnCount(); ++col) {
    QTableWidgetItem* item1 = m_table->takeItem(m_row-1, col);
    QTableWidgetItem* item2 = m_table->takeItem(m_row  , col);
    if(item1 && item2) {
      m_table->setItem(m_row  , col, item1);
      m_table->setItem(m_row-1, col, item2);
    }
  }
  checkModified();
}

void TableFieldWidget::slotMoveRowDown() {
  if(m_row < 0 || m_row > m_table->rowCount()-2) {
    return;
  }
  for(int col = 0; col < m_table->columnCount(); ++col) {
    QTableWidgetItem* item1 = m_table->takeItem(m_row  , col);
    QTableWidgetItem* item2 = m_table->takeItem(m_row+1, col);
    if(item1 && item2) {
      m_table->setItem(m_row+1, col, item1);
      m_table->setItem(m_row  , col, item2);
    }
  }
  checkModified();
}

void TableFieldWidget::slotClear() {
  clearImpl();
}

#include "tablefieldwidget.moc"
