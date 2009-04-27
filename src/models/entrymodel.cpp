/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entrymodel.h"
#include "models.h"
#include "../collection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../entry.h"
#include "../field.h"
#include "../images/image.h"
#include "../document.h"
#include "../tellico_debug.h"
#include "../gui/ratingwidget.h"

#include <kicon.h>
#include <klocale.h>

using Tellico::EntryModel;

EntryModel::EntryModel(QObject* parent) : AbstractEntryModel(parent), m_checkPix(QLatin1String("checkmark")) {
}

EntryModel::~EntryModel() {
}

int EntryModel::columnCount(const QModelIndex&) const {
  return m_fields.count();
}

QVariant EntryModel::headerData(int section_, Qt::Orientation orientation_, int role_) const {
  if(section_ >= m_fields.count() || orientation_ != Qt::Horizontal || role_ != Qt::DisplayRole) {
    return QVariant();
  }
  return m_fields.at(section_)->title();
}

QVariant EntryModel::data(const QModelIndex& index_, int role_) const {
  if(!index_.isValid()) {
    return QVariant();
  }

  if(index_.row() >= rowCount()) {
    return QVariant();
  }

  Data::EntryPtr entry = this->entry(index_);
  if(!entry) {
    return QVariant();
  }

  Data::FieldPtr field = this->field(index_);
  if(!field) {
    return QVariant();
  }

  QString value = entry->field(field);
  if(value.isEmpty()) {
    return QVariant();
  }

  switch(role_) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
      if(field->type() != Data::Field::Image &&
         field->type() != Data::Field::Bool &&
         field->type() != Data::Field::Rating) {
        return value;
      }
      return QVariant();

    case Qt::DecorationRole:
      if(field->type() == Data::Field::Image) {
        const Data::Image& img = ImageFactory::imageById(value);
        if(!img.isNull()) {
          return KIcon(QPixmap::fromImage(img));
        }
      } else if(field->type() == Data::Field::Bool) {
        return m_checkPix;
      } else if(field->type() == Data::Field::Rating) {
        return GUI::RatingWidget::pixmap(value);
      }
      return QVariant();

    case EntryPtrRole:
      return qVariantFromValue(entry);

    case FieldPtrRole:
      return qVariantFromValue(field);

    case SaveStateRole:
      if(!m_saveStates.contains(index_.row())) {
        return NormalState;
      }
      return m_saveStates.value(index_.row());

    case Qt::TextAlignmentRole:
      // special-case a few types to align center, default otherwise
      if(field->type() == Data::Field::Bool ||
         field->type() == Data::Field::Number ||
         field->type() == Data::Field::Image ||
         field->type() == Data::Field::Rating) {
        return Qt::AlignCenter;
      }
      return QVariant();
  }
  return QVariant();
}

Tellico::Data::FieldPtr EntryModel::field(const QModelIndex& index_) const {
  Q_ASSERT(index_.isValid());
  Q_ASSERT(index_.column() < m_fields.count());

  Data::FieldPtr field;
  if(index_.isValid() && index_.column() < m_fields.count()) {
    field = m_fields.at(index_.column());
  }
  return field;
}

bool EntryModel::setData(const QModelIndex& index_, const QVariant& value_, int role_) {
  if(!index_.isValid() || role_ != SaveStateRole) {
    return false;
  }
  const int state = value_.toInt();
  if(state == NormalState) {
    m_saveStates.remove(index_.row());
  } else {
    Q_ASSERT(state == NewState || state == ModifiedState);
    m_saveStates.insert(index_.row(), value_.toInt());
  }
  emit dataChanged(index_, index_);
  return true;
}

void EntryModel::clear() {
  m_fields.clear();
  m_saveStates.clear();
  AbstractEntryModel::clear();
}

void EntryModel::clearSaveState() {
  m_saveStates.clear();
  reset();
}

void EntryModel::setFields(const Tellico::Data::FieldList& fields_) {
  if(!m_fields.isEmpty()) {
    beginRemoveColumns(QModelIndex(), 0, m_fields.size()-1);
    endRemoveColumns();
  }
  if(!fields_.isEmpty()) {
    beginInsertColumns(QModelIndex(), 0, fields_.size()-1);
  }
  m_fields = fields_;
  if(!fields_.isEmpty()) {
    endInsertColumns();
  }
}

void EntryModel::addFields(const Tellico::Data::FieldList& fields_) {
  beginInsertColumns(QModelIndex(), m_fields.size(), m_fields.size());
  m_fields += fields_;
  endInsertColumns();
}

void EntryModel::modifyFields(const Tellico::Data::FieldList& fields_) {
  Q_UNUSED(fields_);
  reset();
}

void EntryModel::removeFields(const Tellico::Data::FieldList& fields_) {
  foreach(Data::FieldPtr field, fields_) {
    int idx = m_fields.indexOf(field);
    if(idx > -1) {
      beginRemoveColumns(QModelIndex(), idx, idx);
      m_fields.removeAt(idx);
      endRemoveColumns();
    }
  }
}

#include "entrymodel.moc"
