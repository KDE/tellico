/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
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

#include "../collection.h"

class QFile;

namespace Bookcase {
  namespace Data {

/**
 * A collection specifically for bibliographies, in Bibtex format.
 *
 * It has the following standard attributes:
 * @li Title
 *
 * @author Robby Stephenson
 * @version $Id: bibtexcollection.h 546 2004-03-16 02:12:05Z robby $
 */
class BibtexCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param addFields A boolean indicating whether the default attributes should be added
   * @param title The title of the collection
   */
  BibtexCollection(bool addFields, const QString& title = QString::null);
  /**
   */
  virtual ~BibtexCollection() {}

  virtual CollectionType collectionType() const { return Bibtex; }
  virtual bool addField(Field* field);
  virtual bool modifyField(Field* field);
  virtual bool deleteField(Field* field, bool force=false);

  Field* const fieldByBibtexName(const QString& name) const;
  const QString& preamble() const { return m_preamble; }
  void setPreamble(const QString& preamble) { m_preamble = preamble; }
  const StringMap& macroList() const { return m_macros; }
  void setMacroList(StringMap map) { m_macros = map; }
  void addMacro(const QString& key, const QString& value) { m_macros.insert(key, value); }
  /**
   * Open a pipe to lyx and send a citation for the selected entries
   */
  void citeEntries(QFile& lyxpipe, const EntryList& list);

  static FieldList defaultFields();
  static BibtexCollection* convertBookCollection(const Collection* coll);

private:
  QDict<Field> m_bibtexFieldDict;
  QString m_preamble;
  StringMap m_macros;
};

  } // end namespace
} // end namespace
#endif
