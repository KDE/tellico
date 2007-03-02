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

#include "pilotdb.h"
#include "strop.h"
#include "libflatfile/Record.h"

#include <kdebug.h>

#include <qbuffer.h>

using namespace PalmLib;
using Tellico::Export::PilotDB;

namespace {
  static const int PI_HDR_SIZE          = 78;
  static const int PI_RESOURCE_ENT_SIZE = 10;
  static const int PI_RECORD_ENT_SIZE   = 8;
}

PilotDB::PilotDB() : Database(false), m_app_info(), m_sort_info(),
    m_next_record_list_id(0) {
  pi_int32_t now = StrOps::get_current_time();
  creation_time(now);
  modification_time(now);
  backup_time(now);
}

PilotDB::~PilotDB() {
  for(record_list_t::iterator i = m_records.begin(); i != m_records.end(); ++i) {
    delete (*i);
  }
}

QByteArray PilotDB::data() {
  QBuffer b;
  b.open(IO_WriteOnly);

  pi_char_t buf[PI_HDR_SIZE];
  pi_int16_t ent_hdr_size = isResourceDB() ? PI_RESOURCE_ENT_SIZE : PI_RECORD_ENT_SIZE;
  std::streampos offset = PI_HDR_SIZE + m_records.size() * ent_hdr_size + 2;

  for(int i=0; i<32; ++i) {
    buf[i] = 0;
  }
  memcpy(buf, name().c_str(), QMIN(31, name().length()));
  set_short(buf + 32, flags());
  set_short(buf + 34, version());
  set_long(buf + 36, creation_time());
  set_long(buf + 40, modification_time());
  set_long(buf + 44, backup_time());
  set_long(buf + 48, modnum());
  if(m_app_info.raw_size() > 0) {
    set_long(buf + 52, offset);
    offset += m_app_info.raw_size();
  } else {
    set_long(buf + 52, 0);
  }
  if(m_sort_info.raw_size() > 0) {
    set_long(buf + 56, offset);
    offset += m_sort_info.raw_size();
  } else {
    set_long(buf + 56, 0);
  }
  set_long(buf + 60, type());
  set_long(buf + 64, creator());
  set_long(buf + 68, unique_id_seed());
  set_long(buf + 72, m_next_record_list_id);
  set_short(buf + 76, m_records.size());

  // Write the PDB/PRC header to the string.
  b.writeBlock(reinterpret_cast<char *>(buf), sizeof(buf));

  for(record_list_t::iterator i = m_records.begin(); i != m_records.end(); ++i) {
    Block* entry = *i;

    if(isResourceDB()) {
      Resource * resource = reinterpret_cast<Resource *> (entry);
      set_long(buf, resource->type());
      set_short(buf + 4, resource->id());
      set_long(buf + 6, offset);
    } else {
      Record * record = reinterpret_cast<Record *> (entry);
      set_long(buf, offset);
      buf[4] = record->attrs();
      set_treble(buf + 5, record->unique_id());
    }
    b.writeBlock(reinterpret_cast<char *>(buf), ent_hdr_size);
    offset += entry->raw_size();
  }

  b.writeBlock("\0", 1);
  b.writeBlock("\0", 1);

  if(m_app_info.raw_size() > 0) {
    b.writeBlock((char *) m_app_info.raw_data(), m_app_info.raw_size());
  }

  if(m_sort_info.raw_size() > 0) {
    b.writeBlock((char *) m_sort_info.raw_data(), m_sort_info.raw_size());
  }

  for(record_list_t::iterator q = m_records.begin(); q != m_records.end(); ++q) {
    Block* entry = *q;
    b.writeBlock((char *) entry->raw_data(), entry->raw_size());
  }

  b.close();
  return b.buffer();
}

// Return the record identified by the given index. The caller owns
// the returned RawRecord object.
Record PilotDB::getRecord(unsigned index) const
{
    if (index >= m_records.size()) kdDebug() << "invalid index" << endl;
    return *(reinterpret_cast<Record *> (m_records[index]));
}

// Set the record identified by the given index to the given record.
void PilotDB::setRecord(unsigned index, const Record& rec)
{
//    if (index >= m_records.size()) kdDebug() << "invalid index");
    *(reinterpret_cast<Record *> (m_records[index])) = rec;
}

// Append the given record to the database.
void PilotDB::appendRecord(const Record& rec)
{
    Record* record = new Record(rec);

    // If this new record has a unique ID that duplicates any other
    // record, then reset the unique ID to an unused value.
    if (m_uid_map.find(record->unique_id()) != m_uid_map.end()) {
        uid_map_t::iterator iter = max_element(m_uid_map.begin(),
                                               m_uid_map.end());
        pi_uint32_t maxuid = (*iter).first;

        // The new unique ID becomes the max plus one.
        record->unique_id(maxuid + 1);
    }

    m_uid_map[record->unique_id()] = record;
    m_records.push_back(record);
}


void PilotDB::clearRecords()
{
  m_records.erase(m_records.begin(), m_records.end());
}

// Return the resource with the given type and ID. NULL is returned if
// the specified (type, ID) combination is not present in the
// database. The caller owns the returned RawRecord object.
Resource PilotDB::getResourceByType(pi_uint32_t type, pi_uint32_t id) const
{
    for (record_list_t::const_iterator i = m_records.begin();
         i != m_records.end(); ++i) {
        Resource* resource = reinterpret_cast<Resource *> (*i);
        if (resource->type() == type && resource->id() == id)
            return *resource;
    }

  kdWarning() << "PilotDB::getResourceByType() - not found!" << endl;
  return Resource();
}

// Return the resource present at the given index. NULL is returned if
// the index is invalid. The caller owns the returned RawRecord
// object.
Resource PilotDB::getResourceByIndex(unsigned index) const
{
    if (index >= m_records.size()) kdDebug() << "invalid index" << endl;
    return *(reinterpret_cast<Resource *> (m_records[index]));
}

// Set the resouce at given index to passed RawResource object.
void PilotDB::setResource(unsigned index, const Resource& resource)
{
    if (index >= m_records.size()) kdDebug() << "invalid index" << endl;
    *(reinterpret_cast<Resource *> (m_records[index])) = resource;
}

FlatFile::Field PilotDB::string2field(FlatFile::Field::FieldType type, const std::string& fldstr) {
  FlatFile::Field field;

  switch (type) {
    case FlatFile::Field::STRING:
      field.type = FlatFile::Field::STRING;
      field.v_string = fldstr;
      break;

    case FlatFile::Field::BOOLEAN:
      field.type = FlatFile::Field::BOOLEAN;
      field.v_boolean = StrOps::string2boolean(fldstr);
      break;

    case FlatFile::Field::INTEGER:
      field.type = FlatFile::Field::INTEGER;
      StrOps::convert_string(fldstr, field.v_integer);
      break;

    case FlatFile::Field::FLOAT:
      field.type = FlatFile::Field::FLOAT;
      StrOps::convert_string(fldstr, field.v_float);
      break;

    case FlatFile::Field::NOTE:
      field.type = FlatFile::Field::NOTE;
      field.v_string = fldstr.substr(0,NOTETITLE_LENGTH - 1);
      field.v_note = fldstr;
      break;

    case FlatFile::Field::LIST:
      field.type = FlatFile::Field::LIST;
      field.v_string = fldstr;
      break;

    case FlatFile::Field::LINK:
      field.type = FlatFile::Field::LINK;
      field.v_integer = 0;
      field.v_string = fldstr;
      break;

    case FlatFile::Field::LINKED:
      field.type = FlatFile::Field::LINKED;
      field.v_string = fldstr;
      break;

    case FlatFile::Field::CALCULATED:
      field.type = FlatFile::Field::CALCULATED;
      field.v_string = fldstr;
      break;

    case FlatFile::Field::DATE:
      field.type = FlatFile::Field::DATE;
      struct tm time;
      if (!fldstr.empty()) {
#ifdef strptime
        if(!strptime(fldstr.c_str(), "%Y/%m/%d", &time)) {
#else
        if(!StrOps::strptime(fldstr.c_str(), "%Y/%m/%d", &time)) {
#endif
          kdDebug() << "invalid date in field" << endl;
        }
        field.v_date.month = time.tm_mon + 1;
        field.v_date.day = time.tm_mday;
        field.v_date.year = time.tm_year + 1900;
        field.v_time.hour = time.tm_hour;
        field.v_time.minute = time.tm_min;
      } else {
        field.v_date.month = 0;
        field.v_date.day = 0;
        field.v_date.year = 0;
        field.v_time.hour = 24;
        field.v_time.minute = 0;
      }
      break;

    default:
      kdWarning() << "PilotDB::string2field() - unsupported field type" <<  endl;
      break;
  }

  return field;
}
