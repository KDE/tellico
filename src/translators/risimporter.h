/***************************************************************************
    copyright            : (C) 2004 by Robby Stephenson
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
  namespace Import {

/**
 * @author Robby Stephenson
 * @version $Id: risimporter.h 966 2004-11-20 01:41:11Z robby $
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
  virtual bool canImport(Data::Collection::Type type) { return (type == Data::Collection::Bibtex); }

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
