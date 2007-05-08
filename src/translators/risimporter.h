/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
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
#include "../datavectors.h"

#include <qstring.h>
#include <qmap.h>

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
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual bool canImport(int type) const;

public slots:
  void slotCancel();

private:
  static void initTagMap();
  static void initTypeMap();

  Data::FieldPtr fieldByTag(const QString& tag);

  Data::CollPtr m_coll;
  bool m_cancelled : 1;

  static QMap<QString, QString>* s_tagMap;
  static QMap<QString, QString>* s_typeMap;
};

  } // end namespace
} // end namespace
#endif
