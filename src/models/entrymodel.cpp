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
#include "../entry.h"
#include "../field.h"
#include "../document.h"
#include "../images/image.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

namespace {
  static const int ENTRYMODEL_IMAGE_HEIGHT = 64;
  // number of entries in a list considered to be "small" in that
  // faster to do individual operations than model reset
  static const int SMALL_OPERATION_ENTRY_SIZE = 10;
}

using namespace Tellico;
using Tellico::EntryModel;

EntryModel::EntryModel(QObject* parent) : QAbstractItemModel(parent),
    m_imagesAreAvailable(false) {
  m_checkPix = QIcon::fromTheme(QStringLiteral("checkmark"), QIcon(QLatin1String(":/icons/checkmark")));
  connect(ImageFactory::self(), &ImageFactory::imageAvailable, this, &EntryModel::refreshImage);
}

EntryModel::~EntryModel() {
}

int EntryModel::rowCount(const QModelIndex& index_) const {
  // valid indexes have no children/rows
  if(index_.isValid()) {
    return 0;
  }
  // even if entries are included, if there are no fields, then no rows either
  return m_fields.isEmpty() ? 0 : m_entries.count();
}

int EntryModel::columnCount(const QModelIndex& index_) const {
  // valid indexes have no columns
  if(index_.isValid()) {
    return 0;
  }
  return m_fields.count();
}

QModelIndex EntryModel::index(int row_, int column_, const QModelIndex& parent_) const {
  return hasIndex(row_, column_, parent_) ? createIndex(row_, column_) : QModelIndex();
}

QModelIndex EntryModel::parent(const QModelIndex&) const {
  return QModelIndex();
}

QVariant EntryModel::headerData(int section_, Qt::Orientation orientation_, int role_) const {
  if(section_ < 0 || section_ >= m_fields.count() || orientation_ != Qt::Horizontal) {
    return QVariant();
  }
  switch(role_) {
    case Qt::DisplayRole:
      return m_fields.at(section_)->title();

    case FieldPtrRole:
      return QVariant::fromValue(m_fields.at(section_));
  }
  return QVariant();
}

QVariant EntryModel::data(const QModelIndex& index_, int role_) const {
  if(!index_.isValid()) {
    return QVariant();
  }

  if(index_.row() >= rowCount()) {
    return QVariant();
  }

  Data::EntryPtr entry;
  Data::FieldPtr field;

  QString value;

  switch(role_) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
      field = this->field(index_);
      if(!field ||
         field->type() == Data::Field::Image ||
         field->type() == Data::Field::Bool) {
        return QVariant();
      }
      entry = this->entry(index_);
      if(!entry) {
        return QVariant();
      }
      value = entry->formattedField(field);
      return value.isEmpty() ? QVariant() : value;

    case Qt::DecorationRole:
      field = this->field(index_);
      if(!field) {
        return QVariant();
      }
      entry = this->entry(index_);
      if(!entry) {
        return QVariant();
      }

      // just return the image for the entry
      // we don't need a formatted value for any pixmaps
      value = entry->field(field);
      if(value.isEmpty()) {
        return QVariant();
      }

      if(field->type() == Data::Field::Bool) {
        // assume any non-empty value equals true
        return m_checkPix;
      }

      if(field->type() == Data::Field::Image) {
        // convert pixmap to icon
        QVariant v = requestImage(entry, value);
        if(!v.isNull() && v.canConvert<QPixmap>()) {
          return QIcon(v.value<QPixmap>());
        }
      }
      return QVariant();

    case PrimaryImageRole:
      // return the primary image for the entry, no matter the index column
      entry = this->entry(index_);
      if(!entry) {
        return QVariant();
      }
      field = entry->collection()->primaryImageField();
      if(!field) {
        return QVariant();
      }
      value = entry->field(field);
      if(value.isEmpty()) {
        return QVariant();
      }
      return requestImage(entry, value);

    case EntryPtrRole:
      entry = this->entry(index_);
      if(!entry) {
        return QVariant();
      }
      return QVariant::fromValue(entry);

    case FieldPtrRole:
      field = this->field(index_);
      if(!field) {
        return QVariant();
      }
      return QVariant::fromValue(field);

    case SaveStateRole:
      if(!m_saveStates.contains(index_.row())) {
        return NormalState;
      }
      return m_saveStates.value(index_.row());

    case Qt::TextAlignmentRole:
      field = this->field(index_);
      if(!field) {
        return QVariant();
      }
      // special-case a few types to align center, default otherwise
      if(field->type() == Data::Field::Bool ||
         field->type() == Data::Field::Number ||
         field->type() == Data::Field::Image ||
         field->type() == Data::Field::Rating) {
        return Qt::AlignCenter;
      }
      return QVariant();

    case Qt::SizeHintRole:
      field = this->field(index_);
      if(field && field->type() == Data::Field::Image) {
        return QSize(0, ENTRYMODEL_IMAGE_HEIGHT+4);
      }
      return QVariant();
  }
  return QVariant();
}

QModelIndex EntryModel::indexFromEntry(Data::EntryPtr entry_) const {
  const int idx = m_entries.indexOf(entry_);
  if(idx == -1) {
    return QModelIndex();
  }
  return createIndex(idx, 0);
}

Tellico::Data::EntryPtr EntryModel::entry(const QModelIndex& index_) const {
  Q_ASSERT(index_.isValid());
  Data::EntryPtr entry;
  if(index_.isValid() && index_.row() < m_entries.count()) {
    entry = m_entries.at(index_.row());
  }
  return entry;
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
  beginResetModel();
  m_entries.clear();
  m_fields.clear();
  m_saveStates.clear();
  endResetModel();
}

void EntryModel::clearSaveState() {
  // if there are many save states to be toggled, do a full model reset
  if(m_saveStates.size() > SMALL_OPERATION_ENTRY_SIZE) {
    beginResetModel();
    m_saveStates.clear();
    endResetModel();
  } else {
    QHashIterator<int, int> i(m_saveStates);
    while(i.hasNext()) {
      i.next();
      // If the hash is modified while a QHashIterator is active, the QHashIterator
      // will continue iterating over the original hash, ignoring the modified copy.
      m_saveStates.remove(i.key());
      QModelIndex idx1 = createIndex(i.key(), 0);
      QModelIndex idx2 = createIndex(i.key(), m_fields.count());
      emit dataChanged(idx1, idx2, QVector<int>() << SaveStateRole);
    }
  }
}

void EntryModel::setEntries(const Tellico::Data::EntryList& entries_) {
  // should never have entries without having fields first
  Q_ASSERT(!m_fields.isEmpty() || entries_.isEmpty());
  beginResetModel();
  m_entries = entries_;
  endResetModel();
}

void EntryModel::addEntries(const Tellico::Data::EntryList& entries_) {
  beginInsertRows(QModelIndex(), m_entries.count(), m_entries.count() + entries_.count() - 1);
  m_entries += entries_;
  endInsertRows();
}

void EntryModel::modifyEntries(const Tellico::Data::EntryList& entries_) {
  foreach(Data::EntryPtr entry, entries_) {
    QModelIndex index = indexFromEntry(entry);
    if(index.isValid()) {
      emit dataChanged(index, index);
    }
  }
}

void EntryModel::removeEntries(const Tellico::Data::EntryList& entries_) {
  // for performance reasons, if more than 10 entries are being removed, rather than
  // iterating over all of them, which really hurts, just signal a full replacement
  const bool bigRemoval = (entries_.size() > SMALL_OPERATION_ENTRY_SIZE);
  if(bigRemoval) {
    beginResetModel();
  }
  foreach(Data::EntryPtr entry, entries_) {
    int idx = m_entries.indexOf(entry);
    if(idx > -1) {
      if(!bigRemoval) {
        beginRemoveRows(QModelIndex(), idx, idx);
      }
      m_entries.removeAt(idx);
      if(!bigRemoval) {
        endRemoveRows();
      }
    }
  }
  if(bigRemoval) {
    endResetModel();
  }
}

void EntryModel::setFields(const Tellico::Data::FieldList& fields_) {
  // if fields are being replaced, it's a full model reset
  // if not, it's just adding columns
  if(!m_fields.isEmpty()) {
    beginResetModel();
    m_fields = fields_;
    endResetModel();
  } else if(!fields_.isEmpty()) {
    beginInsertColumns(QModelIndex(), 0, fields_.size()-1);
    m_fields = fields_;
    endInsertColumns();
  }
}

void EntryModel::reorderFields(const Tellico::Data::FieldList& fields_) {
  emit layoutAboutToBeChanged();
  // update the persistent model indexes by building list of old index
  // and new if the columns are moved
  QModelIndexList oldPersistentList = persistentIndexList();
  QModelIndexList fromList, toList;
  for(int i = 0; i < m_fields.count(); ++i) {
    const int j = fields_.indexOf(m_fields.at(i));
    Q_ASSERT(j >= 0);
    // old add the model index list if the columns are different
    if(i != j) {
      foreach(QModelIndex oldIndex, oldPersistentList) {
        if(oldIndex.column() == i) {
          fromList += oldIndex;
          toList += createIndex(oldIndex.row(), j);
        }
      }
    }
  }

  m_fields = fields_;

  changePersistentIndexList(fromList, toList);
  emit layoutChanged();
}

void EntryModel::addFields(const Tellico::Data::FieldList& fields_) {
  if(!fields_.isEmpty()) {
    beginInsertColumns(QModelIndex(), m_fields.size(), m_fields.size() + fields_.size()-1);
    m_fields += fields_;
    endInsertColumns();
  }
}

void EntryModel::modifyField(Data::FieldPtr oldField_, Data::FieldPtr newField_) {
  for(int i = 0; i < m_fields.count(); ++i) {
    if(m_fields.at(i)->name() == oldField_->name()) {
      m_fields.replace(i, newField_);
      emit headerDataChanged(Qt::Horizontal, i, i);
      break;
    }
  }
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

void EntryModel::setImagesAreAvailable(bool available_) {
  if(m_imagesAreAvailable != available_) {
    beginResetModel();
    m_imagesAreAvailable = available_;
    endResetModel();
  }
}

QVariant EntryModel::requestImage(Data::EntryPtr entry_, const QString& id_) const {
  if(!m_imagesAreAvailable) {
    return QVariant();
  }
  // if it's not a local image, request that it be downloaded
  if(ImageFactory::hasLocalImage(id_)) {
    const Data::Image& img = ImageFactory::imageById(id_);
    if(!img.isNull()) {
      return img.convertToPixmap();
    }
  } else if(!m_requestedImages.contains(id_, entry_)) {
    m_requestedImages.insert(id_, entry_);
    ImageFactory::requestImageById(id_);
  }
  return QVariant();
}

void EntryModel::refreshImage(const QString& id_) {
  QMultiHash<QString, Data::EntryPtr>::iterator i = m_requestedImages.find(id_);
  while(i != m_requestedImages.end() && i.key() == id_) {
    QModelIndex index = indexFromEntry(i.value());
    emit dataChanged(index, index);
    ++i;
  }
  m_requestedImages.remove(id_);
}
