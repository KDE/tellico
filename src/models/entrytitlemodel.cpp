/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
    Copyright (C) 2011 Pedro Miguel Carvalho <kde@pmc.com.pt>
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

#include "entrytitlemodel.h"
#include "../collection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../entry.h"
#include "../field.h"
#include "../images/image.h"
#include "../core/tellico_config.h"
#include "../tellico_debug.h"

using Tellico::EntryTitleModel;

EntryTitleModel::EntryTitleModel(QObject* parent) : AbstractEntryModel(parent) {
  m_iconCache.setMaxCost(Config::iconCacheSize());
}

EntryTitleModel::~EntryTitleModel() {
  qDeleteAll(m_defaultIcons.values());
}

int EntryTitleModel::columnCount(const QModelIndex&) const {
  return rowCount() > 0 ? entry(createIndex(0, 0))->collection()->fields().count() : 1;
}

// TODO: combine this with EntryModel::headerData into AnstractEntryModel
QVariant EntryTitleModel::headerData(int section_, Qt::Orientation orientation_, int role_) const {
  if(section_ < 0 || section_ >= columnCount() || orientation_ != Qt::Horizontal) {
    return QVariant();
  }
  Data::FieldList fields = rowCount() > 0 ? entry(createIndex(0, 0))->collection()->fields() : Data::FieldList();
  if(fields.isEmpty()) {
    return QVariant();
  }

  switch(role_) {
    case Qt::DisplayRole:
      return fields.at(section_)->title();

    case FieldPtrRole:
      return qVariantFromValue(fields.at(section_));
  }
  return QVariant();
}

QVariant EntryTitleModel::data(const QModelIndex& index_, int role_) const {
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

  const int col = index_.column();
  Data::FieldPtr field;
  if(col > 0) {
    // we need a field value, not just the title
    field = entry->collection()->fields().at(col);
  }

  switch(role_) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
      return field ? entry->field(field) : entry->formattedField(QLatin1String("title"));

    case Qt::DecorationRole: {
      QString fieldName = imageField(entry->collection());
      if(fieldName.isEmpty()) {
        return defaultIcon(entry->collection());
      }
      const QString id = entry->field(fieldName);
      KIcon* icon = m_iconCache.object(id);
      if(icon) {
        return KIcon(*icon);
      }
      const Data::Image& img = ImageFactory::imageById(id);
      if(img.isNull()) {
        return defaultIcon(entry->collection());
      }

      icon = new KIcon(QPixmap::fromImage(img));
      if(!m_iconCache.insert(id, icon)) {
        // failing to insert invalidates the icon pointer
        return KIcon(QPixmap::fromImage(img));
      }
      return KIcon(*icon);
    }
    case EntryPtrRole:
      return qVariantFromValue(entry);
  }
  return QVariant();
}

const KIcon& EntryTitleModel::defaultIcon(Tellico::Data::CollPtr coll_) const {
  KIcon* icon = m_defaultIcons.value(coll_->type());
  if(icon) {
    return *icon;
  }
  KIcon tmpIcon(QLatin1String("nocover_") + CollectionFactory::typeName(coll_->type()));
  if(tmpIcon.isNull()) {
    myLog() << "null nocover image, loading tellico.png";
    tmpIcon = KIcon(QLatin1String("tellico"));
  }

  icon = new KIcon(tmpIcon);
  m_defaultIcons.insert(coll_->type(), icon);
  return *icon;
}

QString EntryTitleModel::imageField(Tellico::Data::CollPtr coll_) const {
  if(!m_imageFields.contains(coll_->id())) {
    const Data::FieldList& fields = coll_->imageFields();
    if(!fields.isEmpty()) {
      m_imageFields.insert(coll_->id(), fields[0]->name());
    }
  }
  return m_imageFields.value(coll_->id());
}

void EntryTitleModel::clear() {
  m_iconCache.clear();
  AbstractEntryModel::clear();
}

#include "entrytitlemodel.moc"
