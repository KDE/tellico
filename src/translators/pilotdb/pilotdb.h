/***************************************************************************
                                  pilotdb.h
                             -------------------
    begin                : Thu Nov 20 2003
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

#ifndef PILOTDB_H
#define PILOTDB_H

#include <map>
#include <vector>

#include "libpalm/Database.h"
#include "libflatfile/Field.h"

#include <qcstring.h>

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: pilotdb.h 862 2004-09-15 01:49:51Z robby $
 */
class PilotDB : public PalmLib::Database {
public:
  PilotDB();
  ~PilotDB();

  QByteArray data();

  /**
   * Return the total number of records/resources in this database.
   */
  virtual unsigned getNumRecords() const { return m_records.size(); }

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
