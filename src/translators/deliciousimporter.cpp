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

#include "deliciousimporter.h"
#include "../collection.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include "rtf2html/rtf2html.h"

#include <kstandarddirs.h>

#include <QFile>

using Tellico::Import::DeliciousImporter;

DeliciousImporter::DeliciousImporter(const KUrl& url_) : XSLTImporter(url_) {
  QString xsltFile = KStandardDirs::locate("appdata", QLatin1String("delicious2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    KUrl u;
    u.setPath(xsltFile);
    XSLTImporter::setXSLTURL(u);
  } else {
    kWarning() << "DeliciousImporter() - unable to find delicious2tellico.xml!";
  }
}

bool DeliciousImporter::canImport(int type) const {
  return type == Data::Collection::Book ||
         type == Data::Collection::Video ||
         type == Data::Collection::Game;
}

Tellico::Data::CollPtr DeliciousImporter::collection() {
  Data::CollPtr coll = XSLTImporter::collection();
  if(!coll) {
    return Data::CollPtr();
  }

  KUrl libraryDir = url();
  libraryDir.setPath(url().directory() + QLatin1String("Images/"));
  const QStringList imageDirs = QStringList()
                             << QLatin1String("Large Covers/")
                             << QLatin1String("Medium Covers/")
                             << QLatin1String("Small Covers/")
                             << QLatin1String("Plain Covers/");
  QString commField;
  switch(coll->type()) {
    case Data::Collection::Book:
      commField = QLatin1String("comments"); break;
    case Data::Collection::Video:
      commField = QLatin1String("plot"); break;
    case Data::Collection::Game:
      commField = QLatin1String("description"); break;
    default:
      myWarning() << "bad collection type:" << coll->type();
  }

  const QString uuidField = QLatin1String("uuid");
  const QString coverField = QLatin1String("cover");
  const bool isLocal = url().isLocalFile();

  Data::EntryList entries = coll->entries();
  foreach(Data::EntryPtr entry, entries) {
    QString comments = entry->field(commField);
    if(!comments.isEmpty()) {
      RTF2HTML rtf2html(comments);
      entry->setField(commField, rtf2html.toHTML());
    }

    //try to add images
    QString uuid = entry->field(uuidField);
    if(!uuid.isEmpty() && isLocal) {
      foreach(const QString& imageDir, imageDirs) {
        QString imgPath = libraryDir.path() + imageDir + uuid;
        if(!QFile::exists(imgPath)) {
          continue;
        }
        QString imgID = ImageFactory::addImage(imgPath, true);
        if(!imgID.isEmpty()) {
          entry->setField(coverField, imgID);
        }
        break;
      }
    }
  }
  coll->removeField(uuidField);
  return coll;
}

#include "deliciousimporter.moc"
