/***************************************************************************
                             bibtexcollection.h
                             -------------------
    begin                : Tue Aug 26 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BIBTEXCOLLECTION_H
#define BIBTEXCOLLECTION_H

#include "../bccollection.h"
#include "bibtexattribute.h"

#include <qmap.h>

typedef QMap<QString, QString> StringMap;

/**
 * A BCCollection specifically for bibliographies, in Bibtex format.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 * @version $Id: bibtexcollection.h 227 2003-10-25 17:28:09Z robby $
 */
class BibtexCollection : public BCCollection {
Q_OBJECT

public: 
  /**
   * The constructor
   *
   * @param addAttributes A boolean indicating whether the default attributes should be added
   * @param title The title of the collection
   */
  BibtexCollection(bool addAttributes, const QString& title = QString::null);
  /**
   */
  virtual ~BibtexCollection() {}

  virtual BCCollection::CollectionType collectionType() const { return BCCollection::Bibtex; };
  virtual bool addAttribute(BCAttribute* att);
  virtual bool modifyAttribute(BCAttribute* att);
  virtual bool deleteAttribute(BCAttribute* att, bool force=false);

  BibtexAttribute* const attributeByBibtexField(const QString& field) const;
  const QString& preamble() const { return m_preamble; }
  void setPreamble(const QString& preamble) { m_preamble = preamble; }
  const StringMap& macroList() const { return m_macros; }
  void setMacroList(StringMap map) { m_macros = map; }
  void addMacro(const QString& key, const QString& value);

  static BCAttributeList defaultAttributes();
  static BibtexCollection* convertBookCollection(const BCCollection* coll);

private:
  QDict<BibtexAttribute> m_bibtexFieldDict;
  QString m_preamble;
  StringMap m_macros;
};

#endif
