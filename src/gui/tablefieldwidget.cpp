/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "../tellico_kernel.h"

#include <klocale.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kinputdialog.h>

#include <qtable.h>

namespace {
  static const int MIN_TABLE_ROWS = 5;
  static const int MAX_TABLE_COLS = 10;
}

using Tellico::GUI::TableFieldWidget;

TableFieldWidget::TableFieldWidget(Data::FieldPtr field_, QWidget* parent_, const char* name_/*=0*/)
    : FieldWidget(field_, parent_, name_), m_field(field_), m_row(-1), m_col(-1) {

  bool ok;
  m_columns = Tellico::toUInt(field_->property(QString::fromLatin1("columns")), &ok);
  if(!ok) {
    m_columns = 1;
  } else {
    m_columns = QMIN(m_columns, MAX_TABLE_COLS); // max of 5 columns
  }

  m_table = new QTable(MIN_TABLE_ROWS, m_columns, this);
  labelColumns(m_field);
  // allow renaming of column titles
  m_table->horizontalHeader()->setClickEnabled(true);
  m_table->horizontalHeader()->installEventFilter(this);

  m_table->verticalHeader()->setClickEnabled(true);
  m_table->verticalHeader()->installEventFilter(this);

  m_table->setDragEnabled(false);
  m_table->setFocusStyle(QTable::FollowStyle);
  m_table->setRowMovingEnabled(true); // rows can be moved
  m_table->setColumnMovingEnabled(false); // columns remain fixed

  m_table->setColumnStretchable(m_columns-1, true);
  m_table->adjustColumn(m_columns-1);
  m_table->setSelectionMode(QTable::NoSelection);
  m_table->setHScrollBarMode(QScrollView::AlwaysOff);

  connect(m_table, SIGNAL(valueChanged(int, int)), SIGNAL(modified()));
  connect(m_table, SIGNAL(currentChanged(int, int)), SLOT(slotCheckRows(int, int)));
  connect(m_table, SIGNAL(valueChanged(int, int)), SLOT(slotResizeColumn(int, int)));
  connect(m_table, SIGNAL(contextMenuRequested(int, int, const QPoint&)), SLOT(contextMenu(int, int, const QPoint&)));

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
    m_table->showRow(row);
  }
  // remove any un-needed rows
  int minRow = QMAX(row, MIN_TABLE_ROWS);
  for(row = m_table->numRows()-1; row >= minRow; --row) {
    m_table->removeRow(row);
  }
  // adjust all columns
  for(int col = 0; col < m_table->numCols()-1; ++col) {
    m_table->adjustColumn(col);
  }
}

void TableFieldWidget::clear() {
  for(int row = 0; row < m_table->numRows(); ++row) {
    for(int col = 0; col < m_table->numCols(); ++col) {
      m_table->setText(row, col, QString::null);
    }
    if(row >= MIN_TABLE_ROWS) {
      m_table->removeRow(row);
      --row;
    }
  }
  editMultiple(false);
}

QWidget* TableFieldWidget::widget() {
  return m_table;
}

void TableFieldWidget::slotCheckRows(int row_, int) {
  if(row_ == m_table->numRows()-1 && !emptyRow(row_)) { // if is last row and row above is not empty
    m_table->insertRows(m_table->numRows());
  }
}

void TableFieldWidget::slotResizeColumn(int, int col_) {
  m_table->adjustColumn(col_);
}

void TableFieldWidget::slotRenameColumn() {
  if(m_col < 0 || m_col >= m_columns) {
    return;
  }
  QString name = m_table->horizontalHeader()->label(m_col);
  bool ok;
  QString newName = KInputDialog::getText(i18n("Rename Column"), i18n("New column name:"),
                                           name, &ok, this);
  if(ok && !newName.isEmpty()) {
    Data::FieldPtr newField = m_field->clone();
    newField->setProperty(QString::fromLatin1("column%1").arg(m_col+1), newName);
    if(Kernel::self()->modifyField(newField)) {
      labelColumns(newField);
    }
  }
}

bool TableFieldWidget::emptyRow(int row_) const {
  for(int col = 0; col < m_table->numCols(); ++col) {
    if(!m_table->text(row_, col).isEmpty()) {
      return false;
    }
  }
  return true;
}

void TableFieldWidget::labelColumns(Data::FieldPtr field_) {
  for(int i = 0; i < m_columns; ++i) {
    QString s = field_->property(QString::fromLatin1("column%1").arg(i+1));
    if(s.isEmpty()) {
      s = i18n("Column %1").arg(i+1);
    }
    m_table->horizontalHeader()->setLabel(i, s);
  }
}

void TableFieldWidget::updateFieldHook(Data::FieldPtr, Data::FieldPtr newField_) {
  bool ok;
  m_columns = Tellico::toUInt(newField_->property(QString::fromLatin1("columns")), &ok);
  if(!ok) {
    m_columns = 1;
  } else {
    m_columns = QMIN(m_columns, MAX_TABLE_COLS); // max of 5 columns
  }
  if(m_columns != m_table->numCols()) {
    m_table->setNumCols(m_columns);
  }
  // adjust all columns
  for(int col = 0; col < m_table->numCols(); ++col) {
    m_table->adjustColumn(col);
  }
  labelColumns(newField_);
}

bool TableFieldWidget::eventFilter(QObject* obj_, QEvent* ev_) {
  if(ev_->type() == QEvent::MouseButtonPress
      && static_cast<QMouseEvent*>(ev_)->button() == Qt::RightButton) {
    if(obj_ == m_table->horizontalHeader()) {
      QMouseEvent* ev = static_cast<QMouseEvent*>(ev_);
      // might be scrolled
      int pos = ev->x() + m_table->horizontalHeader()->offset();
      int col = m_table->horizontalHeader()->sectionAt(pos);
      if(col >= m_columns) {
        return false;
      }
      m_row = -1;
      m_col = col;
      KPopupMenu menu(this);
      menu.insertItem(SmallIconSet(QString::fromLatin1("edit")), i18n("Rename Column..."),
                      this, SLOT(slotRenameColumn()));
      menu.exec(ev->globalPos());
      return true;
    } else if(obj_ == m_table->verticalHeader()) {
      QMouseEvent* ev = static_cast<QMouseEvent*>(ev_);
      // might be scrolled
      int pos = ev->y() + m_table->verticalHeader()->offset();
      int row = m_table->verticalHeader()->sectionAt(pos);
      if(row < 0 || row > m_table->numRows()-1) {
        return false;
      }
      m_row = row;
      m_col = -1;
      // show regular right-click menu
      contextMenu(m_row, m_col, ev->globalPos());
      return true;
    }
  }
  return FieldWidget::eventFilter(obj_, ev_);
}

void TableFieldWidget::contextMenu(int row_, int col_, const QPoint& p_) {
  // might get called with col == -1 for clicking on vertical header
  // but a negative row means clicking outside bounds of table
  if(row_ < 0) {
    return;
  }
  m_row = row_;
  m_col = col_;

  int id;
  KPopupMenu menu(this);
  menu.insertItem(SmallIconSet(QString::fromLatin1("insrow")), i18n("Insert Row"),
                  this, SLOT(slotInsertRow()));
  menu.insertItem(SmallIconSet(QString::fromLatin1("remrow")), i18n("Remove Row"),
                  this, SLOT(slotRemoveRow()));
  id = menu.insertItem(SmallIconSet(QString::fromLatin1("1uparrow")), i18n("Move Row Up"),
                       this, SLOT(slotMoveRowUp()));
  if(m_row == 0) {
    menu.setItemEnabled(id, false);
  }
  id = menu.insertItem(SmallIconSet(QString::fromLatin1("1downarrow")), i18n("Move Row Down"),
                       this, SLOT(slotMoveRowDown()));
  if(m_row == m_table->numRows()-1) {
    menu.setItemEnabled(id, false);
  }
  menu.insertSeparator();
  id = menu.insertItem(SmallIconSet(QString::fromLatin1("edit")), i18n("Rename Column..."),
                       this, SLOT(slotRenameColumn()));
  if(m_col < 0 || m_col > m_columns-1) {
    menu.setItemEnabled(id, false);
  }
  menu.exec(p_);
}

void TableFieldWidget::slotInsertRow() {
  if(m_row > -1) {
    m_table->insertRows(m_row);
    emit modified();
  }
}

void TableFieldWidget::slotRemoveRow() {
  if(m_row > -1) {
    m_table->removeRow(m_row);
    emit modified();
  }
}

void TableFieldWidget::slotMoveRowUp() {
  if(m_row > 0) {
    m_table->swapRows(m_row, m_row-1, true);
    m_table->updateContents();
    emit modified();
  }
}

void TableFieldWidget::slotMoveRowDown() {
  if(m_row < m_table->numRows()-1) {
    m_table->swapRows(m_row, m_row+1, true);
    m_table->updateContents();
    emit modified();
  }
}

#include "tablefieldwidget.moc"
