/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "xmphandler.h"
#include "../tellico_debug.h"

#include <config.h>

#include <QFile>
#include <QTextStream>

#ifdef HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

using Tellico::XMPHandler;

int XMPHandler::s_initCount = 0;

bool XMPHandler::isXMPEnabled() {
#ifdef HAVE_EXEMPI
  return true;
#else
  return false;
#endif
}

XMPHandler::XMPHandler() {
  init();
}

XMPHandler::~XMPHandler() {
#ifdef HAVE_EXEMPI
  --s_initCount;
  if(s_initCount == 0) {
    xmp_terminate();
  }
#endif
}

void XMPHandler::init() {
#ifdef HAVE_EXEMPI
  if(s_initCount == 0) {
    xmp_init();
  }
  ++s_initCount;
#endif
}

QString XMPHandler::extractXMP(const QString& file) {
  QString result;
#ifdef HAVE_EXEMPI
  XmpFilePtr xmpfile = xmp_files_open_new(QFile::encodeName(file), XMP_OPEN_READ);
  if(!xmpfile) {
    myDebug() << "unable to open " << file;
    return result;
  }
  XmpPtr xmp = xmp_files_get_new_xmp(xmpfile);
  if(xmp) {
    XmpStringPtr buffer = xmp_string_new();
    xmp_serialize(xmp, buffer, 0, 0);
    result = QString::fromUtf8(xmp_string_cstr(buffer));
    xmp_string_free(buffer);
//    myDebug() << result;
#if 0
    myWarning() << "turn me off!";
    QFile f1(QLatin1String("/tmp/xmp.xml"));
    if(f1.open(QIODevice::WriteOnly)) {
      QTextStream t(&f1);
      t << result;
    }
    f1.close();
#endif
    xmp_free(xmp);
    xmp_files_close(xmpfile, XMP_CLOSE_NOOPTION);
    xmp_files_free(xmpfile);
  } else {
    myDebug() << "unable to parse " << file;
  }
#endif
  return result;
}
