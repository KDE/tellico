/***************************************************************************
    copyright            : (C) 2004-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef RISIMPORTER_H
#define RISIMPORTER_H

#include "textimporter.h"

#include <qstring.h>

namespace Tellico {
  namespace Data {
    class Field;
  }
  namespace Import {

/**
 * @author Robby Stephenson
 */
class RISImporter : public TextImporter {
Q_OBJECT

public:
  /**
   */
  RISImporter(const KURL& url);

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::Collection* collection();
  /**
   */
  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual bool canImport(int type) const;

  static void initTagMap();
  static void initTypeMap();

private:
  Data::Field* fieldByTag(const QString& tag);
  Data::Collection* m_coll;

  static QMap<QString, QString>* s_tagMap;
  static QMap<QString, QString>* s_typeMap;
};

  } // end namespace
} // end namespace
#endif
