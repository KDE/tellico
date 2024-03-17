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

#ifndef TELLICO_GUI_CHECKABLECOMBOBOX_H
#define TELLICO_GUI_CHECKABLECOMBOBOX_H

#include <QComboBox>
#include <QMenu>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

class QEvent;

namespace Tellico {
  namespace GUI {

class CheckableItemModel : public QStandardItemModel {
Q_OBJECT

public:
  CheckableItemModel(QObject* parent = nullptr);

  Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  bool setData(const QModelIndex& index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

Q_SIGNALS:
  void itemCheckStateChanged(const QModelIndex& index);
};

class CheckBoxDelegate : public QStyledItemDelegate {
Q_OBJECT

public:
  CheckBoxDelegate(QObject* parent = nullptr);

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const Q_DECL_OVERRIDE;
};

class CheckableComboBox : public QComboBox {
Q_OBJECT

public:
  CheckableComboBox(QWidget* parent = nullptr);

  QString separator() const;
  void setSeparator(const QString& separator);

  QStringList checkedItems() const;
  QVariantList checkedItemsData() const;

  Qt::CheckState itemCheckState(int index) const;
  void setAllCheckState(Qt::CheckState checkState);

  void hidePopup() override;
  bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

Q_SIGNALS:
  void checkedItemsChanged(const QStringList& items);

public Q_SLOTS:
  void setCheckedDataText(const QString& text);
  void setCheckedData(const QStringList& values);

protected:
  void resizeEvent(QResizeEvent* event) override;

protected:

private:
  void updateCheckedItems();
  void updateDisplayText();

  QString m_separator;
  bool m_skipHide = false;
};

  } // end namespace
} //end namespace

#endif
