/***************************************************************************
    Copyright (C) 2003-2021 Robby Stephenson <robby@periapsis.org>
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

#include "tellico_strings.h"

KLazyLocalizedString Tellico::errorOpen = kli18n("Tellico is unable to open the file - %1.");
KLazyLocalizedString Tellico::errorLoad = kli18n("Tellico is unable to load the file - %1.");
KLazyLocalizedString Tellico::errorWrite = kli18n("Tellico is unable to write the file - %1.");
KLazyLocalizedString Tellico::errorUpload = kli18n("Tellico is unable to upload the file - %1.");
KLazyLocalizedString Tellico::errorAppendType = kli18n("Only collections with the same type of entries as "
                                                       "the current one can be appended. No changes are being "
                                                       "made to the current collection.");
KLazyLocalizedString Tellico::errorMergeType = kli18n("Only collections with the same type of entries as "
                                                      "the current one can be merged. No changes are being "
                                                      "made to the current collection.");
KLazyLocalizedString Tellico::errorImageLoad = kli18n("Tellico is unable to load an image from the file - %1.");

KLazyLocalizedString Tellico::untitledFilename = kli18n("Untitled");
KLazyLocalizedString Tellico::providedBy = kli18n("This information was freely provided by <a href=\"%1\">%2</a>.");

KLazyLocalizedString Tellico::categoryGeneral        = kli18n("General");
KLazyLocalizedString Tellico::categoryFeatures       = kli18n("Features");
KLazyLocalizedString Tellico::categoryPeople         = kli18n("Other People");
KLazyLocalizedString Tellico::categoryPublishing     = kli18n("Publishing");
KLazyLocalizedString Tellico::categoryClassification = kli18n("Classification");
KLazyLocalizedString Tellico::categoryCondition      = kli18n("Condition");
KLazyLocalizedString Tellico::categoryPersonal       = kli18n("Personal");
KLazyLocalizedString Tellico::categoryMisc           = kli18n("Miscellaneous");
