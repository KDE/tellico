/***************************************************************************
                                  pilotdb.h
                             -------------------
    begin                : Thu Nov 20 2003
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

#ifndef PILOTDB_H
#define PILOTDB_H

#include <map>
#include <vector>

#include "libpalm/Database.h"
#include "libflatfile/Field.h"

#include <QByteArray>

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class PilotDB : public PalmLib::Database {
public:
  PilotDB();
  ~PilotDB();

  QByteArray data();

  /**
   * Return the total number of records/resources in this database.
   */
  virtual size_t getNumRecords() const { return m_records.size(); }

  /**
   * Return the database's application info block as a Block
   * object. The caller owns the returned object.
   */
  virtual PalmLib::Block getAppInfoBlock() const { return m_app_info; }

  /**
   * Set the database's app info block to the contents of the
   * passed Block object.
   */
  virtual void setAppInfoBlock(const PalmLib::Block& new_app_info) { m_app_info = new_app_info; }

  /**
   * Return the database's sort info block as a Block
   * object. The caller owns the returned object.
   */
  virtual PalmLib::Block getSortInfoBlock() const { return m_sort_info; }

  /**
   * Set the database's sort info block to the contents of the
   * passed Block object.
   */
  virtual void setSortInfoBlock(const PalmLib::Block& new_sort_info) { m_sort_info = new_sort_info; }

  /**
   * Return the record identified by the given index. The caller
   * owns the returned RawRecord object.
   */
  virtual PalmLib::Record getRecord(unsigned index) const;

  /**
   * Set the record identified by the given index to the given record.
   */
  virtual void setRecord(unsigned index, const PalmLib::Record& rec);

  /**
   * Append the given record to the database.
   */
  virtual void appendRecord(const PalmLib::Record& rec);

  /**
   * Delete all records
   */
  virtual void clearRecords();

  /**
   * returned if the specified (type, ID) combination is not
   * present in the database. The caller owns the returned RawRecord object.
   */
  virtual PalmLib::Resource getResourceByType(PalmLib::pi_uint32_t type, PalmLib::pi_uint32_t id) const;

  /**
   * Return the resource present at the given index. NULL is
   * returned if the index is invalid. The caller owns the
   * returned RawRecord object.
   */
  virtual PalmLib::Resource getResourceByIndex(unsigned index) const;

  /**
   * Set the resource at given index to passed Resource object.
   */
  virtual void setResource(unsigned index, const PalmLib::Resource& rsrc);

  static PalmLib::FlatFile::Field string2field(PalmLib::FlatFile::Field::FieldType type,
                                               const std::string& fldstr);

protected:
  typedef std::vector<PalmLib::Block *> record_list_t;
  typedef std::map<PalmLib::pi_uint32_t, PalmLib::Record *> uid_map_t;

  record_list_t m_records;
  uid_map_t m_uid_map;

private:
  PalmLib::Block m_app_info;
  PalmLib::Block m_sort_info;
  PalmLib::pi_int32_t m_next_record_list_id;
};

  } //end namespace
} // end namespace
#endif
