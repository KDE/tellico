/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include <klocale.h>

const char* Tellico::errorOpen = I18N_NOOP("Tellico is unable to open the file - %1.");
const char* Tellico::errorLoad = I18N_NOOP("Tellico is unable to load the file - %1.");
const char* Tellico::errorWrite = I18N_NOOP("Tellico is unable to write the file - %1.");
const char* Tellico::errorUpload = I18N_NOOP("Tellico is unable to upload the file - %1.");
const char* Tellico::errorAppendType = I18N_NOOP("Only collections with the same type of entries as "
                                                 "the current one can be appended. No changes are being "
                                                 "made to the current collection.");
const char* Tellico::errorMergeType = I18N_NOOP("Only collections with the same type of entries as "
                                                "the current one can be merged. No changes are being "
                                                "made to the current collection.");
const char* Tellico::errorImageLoad = I18N_NOOP("Tellico is unable to load an image from the file - %1.");

const char* Tellico::untitledFilename = I18N_NOOP("Untitled");
