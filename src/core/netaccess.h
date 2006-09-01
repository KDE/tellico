/***************************************************************************
    copyright            : (C) 2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_NETACCESS_H
#define TELLICO_NETACCESS_H

class KURL;
namespace KIO {
  class Job;
}

#include <qobject.h>

namespace Tellico {

class NetAccess : public QObject {
Q_OBJECT

public:
  static bool download(const KURL& u, QString& target, QWidget* window);
  static void removeTempFile(const QString& name);

private:
  static QStringList* s_tmpFiles;
};

}
#endif
