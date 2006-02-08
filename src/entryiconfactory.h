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

#ifndef TELLICO_ENTRYICONFACTORY_H
#define TELLICO_ENTRYICONFACTORY_H

#include <qiconset.h>

namespace Tellico {

/**
 * @author Robby Stephenson
 */
class EntryIconFactory : public QIconFactory {
public:
  EntryIconFactory(int size);

  virtual QPixmap* createPixmap(const QIconSet&, QIconSet::Size, QIconSet::Mode, QIconSet::State);

private:
  int m_size;
};

}

#endif
