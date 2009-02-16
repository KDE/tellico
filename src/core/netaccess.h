/***************************************************************************
    copyright            : (C) 2006-2008 by Robby Stephenson
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

#include <QObject>
#include <QPixmap>

class KUrl;
class KFileItem;
namespace KIO {
  class Job;
}

namespace Tellico {

class NetAccess : public QObject {
Q_OBJECT

public:
  static bool download(const KUrl& u, QString& target, QWidget* window, bool quiet=false);
  static QPixmap filePreview(const KUrl& fileName, int size=196);
  static QPixmap filePreview(const KFileItem& item, int size=196);
  static void removeTempFile(const QString& name);

private slots:
  void slotPreview(const KFileItem* item, const QPixmap& pix);

private:
  QPixmap m_preview;
};

}
#endif
