/*
 * palm-db-tools: General interface to a PalmOS database.
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 *
 * This header defines an abstract interface to PalmOS
 * databases. Subclasses would include the class that reads/writes PDB
 * files and possibly databases that can be accessed over the HotSync
 * protocols.
 */

#ifndef __PALMLIB_DATABASE_H__
#define __PALMLIB_DATABASE_H__

#include <string>

#include "palmtypes.h"
#include "Block.h"
#include "Record.h"
#include "Resource.h"

namespace PalmLib {

    class Database {
    public:
  // Constants for bits in the flags field of a PalmOS database.
#ifdef __GNUG__
  static const pi_uint16_t FLAG_HDR_RESOURCE            = 0x0001;
  static const pi_uint16_t FLAG_HDR_READ_ONLY           = 0x0002;
  static const pi_uint16_t FLAG_HDR_APPINFO_DIRTY       = 0x0004;
  static const pi_uint16_t FLAG_HDR_BACKUP              = 0x0008;
  static const pi_uint16_t FLAG_HDR_OK_TO_INSTALL_NEWER = 0x0010;
  static const pi_uint16_t FLAG_HDR_RESET_AFTER_INSTALL = 0x0020;
  static const pi_uint16_t FLAG_HDR_COPY_PREVENTION     = 0x0040;
  static const pi_uint16_t FLAG_HDR_STREAM              = 0x0080;
  static const pi_uint16_t FLAG_HDR_HIDDEN              = 0x0100;
  static const pi_uint16_t FLAG_HDR_LAUNCHABLE_DATA     = 0x0200;
  static const pi_uint16_t FLAG_HDR_OPEN                = 0x8000;
#else
  static const pi_uint16_t FLAG_HDR_RESOURCE;
  static const pi_uint16_t FLAG_HDR_READ_ONLY;
  static const pi_uint16_t FLAG_HDR_APPINFO_DIRTY;
  static const pi_uint16_t FLAG_HDR_BACKUP;
  static const pi_uint16_t FLAG_HDR_OK_TO_INSTALL_NEWER;
  static const pi_uint16_t FLAG_HDR_RESET_AFTER_INSTALL;
  static const pi_uint16_t FLAG_HDR_COPY_PREVENTION;
  static const pi_uint16_t FLAG_HDR_STREAM;
  static const pi_uint16_t FLAG_HDR_HIDDEN;
  static const pi_uint16_t FLAG_HDR_LAUNCHABLE_DATA;
  static const pi_uint16_t FLAG_HDR_OPEN;
#endif

  Database(bool resourceDB = false);
  virtual ~Database() { }

  bool isResourceDB() const {return (m_flags & FLAG_HDR_RESOURCE) != 0;}

  virtual pi_uint32_t type() const { return m_type; }
  virtual void type(pi_uint32_t new_type) { m_type = new_type; }

  virtual pi_uint32_t creator() const { return m_creator; }
  virtual void creator(pi_uint32_t new_creator)
      { m_creator = new_creator; }

  virtual pi_uint16_t version() const { return m_version; }
  virtual void version(pi_uint16_t v) { m_version = v; }

  virtual pi_int32_t creation_time() const { return m_time_created; }
  virtual void creation_time(pi_int32_t ct) { m_time_created = ct; }

  virtual pi_uint32_t modification_time() const
      { return m_time_modified; }
  virtual void modification_time(pi_uint32_t mt)
      { m_time_modified = mt; }

  virtual pi_uint32_t backup_time() const { return m_time_backup; }
  virtual void backup_time(pi_uint32_t bt) { m_time_backup = bt; }

  virtual pi_uint32_t modnum() const { return m_modification; }
  virtual void modnum(pi_uint32_t new_modnum)
      { m_modification = new_modnum; }

  virtual pi_uint32_t unique_id_seed() const
      { return m_unique_id_seed; }
  virtual void unique_id_seed(pi_uint32_t uid_seed)
      { m_unique_id_seed = uid_seed; }

  virtual pi_uint16_t flags() const { return m_flags; }
  virtual void flags(pi_uint16_t flags)
      { m_flags = flags & ~(FLAG_HDR_RESOURCE | FLAG_HDR_OPEN); }

  virtual std::string name() const { return m_name; }
  virtual void name(const std::string& new_name) { m_name = new_name; }

  virtual bool backup() const
      { return (m_flags & FLAG_HDR_BACKUP) != 0; }
  virtual void backup(bool state) {
      if (state)
    m_flags |= FLAG_HDR_BACKUP;
      else
    m_flags &= ~(FLAG_HDR_BACKUP);
  }

  virtual bool readonly() const
      { return (m_flags & FLAG_HDR_READ_ONLY) != 0; }
  virtual void readonly(bool state) {
      if (state)
    m_flags |= FLAG_HDR_READ_ONLY;
      else
    m_flags &= ~(FLAG_HDR_READ_ONLY);
  }

  virtual bool copy_prevention() const
      { return (m_flags & FLAG_HDR_COPY_PREVENTION) != 0; }
  virtual void copy_prevention(bool state) {
      if (state)
    m_flags |= FLAG_HDR_COPY_PREVENTION;
      else
    m_flags &= ~(FLAG_HDR_COPY_PREVENTION);
  }

  // Return the total number of records/resources in this
  // database.
  virtual size_t getNumRecords() const = 0;

  // Return the database's application info block as a Block
  // object.
  virtual Block getAppInfoBlock() const { return Block(); }

  // Set the database's app info block to the contents of the
  // passed Block object.
  virtual void setAppInfoBlock(const Block &) { }

  // Return the database's sort info block as a Block object.
  virtual Block getSortInfoBlock() const { return Block(); }

  // Set the database's sort info block to the contents of the
  // passed Block object.
  virtual void setSortInfoBlock(const Block &) { }

  // Return the record identified by the given index. The caller
  // owns the returned RawRecord object.
  virtual Record getRecord(unsigned index) const = 0;

  // Set the record identified by the given index to the given
  // record.
  virtual void setRecord(unsigned index, const Record& rec) = 0;

  // Append the given record to the database.
  virtual void appendRecord(const Record& rec) = 0;

  // returned if the specified (type, ID) combination is not
  // present in the database. The caller owns the returned
  // RawRecord object.
  virtual Resource getResourceByType(pi_uint32_t type,
             pi_uint32_t id) const = 0;

  // Return the resource present at the given index. NULL is
  // returned if the index is invalid. The caller owns the
  // returned RawRecord object.
  virtual Resource getResourceByIndex(unsigned index) const = 0;

  // Set the resouce at given index to passed Resource object.
  virtual void setResource(unsigned index, const Resource& rsrc) = 0;

    private:
        std::string m_name;
        pi_uint16_t m_flags;
        pi_uint16_t m_version;
        pi_uint32_t m_time_created;
        pi_uint32_t m_time_modified;
        pi_uint32_t m_time_backup;
        pi_uint32_t m_modification;
        pi_uint32_t m_type;
        pi_uint32_t m_creator;
        pi_uint32_t m_unique_id_seed;

    };

} // namespace PalmLib

#endif
