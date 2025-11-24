/***************************************************************************
    Copyright (C) 2014-2021 Robby Stephenson <robby@periapsis.org>
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

#include "boardgamegeekfetcher.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

using namespace Tellico;
using Tellico::Fetch::BoardGameGeekFetcher;

BoardGameGeekFetcher::BoardGameGeekFetcher(QObject* parent_)
    : AbstractBGGFetcher(parent_) {
}

bool BoardGameGeekFetcher::canFetch(int type) const {
  return type == Data::Collection::BoardGame;
}

Tellico::Fetch::ConfigWidget* BoardGameGeekFetcher::configWidget(QWidget* parent_) const {
  return new BoardGameGeekFetcher::ConfigWidget(parent_, this);
}

QString BoardGameGeekFetcher::defaultName() {
  return QStringLiteral("BoardGameGeek");
}

QString BoardGameGeekFetcher::defaultIcon() {
  return favIcon("https://cf.geekdo-static.com/icons/favicon2.ico");
}

Tellico::StringHash BoardGameGeekFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("artist")]             = i18nc("Comic Book Illustrator", "Artist");
  hash[QStringLiteral("boardgamegeek-link")] = i18n("BoardGameGeek Link");
  return hash;
}

QString BoardGameGeekFetcher::bggType() const {
  return QStringLiteral("boardgame,boardgameexpansion");
}

BoardGameGeekFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const BoardGameGeekFetcher* fetcher_)
   : AbstractBGGFetcher::ConfigWidget(parent_) {
  addFieldsWidget(allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString BoardGameGeekFetcher::ConfigWidget::preferredName() const {
  return BoardGameGeekFetcher::defaultName();
}
