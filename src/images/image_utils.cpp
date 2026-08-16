/***************************************************************************
    Copyright (C) 2026 Robby Stephenson <robby@periapsis.org>
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

#include "image_utils.h"
#include "../config/tellico_config.h"
#include "../utils/gradient.h"

#include <KColorUtils>

QImage Tellico::gradientImage(Tellico::GradientImageType gradType_, int collectionType_, const Tellico::StyleOptions& opt_) {
  const QColor& baseColor = opt_.baseColor.isValid()
                          ? opt_.baseColor
                          : Config::templateBaseColor(collectionType_);
  const QColor& highColor = opt_.highlightedBaseColor.isValid()
                          ? opt_.highlightedBaseColor
                          : Config::templateHighlightedBaseColor(collectionType_);

  QImage img;
  switch(gradType_) {
    case GradientBackground:
      img = Tellico::gradient(QSize(600, 1),
                              KColorUtils::mix(baseColor, highColor, 0.3),
                              baseColor,
                              Tellico::PipeCrossGradient);
      img = img.transformed(QTransform().rotate(90));
      break;

    case GradientHeader:
      img = Tellico::unbalancedGradient(QSize(1, 10),
                                        highColor,
                                        KColorUtils::mix(baseColor, highColor, 0.5),
                                        Tellico::VerticalGradient,
                                        100,
                                        -100);
      break;
  }

  return img;
}
