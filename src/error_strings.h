/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef ERROR_STRINGS_H
#define ERROR_STRINGS_H

#include <klocale.h>

/*
 * @author Robby Stephenson
 * @version $Id: error_strings.h 469 2004-02-18 03:03:59Z robby $
 */

static const char* loadError = I18N_NOOP("Bookcase is unable to load the file - %1.");
static const char* writeError = I18N_NOOP("Bookcase is unable to write the file - %1.");
static const char* uploadError = I18N_NOOP("Bookcase is unable to upload the file - %1.");

#endif
