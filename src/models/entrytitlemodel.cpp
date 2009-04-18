/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entrytitlemodel.h"
#include "../collection.h"
#include "../collectionfactory.h"
#include "../images/imagefactory.h"
#include "../entry.h"
#include "../field.h"
#include "../images/image.h"
#include "../tellico_debug.h"

#include <kicon.h>

using Tellico::EntryTitleModel;

EntryTitleModel::EntryTitleModel(QObject* parent) : AbstractEntryModel(parent) {
}

EntryTitleModel::~EntryTitleModel() {
  qDeleteAll(m_defaultIcons.values());
}

int EntryTitleModel::columnCount(const QModelIndex&) const {
  return rowCount() > 0 ? entry(createIndex(0, 0))->collection()->fields().count() : 1;
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
      return field ? entry->field(field) : entry->title();

    case Qt::DecorationRole: {
      QString fieldName = imageField(entry->collection());
      if(fieldName.isEmpty()) {
        return defaultIcon(entry->collection());
      } else {
        const Data::Image& img = ImageFactory::imageById(entry->field(fieldName));
        if(img.isNull()) {
          return defaultIcon(entry->collection());
        }
        return KIcon(QPixmap::fromImage(img));
      }
      return QVariant();
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

#include "entrytitlemodel.moc"
