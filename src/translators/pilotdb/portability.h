/*
 * palm-db-tools: Support Library: String Parsing Utility Functions
 * Copyright (C) 1999-2000 by Tom Dyas (tdyas@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LIBSUPPORT_PORTABILITY_H__
#define __LIBSUPPORT_PORTABILITY_H__

/*
 * Pull in the correct configuration header.
 */

#ifndef WIN32
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#else
#include "win32/win32-config.h"
#endif

#ifdef _MSC_VER
/* Borrowed from GLib: Make MSVC more pedantic, this is a recommended
 * pragma list from _Win32_Programming_ by Rector and Newcomer.
 */
#pragma warning(error:4002)
#pragma warning(error:4003)
#pragma warning(1:4010)
#pragma warning(error:4013)
#pragma warning(1:4016)
#pragma warning(error:4020)
#pragma warning(error:4021)
#pragma warning(error:4027)
#pragma warning(error:4029)
#pragma warning(error:4033)
#pragma warning(error:4035)
#pragma warning(error:4045)
#pragma warning(error:4047)
#pragma warning(error:4049)
#pragma warning(error:4053)
#pragma warning(error:4071)
#pragma warning(disable:4101)
#pragma warning(error:4150)
 
#pragma warning(disable:4244)   /* No possible loss of data warnings */
#pragma warning(disable:4305)   /* No truncation from int to char warnings */
#endif /* _MSC_VER */

/* MSVC is screwed up when it comes to calling base class virtual
 * functions from a subclass. Thus, the following macro which makes
 * calling the superclass nice and simple.
 */
#ifndef _MSC_VER
#define SUPERCLASS(namespace, class, function, args) namespace::class::function args
#else
#define SUPERCLASS(namespace, class, function, args) this-> class::function args
#endif

#endif
