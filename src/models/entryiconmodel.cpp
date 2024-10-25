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
#include "../constants.h"
#include "../collectionfactory.h"
#include "../config/tellico_config.h"
#include "../tellico_debug.h"

#include <QIcon>
#include <QPixmap>

using namespace Tellico;
using Tellico::EntryIconModel;

EntryIconModel::EntryIconModel(QObject* parent_) : QIdentityProxyModel(parent_) {
  myLog() << "Setting max icon cache cost:" << Config::iconCacheSize();
  m_iconCache.setMaxCost(Config::iconCacheSize());
  connect(this, &QAbstractItemModel::dataChanged, [this](const QModelIndex& topLeft, const QModelIndex& botRight) {
    for(auto i = topLeft.row(); i <= botRight.row(); ++i) {
      m_updatedRows.insert(i);
    }
  });
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
      Data::EntryPtr entry = index_.data(EntryPtrRole).value<Data::EntryPtr>();
      if(!entry) {
        return QVariant();
      }

      // return entry primary image in this case
      Data::FieldPtr field = entry->collection()->primaryImageField();
      if(!field) {
        return defaultIcon(entry->collection());
      }
      const QString id = entry->field(field);
      if(m_iconCache.contains(id)) {
        if(m_updatedRows.contains(index_.row())) {
          delete m_iconCache.take(id);
        } else {
          return QIcon(*m_iconCache.object(id));
        }
      }
      m_updatedRows.remove(index_.row());

      QVariant v = QIdentityProxyModel::data(index_, PrimaryImageRole);
      if(v.isNull() || !v.canConvert<QPixmap>()) {
        return defaultIcon(entry->collection());
      }

      QPixmap p = v.value<QPixmap>();
      if(p.height() > MAX_ENTRY_ICON_SIZE || p.width() > MAX_ENTRY_ICON_SIZE) {
        p = p.scaled(MAX_ENTRY_ICON_SIZE, MAX_ENTRY_ICON_SIZE, Qt::KeepAspectRatioByExpanding);
      }
      QIcon* icon = new QIcon(p);
      if(!m_iconCache.insert(id, icon)) {
        // failing to insert invalidates the icon pointer
        myDebug() << "failed to insert into icon cache";
        return QIcon(p);
      }
      return QIcon(*icon);
    }
  }

  return QIdentityProxyModel::data(index_, role_);
}

void EntryIconModel::clearCache() {
  m_iconCache.clear();
  m_updatedRows.clear();
}

const QIcon& EntryIconModel::defaultIcon(Data::CollPtr coll_) const {
  QIcon* icon = m_defaultIcons.value(coll_->type());
  if(icon) {
    return *icon;
  }
  QIcon tmpIcon = QIcon(QLatin1String(":/icons/nocover_") + CollectionFactory::typeName(coll_->type()));
  // apparently, for a resource icon that doesn't exist, it may not be null, but just have no available sizes
  if(tmpIcon.isNull() || tmpIcon.availableSizes().isEmpty()) {
//    myLog() << "null nocover image, loading tellico.png";
    tmpIcon = QIcon::fromTheme(QStringLiteral("tellico"), QIcon(QLatin1String(":/icons/tellico")));
  }

  icon = new QIcon(tmpIcon);
  m_defaultIcons.insert(coll_->type(), icon);
  return *icon;
}
