/***************************************************************************
    Copyright (C) 2024 Robby Stephenson <robby@periapsis.org>
    Adapted from code (C) 2017 by Alexander Bruy
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

#include "checkablecombobox.h"

#include <QEvent>
#include <QMouseEvent>
#include <QLineEdit>
#include <QPoint>
#include <QAbstractItemView>

using Tellico::GUI::CheckableItemModel;
using Tellico::GUI::CheckBoxDelegate;
using Tellico::GUI::CheckableComboBox;

CheckableItemModel::CheckableItemModel(QObject* parent_)
    : QStandardItemModel(0, 1, parent_) {
}

Qt::ItemFlags CheckableItemModel::flags(const QModelIndex& index_) const {
  return index_.row() > 0 ? QStandardItemModel::flags(index_) | Qt::ItemIsUserCheckable :
                            QStandardItemModel::flags(index_);
}

QVariant CheckableItemModel::data(const QModelIndex& index_, int role_) const {
  QVariant value = QStandardItemModel::data(index_, role_);
  if(index_.isValid() && role_ == Qt::CheckStateRole && !value.isValid()) {
    value = Qt::Unchecked;
  }
  return value;
}

bool CheckableItemModel::setData(const QModelIndex& index_, const QVariant& value_, int role_) {
  const bool ok = QStandardItemModel::setData(index_, value_, role_);
  if(ok && role_ == Qt::CheckStateRole) {
    Q_EMIT itemCheckStateChanged(index_);
  }
  Q_EMIT dataChanged(index_, index_);
  return ok;
}

/**************************************************************/

CheckBoxDelegate::CheckBoxDelegate(QObject* parent_)
  : QStyledItemDelegate(parent_) {
}

void CheckBoxDelegate::paint(QPainter* painter_, const QStyleOptionViewItem& option_, const QModelIndex& index_) const {
  QStyleOptionViewItem& nonConstOpt = const_cast<QStyleOptionViewItem &>(option_);
  nonConstOpt.showDecorationSelected = false;
  QStyledItemDelegate::paint(painter_, nonConstOpt, index_);
}

/**************************************************************/

CheckableComboBox::CheckableComboBox(QWidget* parent_)
    : QComboBox(parent_)
    , m_separator(QStringLiteral(", ")) {
  setModel(new CheckableItemModel(this));
  setItemDelegate(new CheckBoxDelegate(this));

  auto lineEdit = new QLineEdit(this);
  lineEdit->setReadOnly(true);
  setLineEdit(lineEdit);
  lineEdit->installEventFilter(this);
  view()->viewport()->installEventFilter(this);

// c++20 deprecated the implicit capture
#if __cplusplus > 201703L
  connect(model(), &QStandardItemModel::rowsInserted,
          this, [=, this](const QModelIndex&, int, int) { updateDisplayText(); });
  connect(model(), &QStandardItemModel::rowsRemoved,
          this, [=, this](const QModelIndex&, int, int) { updateDisplayText(); });
  connect(model(), &QStandardItemModel::dataChanged,
          this, [=, this](const QModelIndex&, const QModelIndex&, const QVector<int> &) { updateDisplayText(); });
#else
  connect(model(), &QStandardItemModel::rowsInserted,
          this, [=](const QModelIndex&, int, int) { updateDisplayText(); });
  connect(model(), &QStandardItemModel::rowsRemoved,
          this, [=](const QModelIndex&, int, int) { updateDisplayText(); });
  connect(model(), &QStandardItemModel::dataChanged,
          this, [=](const QModelIndex&, const QModelIndex&, const QVector<int> &) { updateDisplayText(); });
#endif

  connect(this, &QComboBox::editTextChanged,
          this, &CheckableComboBox::setCheckedDataText);
}

QString CheckableComboBox::separator() const {
  return m_separator;
}

void CheckableComboBox::setSeparator(const QString& separator_) {
  if(m_separator != separator_) {
    m_separator = separator_;
    updateDisplayText();
  }
}

QStringList CheckableComboBox::checkedItems() const {
  QStringList items;
  if(auto* lModel = model()) {
    const QModelIndex index = lModel->index(0, modelColumn(), rootModelIndex());
    const QModelIndexList indexes = lModel->match(index, Qt::CheckStateRole, Qt::Checked, -1, Qt::MatchExactly);
    for(const QModelIndex& index : indexes) {
      items += index.data().toString();
    }
  }
  return items;
}

QVariantList CheckableComboBox::checkedItemsData() const {
  QVariantList data;
  if(auto* lModel = model()) {
    const QModelIndex index = lModel->index(0, modelColumn(), rootModelIndex());
    const QModelIndexList indexes = lModel->match(index, Qt::CheckStateRole, Qt::Checked, -1, Qt::MatchExactly);
    for(const QModelIndex& index : indexes) {
      data += index.data(Qt::UserRole).toString();
    }
  }
  return data;
}

Qt::CheckState CheckableComboBox::itemCheckState(int index_) const {
  return static_cast<Qt::CheckState>(itemData(index_, Qt::CheckStateRole).toInt());
}

void CheckableComboBox::setAllCheckState(Qt::CheckState checkState_) {
  for(int i = 0;  i < count(); i++) {
    setItemData(i, checkState_, Qt::CheckStateRole);
  }
  updateCheckedItems();
}

void CheckableComboBox::hidePopup() {
  if(!m_skipHide) {
    QComboBox::hidePopup();
  }
  m_skipHide = false;
}

bool CheckableComboBox::eventFilter(QObject* obj_, QEvent* ev_) {
  if(obj_ == lineEdit()) {
    if(ev_->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent*>(ev_)->button() == Qt::LeftButton) {
      m_skipHide = true;
      showPopup();
    }
  } else if((ev_->type() == QEvent::MouseButtonPress || ev_->type() == QEvent::MouseButtonRelease)
              && obj_ == view()->viewport()) {
    m_skipHide = true;
    if(ev_->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent*>(ev_)->button() == Qt::RightButton) {
      return true;
    }

    if(ev_->type() == QEvent::MouseButtonRelease) {
      const QModelIndex index = view()->indexAt(static_cast<QMouseEvent*>(ev_)->pos());
      if(index.isValid()) {
        auto item = static_cast<CheckableItemModel*>(model())->itemFromIndex(index);
        // uncheck the first item, if a different item was checked
        // uncheckk all other items if the first item was checked
        // remember the check state hasn't been changed yet. Do this check first so that the state changed signal
        // gets fired after all items are updated
        blockSignals(true);
        if(item->checkState() == Qt::Unchecked) {
          if(index.row() == 0) {
            for(int i = 1; i < count(); i++) {
              setItemData(i, Qt::Unchecked, Qt::CheckStateRole);
            }
          } else {
             setItemData(0, Qt::Unchecked, Qt::CheckStateRole);
          }
        }
        item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
        blockSignals(false);
        updateCheckedItems();
      }
      return true;
    }
  }

  return QComboBox::eventFilter(obj_, ev_);
}

void CheckableComboBox::setCheckedData(const QStringList& values_) {
  blockSignals(true);
  setItemData(0, values_.isEmpty() ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
  for(int i = 1; i < count(); i++) {
    setItemData(i, values_.contains(itemData(i).toString()) ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
  }
  blockSignals(false);
  updateCheckedItems();
}

// have to update the checked item state after setting the custom text
void CheckableComboBox::setCheckedDataText(const QString& text_) {
  blockSignals(true);
  const auto values = text_.split(m_separator);
  setItemData(0, text_.isEmpty() ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
  for(int i = 1; i < count(); i++) {
    const bool hasValue = values.contains(itemData(i).toString());
    setItemData(i, hasValue ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
  }
  blockSignals(false);
  updateCheckedItems();
}

void CheckableComboBox::resizeEvent(QResizeEvent* ev_) {
  QComboBox::resizeEvent(ev_);
  updateDisplayText();
}

void CheckableComboBox::updateCheckedItems() {
  const QStringList items = checkedItems();
  updateDisplayText();
  Q_EMIT checkedItemsChanged(items);
}

void CheckableComboBox::updateDisplayText() {
  // There is only a line edit if the combobox is in editable state
  if(!lineEdit()) return;

  const QString oldText = lineEdit()->text();
  QString text;
  QStringList items = checkedItems();
  items.removeOne(QString()); // skip empty string
  if(!items.isEmpty()) {
    text = items.join(m_separator);
  }
  if(oldText != text) {
    setEditText(text);
  }
}
