/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "modelmanager.h"

#include <qglobal.h>

using Tellico::ModelManager;

ModelManager* ModelManager::self() {
  static ModelManager manager;
  return &manager;
}

ModelManager::ModelManager() : m_entryModel(nullptr), m_groupModel(nullptr) {
}

QAbstractItemModel* ModelManager::entryModel() {
  Q_ASSERT(m_entryModel);
  return m_entryModel;
}

void ModelManager::setEntryModel(QAbstractItemModel* model_) {
  Q_ASSERT(model_);
  m_entryModel = model_;
}

QAbstractItemModel* ModelManager::groupModel() {
  Q_ASSERT(m_groupModel);
  return m_groupModel;
}

void ModelManager::setGroupModel(QAbstractItemModel* model_) {
  Q_ASSERT(model_);
  m_groupModel = model_;
}
