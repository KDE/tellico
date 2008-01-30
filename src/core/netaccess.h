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

#include <qobject.h>
#include <qpixmap.h>

class KURL;
class KFileItem;
namespace KIO {
  class Job;
}

namespace Tellico {

class NetAccess : public QObject {
Q_OBJECT

public:
  static bool download(const KURL& u, QString& target, QWidget* window);
  static void removeTempFile(const QString& name);
  static QPixmap filePreview(const KURL& fileName, int size=196);
  static QPixmap filePreview(KFileItem* item, int size=196);

private slots:
  void slotPreview(const KFileItem* item, const QPixmap& pix);

private:
  static QStringList* s_tmpFiles;

  QPixmap m_preview;
};

}
#endif
