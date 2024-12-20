/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
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
  explicit BibtexCollection(bool addDefaultFields, const QString& title = QString());
  /**
   */
  virtual ~BibtexCollection() {}

  virtual Type type() const override { return Bibtex; }
  virtual bool addField(FieldPtr field) override;
  virtual bool modifyField(FieldPtr field) override;
  virtual bool removeField(FieldPtr field, bool force=false) override;
  virtual bool removeField(const QString& name, bool force=false) override;

  FieldPtr fieldByBibtexName(const QString& name) const;
  EntryPtr entryByBibtexKey(const QString& key) const;
  const QString& preamble() const { return m_preamble; }
  void setPreamble(const QString& preamble) { m_preamble = preamble; }
  const StringMap& macroList() const { return m_macros; }
  void setMacroList(const StringMap& map) { m_macros = map; }
  void addMacro(const QString& key, const QString& value) { m_macros.insert(key, value); }
  void removeMacro(const QString& key) { m_macros.remove(key); }

  virtual QString prepareText(const QString& text) const override;
  virtual int sameEntry(Data::EntryPtr entry1, Data::EntryPtr entry2) const override;

  EntryList duplicateBibtexKeys() const;

  static FieldList defaultFields();
  static CollPtr convertBookCollection(CollPtr coll);
  static bool setFieldValue(EntryPtr entry, const QString& bibtexField, const QString& value, CollPtr existingCollection);

private:
  QHash<QString, Data::Field*> m_bibtexFieldDict;
  QString m_preamble;
  StringMap m_macros;
};

  } // end namespace
} // end namespace
#endif
