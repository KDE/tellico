/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include <qstring.h>

namespace Tellico {

class RTF2HTML {
public:
  RTF2HTML(const QString& text);

  QString toHTML() const;

private:
  QString m_text;
};

}
