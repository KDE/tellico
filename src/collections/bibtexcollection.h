/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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

#include <QHash>

namespace Tellico {
  namespace Data {

/**
 * A collection specifically for bibliographies, in Bibtex format.
 *
 * @author Robby Stephenson
 */
class BibtexCollection : public Collection {
Q_OBJECT

public:
  /**
   * The constructor
   *
   * @param title The title of the collection
   */
  BibtexCollection(bool addDefaultFields, const QString& title = QString());
  /**
   */
  virtual ~BibtexCollection() {}

  virtual Type type() const { return Bibtex; }
  virtual bool addField(FieldPtr field);
  virtual bool modifyField(FieldPtr field);
  virtual bool deleteField(FieldPtr field, bool force=false);

  FieldPtr fieldByBibtexName(const QString& name) const;
  const QString& preamble() const { return m_preamble; }
  void setPreamble(const QString& preamble) { m_preamble = preamble; }
  const StringMap& macroList() const { return m_macros; }
  void setMacroList(StringMap map) { m_macros = map; }
  void addMacro(const QString& key, const QString& value) { m_macros.insert(key, value); }

  virtual QString prepareText(const QString& text) const;
  virtual int sameEntry(Data::EntryPtr entry1, Data::EntryPtr entry2) const;

  static FieldList defaultFields();
  static CollPtr convertBookCollection(CollPtr coll);
  static bool setFieldValue(EntryPtr entry, const QString& bibtexField, const QString& value);

private:
  QHash<QString, Data::Field*> m_bibtexFieldDict;
  QString m_preamble;
  StringMap m_macros;
};

  } // end namespace
} // end namespace
#endif
