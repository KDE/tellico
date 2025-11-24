/***************************************************************************
    Copyright (C) 2021 Robby Stephenson <robby@periapsis.org>
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

#include "rpggeekfetcher.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

using namespace Tellico;
using Tellico::Fetch::RPGGeekFetcher;

RPGGeekFetcher::RPGGeekFetcher(QObject* parent_)
    : AbstractBGGFetcher(parent_) {
}

bool RPGGeekFetcher::canFetch(int type) const {
  // it's a custom collection
  return type == Data::Collection::Base;
}

Tellico::Fetch::ConfigWidget* RPGGeekFetcher::configWidget(QWidget* parent_) const {
  return new RPGGeekFetcher::ConfigWidget(parent_, this);
}

QString RPGGeekFetcher::defaultName() {
  return QStringLiteral("RPGGeek");
}

QString RPGGeekFetcher::defaultIcon() {
  return favIcon("https://cf.geekdo-static.com/icons/favicon2.ico");
}

Tellico::StringHash RPGGeekFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("genre")] = i18n("Genre");
  hash[QStringLiteral("year")] = i18n("Release Year");
  hash[QStringLiteral("publisher")] = i18n("Publisher");
  hash[QStringLiteral("artist")]  = i18nc("Comic Book Illustrator", "Artist");
  hash[QStringLiteral("designer")] = i18n("Designer");
  hash[QStringLiteral("producer")] = i18n("Producer");
  hash[QStringLiteral("mechanism")] = i18n("Mechanism");
  hash[QStringLiteral("description")] = i18n("Description");
  hash[QStringLiteral("rpggeek-link")] = i18n("RPGGeek Link");
  return hash;
}

QString RPGGeekFetcher::bggType() const {
  return QStringLiteral("rpgitem");
}

RPGGeekFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const RPGGeekFetcher* fetcher_)
   : AbstractBGGFetcher::ConfigWidget(parent_) {
  addFieldsWidget(allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString RPGGeekFetcher::ConfigWidget::preferredName() const {
  return RPGGeekFetcher::defaultName();
}
