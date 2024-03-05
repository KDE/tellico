/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#include "entrygroup.h"
#include "entry.h"
#include "utils/string_utils.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KLocalizedString>
#define TC_STR const char*
#define STR_NOOP I18N_NOOP
#define TC_I18N i18n
#else
#include <KLazyLocalizedString>
#define TC_STR KLazyLocalizedString
#define STR_NOOP kli18n
#define TC_I18N(str) str.toString()
#endif

namespace {
  static TC_STR emptyString = STR_NOOP("(Empty)");
}

using Tellico::Data::EntryGroup;

EntryGroup::EntryGroup(const QString& group, const QString& field)
   : EntryList(), m_group(Tellico::shareString(group)), m_field(Tellico::shareString(field)) {
}

EntryGroup::~EntryGroup() {
  // need a copy since we remove ourselves
  EntryList vec = *this;
  foreach(EntryPtr entry, vec) {
    entry->removeFromGroup(this);
  }
}

QString EntryGroup::groupName() const {
  return hasEmptyGroupName() ? emptyGroupName() : m_group;
}

QString EntryGroup::fieldName() const {
  return m_field;
}

bool EntryGroup::hasEmptyGroupName() const {
  return m_group.isEmpty();
}

QString EntryGroup::emptyGroupName() {
  static const QString name = TC_I18N(emptyString);
  return name;
}
