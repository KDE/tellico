/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tablefieldwidget.h"
#include "../field.h"
#include "../tellico_utils.h"

#include <kglobal.h> // for KMIN

#include <qtable.h>

namespace {
  static const int MIN_TABLE_ROWS = 5;
  static const int MAX_TABLE_COLS = 5;
}

using Tellico::GUI::TableFieldWidget;

TableFieldWidget::TableFieldWidget(const Data::Field* field_, QWidget* parent_, const char* name_/*=0*/)
    : FieldWidget(field_, parent_, name_) {

  bool ok;
  m_columns = Tellico::toUInt(field_->property(QString::fromLatin1("columns")), &ok);
  if(!ok) {
    m_columns = 1;
  } else {
    m_columns = KMIN(m_columns, MAX_TABLE_COLS); // max of 5 columns
  }

  m_table = new QTable(MIN_TABLE_ROWS, m_columns, this);
  m_table->setTopMargin(0);
  m_table->horizontalHeader()->hide();
  m_table->verticalHeader()->setClickEnabled(false);
  m_table->verticalHeader()->setResizeEnabled(false);
  m_table->setDragEnabled(false);
  m_table->setRowMovingEnabled(false);
  m_table->setColumnMovingEnabled(false);

  m_table->setColumnStretchable(m_columns-1, true);
  m_table->adjustColumn(m_columns-1);
  m_table->setSelectionMode(m_columns == 1 ? QTable::Single : QTable::NoSelection);

  connect(m_table, SIGNAL(valueChanged(int, int)), SIGNAL(modified()));
  connect(m_table, SIGNAL(currentChanged(int, int)), SLOT(slotCheckRows(int, int)));
  connect(m_table, SIGNAL(valueChanged(int, int)), SLOT(slotResizeColumn(int, int)));

  registerWidget();
}

QString TableFieldWidget::text() const {
  QString text, str, rstack, cstack, rowStr;
  for(int row = 0; row < m_table->numRows(); ++row) {
    rowStr.truncate(0);
    cstack.truncate(0);
    for(int col = 0; col < m_table->numCols(); ++col) {
      str = m_table->text(row, col).simplifyWhiteSpace();
      if(str.isEmpty()) {
        cstack += QString::fromLatin1("::");
      } else {
        rowStr += cstack + str + QString::fromLatin1("::");
        cstack.truncate(0);
      }
    }
    if(rowStr.isEmpty()) {
      rstack += QString::fromLatin1("; ");
    } else {
      rowStr.truncate(rowStr.length()-2); // remove last semi-colon and space
      text += rstack + rowStr + QString::fromLatin1("; ");
      rstack.truncate(0);
    }
  }
  if(!text.isEmpty()) {
    text.truncate(text.length()-2); // remove last semi-colon and space
  }

  // now reduce number of rows if necessary
  bool loop = true;
  for(int row = m_table->numRows()-1; loop && row > MIN_TABLE_ROWS; --row) {
    bool empty = true;
    for(int col = 0; col < m_table->numCols(); ++col) {
      if(!m_table->text(row, col).isEmpty()) {
        empty = false;
        break;
      }
    }
    if(empty) {
      m_table->removeRow(row);
    } else {
      loop = false;
    }
  }
  return text;
}

void TableFieldWidget::setText(const QString& text_) {
  blockSignals(true);
  m_table->blockSignals(true);

  QStringList list = Data::Field::split(text_, true);
  // add additional rows if needed
  if(static_cast<int>(list.count()) > m_table->numRows()) {
    m_table->insertRows(m_table->numRows(), list.count()-m_table->numRows());
  }
  int row;
  for(row = 0; row < static_cast<int>(list.count()); ++row) {
    for(int col = 0; col < m_table->numCols(); ++col) {
      m_table->setText(row, col, list[row].section(QString::fromLatin1("::"), col, col));
    }
  }
  // now need to clear remaining rows
  for( ; row < m_table->numRows(); ++row) {
    for(int col = 0; col < m_table->numCols(); ++col) {
      m_table->setText(row, col, QString::null);
    }
  }
  // adjust all columns
  for(int col = 0; col < m_table->numCols(); ++col) {
    m_table->adjustColumn(col);
  }

  m_table->blockSignals(false);
  blockSignals(false);
}

void TableFieldWidget::clear() {
  for(int row = 0; row < m_table->numRows(); ++row) {
    for(int col = 0; col < m_table->numCols(); ++col) {
      m_table->setText(row, col, QString::null);
    }
  }
  editMultiple(false);
}

QWidget* TableFieldWidget::widget() {
  return m_table;
}

void TableFieldWidget::slotCheckRows(int row_, int) {
  if(row_ == m_table->numRows()-1) { // if is last row
    m_table->insertRows(m_table->numRows());
  }
}

void TableFieldWidget::slotResizeColumn(int, int col_) {
  m_table->adjustColumn(col_);
}

void TableFieldWidget::updateFieldHook(Data::Field*, Data::Field* newField_) {
  bool ok;
  m_columns = Tellico::toUInt(newField_->property(QString::fromLatin1("columns")), &ok);
  if(!ok) {
    m_columns = 1;
  } else {
    m_columns = KMIN(m_columns, MAX_TABLE_COLS); // max of 5 columns
  }
  if(m_columns != m_table->numCols()) {
    m_table->setNumCols(m_columns);
  }
  // adjust all columns
  for(int col = 0; col < m_table->numCols(); ++col) {
    m_table->adjustColumn(col);
  }
}

#include "tablefieldwidget.moc"
