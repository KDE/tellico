/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "entryiconfactory.h"
#include "document.h"
#include "collection.h"

#include <kiconloader.h>
#include <kimageeffect.h>

#include <qimage.h>

using Tellico::EntryIconFactory;

EntryIconFactory::EntryIconFactory(int size_) : QIconFactory(), m_size(size_) {
  setAutoDelete(true);
}

QPixmap* EntryIconFactory::createPixmap(const QIconSet&, QIconSet::Size, QIconSet::Mode, QIconSet::State) {
  QPixmap newPix = BarIcon(QString::fromLatin1("mime_empty"), m_size);
  QImage newImg = newPix.convertToImage();
  QPixmap entryPix = UserIcon(Data::Document::self()->collection()->entryName());

//  QImage blend; Not exactly sure what the coordinates mean, but this seems to work ok.
  KImageEffect::blendOnLower(m_size/4, m_size/4, entryPix.convertToImage(), newImg);
  newPix.convertFromImage(newImg, 0);
  return new QPixmap(newPix);
}
