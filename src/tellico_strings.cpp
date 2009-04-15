/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellico_strings.h"

#include <klocale.h>

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