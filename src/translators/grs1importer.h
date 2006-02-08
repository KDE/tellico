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

#ifndef TELLICO_IMPORT_GRS1IMPORTER_H
#define TELLICO_IMPORT_GRS1IMPORTER_H

#include "textimporter.h"
#include "../datavectors.h"

#include <ksortablevaluelist.h>

#include <qvariant.h>

namespace Tellico {
  namespace Import {

/**
 * @author Robby Stephenson
 */
class GRS1Importer : public TextImporter {
Q_OBJECT

public:
  GRS1Importer(const QString& text);
  virtual ~GRS1Importer() {}

  /**
   * @return A pointer to a @ref Data::Collection, or 0 if none can be created.
   */
  virtual Data::CollPtr collection();
  /**
   */
  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual bool canImport(int type) const;

private:
  static void initTagMap();

  class TagPair : public QPair<int, QVariant> {
  public:
    TagPair() : QPair<int, QVariant>(-1, QVariant()) {}
    TagPair(int n, const QVariant& v) : QPair<int, QVariant>(n, v) {}
    QString toString() const { return QString::number(first) + second.toString(); }
    bool operator< (const TagPair& p) const {
      return toString() < p.toString();
    }
  };

  typedef QMap<TagPair, QString> TagMap;
  static TagMap* s_tagMap;
};

  } // end namespace
} // end namespace
#endif
