/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
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
#include "tellico_kernel.h"

#include <kiconloader.h>
#include <kimageeffect.h>

#include <qimage.h>

using Tellico::EntryIconFactory;

EntryIconFactory::EntryIconFactory(int size_) : QIconFactory(), m_size(size_) {
  setAutoDelete(true);
}

QPixmap* EntryIconFactory::createPixmap(const QIconSet&, QIconSet::Size, QIconSet::Mode, QIconSet::State) {
  QPixmap entryPix = UserIcon(Kernel::self()->collectionTypeName());
  // if we're 22x22 or smaller, just use entry icon
  if(m_size < 23) {
    QImage entryImg = entryPix.convertToImage();
    entryPix.convertFromImage(entryImg.smoothScale(m_size, m_size, QImage::ScaleMin), 0);
    return new QPixmap(entryPix);
  }

  QPixmap newPix = BarIcon(QString::fromLatin1("mime_empty"), m_size);
  QImage newImg = newPix.convertToImage();
//  QImage blend; Not exactly sure what the coordinates mean, but this seems to work ok.
  KImageEffect::blendOnLower(m_size/4, m_size/4, entryPix.convertToImage(), newImg);
  newPix.convertFromImage(newImg, 0);
  return new QPixmap(newPix);
}
