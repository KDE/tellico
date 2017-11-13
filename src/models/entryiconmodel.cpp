/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "entryiconmodel.h"
#include "models.h"
#include "../images/image.h"
#include "../images/imagefactory.h"
#include "../collectionfactory.h"
#include "../config/tellico_config.h"
#include "../tellico_debug.h"

#include <QIcon>

using Tellico::EntryIconModel;

EntryIconModel::EntryIconModel(QObject* parent_) : QIdentityProxyModel(parent_) {
  m_iconCache.setMaxCost(Config::iconCacheSize());
}

EntryIconModel::~EntryIconModel() {
  qDeleteAll(m_defaultIcons);
}

void EntryIconModel::setSourceModel(QAbstractItemModel* newSourceModel_) {
  QIdentityProxyModel::setSourceModel(newSourceModel_);
  if(newSourceModel_) {
    connect(newSourceModel_, &QAbstractItemModel::modelReset, this, &EntryIconModel::clearCache);
  }
}

QVariant EntryIconModel::data(const QModelIndex& index_, int role_) const {
  switch(role_) {
    // this IdentityModel serves to return the entry's primary image as the DecorationRole
    // no matter what the index column may be
    case Qt::DecorationRole:
    {
      Data::FieldPtr field = index_.data(FieldPtrRole).value<Data::FieldPtr>();
      if(!field) {
        return QVariant();
      }
      Data::EntryPtr entry = index_.data(EntryPtrRole).value<Data::EntryPtr>();
      if(!entry) {
        return QVariant();
      }

      // return entry image in this case
      const QString fieldName = entry->collection()->primaryImageField();
//      if(fieldName.isEmpty() || !m_imagesAreAvailable) {
      if(fieldName.isEmpty()) {
        return defaultIcon(entry->collection());
      }
      const QString id = entry->field(fieldName);
      QIcon* icon = m_iconCache.object(id);
      if(icon) {
        return QIcon(*icon);
      }
      // if it's not a local image, request that it be downloaded
      if(!ImageFactory::hasLocalImage(id)) {
        // TODO: figure out how to handle image requests as in EntryModel
        return defaultIcon(entry->collection());
      }
      const Data::Image& img = ImageFactory::imageById(id);
      if(img.isNull()) {
        return defaultIcon(entry->collection());
      }

      icon = new QIcon(img.convertToPixmap());
      if(!m_iconCache.insert(id, icon)) {
        // failing to insert invalidates the icon pointer
        return QIcon(img.convertToPixmap());
      }
      return QIcon(*icon);
    }
  }

  return QIdentityProxyModel::data(index_, role_);
}

void EntryIconModel::clearCache() {
  m_iconCache.clear();
}

const QIcon& EntryIconModel::defaultIcon(Data::CollPtr coll_) const {
  QIcon* icon = m_defaultIcons.value(coll_->type());
  if(icon) {
    return *icon;
  }
  QIcon tmpIcon = QIcon(QLatin1String(":/icons/nocover_") + CollectionFactory::typeName(coll_->type()));
  if(tmpIcon.isNull()) {
    myLog() << "null nocover image, loading tellico.png";
    tmpIcon = QIcon::fromTheme(QLatin1String("tellico"), QIcon(QLatin1String(":/icons/tellico")));
  }

  icon = new QIcon(tmpIcon);
  m_defaultIcons.insert(coll_->type(), icon);
  return *icon;
}
