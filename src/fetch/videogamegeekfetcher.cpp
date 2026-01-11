/***************************************************************************
    Copyright (C) 2017-2021 Robby Stephenson <robby@periapsis.org>
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

#include "videogamegeekfetcher.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

using namespace Tellico;
using Tellico::Fetch::VideoGameGeekFetcher;

VideoGameGeekFetcher::VideoGameGeekFetcher(QObject* parent_)
    : AbstractBGGFetcher(parent_) {
}

bool VideoGameGeekFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

Tellico::Fetch::ConfigWidget* VideoGameGeekFetcher::configWidget(QWidget* parent_) const {
  return new VideoGameGeekFetcher::ConfigWidget(parent_, this);
}

QString VideoGameGeekFetcher::defaultName() {
  return QStringLiteral("VideoGameGeek");
}

QString VideoGameGeekFetcher::defaultIcon() {
  return QStringLiteral(":/icons/boardgamegeek");
}

Tellico::StringHash VideoGameGeekFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("videogamegeek-link")] = i18n("VideoGameGeek Link");
  return hash;
}

QString VideoGameGeekFetcher::bggType() const {
  // there's an evident bug where the videogameexpansion type is no longer used
  // but some video game ids like 139806 still identify as expansions
  // but videogamepexansion type can't be included in general search without an error
  return QStringLiteral("videogame");
}

VideoGameGeekFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const VideoGameGeekFetcher* fetcher_)
    : AbstractBGGFetcher::ConfigWidget(parent_) {
  addFieldsWidget(allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString VideoGameGeekFetcher::ConfigWidget::preferredName() const {
  return VideoGameGeekFetcher::defaultName();
}
