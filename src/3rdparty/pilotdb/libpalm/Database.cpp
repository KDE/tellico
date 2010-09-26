/*
 * palm-db-tools: General interface to a PalmOS database.
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 *
 * This file implens an abstract interface to PalmOS
 * databases. Subclasses would include the class that reads/writes PDB
 * files and possibly databases that can be accessed over the HotSync
 * protocols.
 */

#include "Database.h"
#include "palmtypes.h"
#include "Record.h"

#ifndef __GNUG__

// MSVC: Visual C++ doesn't like initializers in the header ...
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_RESOURCE = 0x0001;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_READ_ONLY = 0x0002;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_APPINFO_DIRTY = 0x0004;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_BACKUP = 0x0008;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_OK_TO_INSTALL_NEWER = 0x0010;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_RESET_AFTER_INSTALL = 0x0020;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_COPY_PREVENTION = 0x0040;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_STREAM = 0x0080;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_HIDDEN = 0x0100;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_LAUNCHABLE_DATA = 0x0200;
const PalmLib::pi_uint16_t PalmLib::Database::FLAG_HDR_OPEN = 0x8000;
const PalmLib::pi_char_t PalmLib::Record::FLAG_ATTR_DELETED = 0x80;
const PalmLib::pi_char_t PalmLib::Record::FLAG_ATTR_DIRTY   = 0x40;
const PalmLib::pi_char_t PalmLib::Record::FLAG_ATTR_BUSY    = 0x20;
const PalmLib::pi_char_t PalmLib::Record::FLAG_ATTR_SECRET  = 0x10;

#endif

PalmLib::Database::Database(bool resourceDB)
    : m_name(""), m_version(0), m_time_created(0), m_time_modified(0),
      m_time_backup(0), m_modification(0), m_unique_id_seed(0)
{
    m_flags = resourceDB ? FLAG_HDR_RESOURCE : 0;
    m_type = PalmLib::mktag(' ', ' ', ' ', ' ');
    m_creator = PalmLib::mktag(' ', ' ', ' ', ' ');
}
