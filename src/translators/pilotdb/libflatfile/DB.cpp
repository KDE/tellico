/*
 * palm-db-tools: Read/write DB-format databases
 * Copyright (C) 1999-2001 by Tom Dyas (tdyas@users.sourceforge.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <time.h>

#include <cstring>

#include <kdebug.h>

#include "../strop.h"

#include "DB.h"

#include <kdebug.h>

#define charSeperator '/'
#define VIEWFLAG_USE_IN_EDITVIEW 0x01

#define INVALID_DEFAULT 0
#define NOW_DEFAULT 1
#define CONSTANT_DEFAULT 2

using namespace PalmLib::FlatFile;
using namespace PalmLib;

namespace {
  static const pi_uint16_t CHUNK_FIELD_NAMES         = 0;
  static const pi_uint16_t CHUNK_FIELD_TYPES         = 1;
  static const pi_uint16_t CHUNK_FIELD_DATA          = 2;
  static const pi_uint16_t CHUNK_LISTVIEW_DEFINITION = 64;
  static const pi_uint16_t CHUNK_LISTVIEW_OPTIONS    = 65;
  static const pi_uint16_t CHUNK_LFIND_OPTIONS       = 128;
  static const pi_uint16_t CHUNK_ABOUT               = 254;
}

template <class Map, class Key>
static inline bool has_key(const Map& map, const Key& key)
{
    return map.find(key) != map.end();
}

bool PalmLib::FlatFile::DB::classify(PalmLib::Database& pdb)
{
    return (! pdb.isResourceDB())
        && (pdb.creator() == PalmLib::mktag('D','B','O','S'))
        && (pdb.type()    == PalmLib::mktag('D','B','0','0'));
}

bool PalmLib::FlatFile::DB::match_name(const std::string& name)
{
    return (name == "DB") || (name == "db");
}

void PalmLib::FlatFile::DB::extract_chunks(const PalmLib::Block& appinfo)
{
    size_t i;
    pi_uint16_t chunk_type;
    pi_uint16_t chunk_size;

    if (appinfo.size() > 4) {
        // Loop through each chunk in the block while data remains.
        i = 4;
        while (i < appinfo.size()) {
            /* Stop the loop if there is not enough room for even one
             * chunk header.
             */
//            if (i + 4 >= appinfo.size())
//                throw PalmLib::error("header is corrupt");

            // Copy the chunk type and size into the local buffer.
            chunk_type = get_short(appinfo.data() + i);
            chunk_size = get_short(appinfo.data() + i + 2);
            i += 4;

            // Copy the chunk into seperate storage.
            Chunk chunk(appinfo.data() + i, chunk_size);
            chunk.chunk_type = chunk_type;
            m_chunks[chunk.chunk_type].push_back(chunk);

            /* Advance the index by the size of the chunk. */
            i += chunk.size();
        }

        // If everything was correct, then we should be exactly at the
        // end of the block.
//        if (i != appinfo.size())
//            throw PalmLib::error("header is corrupt");
//    } else {
//        throw PalmLib::error("header is corrupt");
    }
}

void PalmLib::FlatFile::DB::extract_schema(unsigned numFields)
{
    unsigned i;

    if (!has_key(m_chunks, CHUNK_FIELD_NAMES)
        || !has_key(m_chunks, CHUNK_FIELD_TYPES))
        return;
//        throw PalmLib::error("database is missing its schema");

    Chunk names_chunk = m_chunks[CHUNK_FIELD_NAMES][0];
    Chunk types_chunk = m_chunks[CHUNK_FIELD_TYPES][0];
    pi_char_t* p = names_chunk.data();
    pi_char_t* q = types_chunk.data();

    // Ensure that the types chunk has the expected size.
//    if (types_chunk.size() != numFields * sizeof(pi_uint16_t))
//        throw PalmLib::error("types chunk is corrupt");

    // Loop for each field and extract the name and type.
    for (i = 0; i < numFields; ++i) {
        PalmLib::FlatFile::Field::FieldType type;
        int len;

        // Determine the length of the name string. Ensure that the
        // string does not go beyond the end of the chunk.
        pi_char_t* null_p = reinterpret_cast<pi_char_t*>
            (memchr(p, 0, names_chunk.size() - (p - names_chunk.data())));
//        if (!null_p)
//            throw PalmLib::error("names chunk is corrupt");
        len = null_p - p;

        switch (PalmLib::get_short(q)) {
        case 0:
            type = PalmLib::FlatFile::Field::STRING;
            break;

        case 1:
            type = PalmLib::FlatFile::Field::BOOLEAN;
            break;

        case 2:
            type = PalmLib::FlatFile::Field::INTEGER;
            break;

        case 3:
            type = PalmLib::FlatFile::Field::DATE;
            break;

        case 4:
            type = PalmLib::FlatFile::Field::TIME;
            break;

        case 5:
            type = PalmLib::FlatFile::Field::NOTE;
            break;

        case 6:
            type = PalmLib::FlatFile::Field::LIST;
            break;

        case 7:
            type = PalmLib::FlatFile::Field::LINK;
            break;

        case 8:
            type = PalmLib::FlatFile::Field::FLOAT;
            break;

        case 9:
            type = PalmLib::FlatFile::Field::CALCULATED;
            break;

        case 10:
            type = PalmLib::FlatFile::Field::LINKED;
            break;

        default:
//            throw PalmLib::error("unknown field type");
            kdDebug() << "PalmLib::FlatFile::DB::extract_schema() - unknown field type" <<  endl;
            type = PalmLib::FlatFile::Field::STRING;
            break;
        }

        // Inform the superclass about this field.
        appendField(std::string((char *) p, len), type, extract_fieldsdata(i, type));

        // Advance to the information on the next field.
        p += len + 1;
        q += 2;
    }
}

void PalmLib::FlatFile::DB::extract_listviews()
{
    if (!has_key(m_chunks, CHUNK_LISTVIEW_DEFINITION))
        return;

/*        throw PalmLib::error("no list views in database");*/

    const std::vector<Chunk>& chunks = m_chunks[CHUNK_LISTVIEW_DEFINITION];

    for (std::vector<Chunk>::const_iterator iter = chunks.begin();
         iter != chunks.end(); ++iter) {
        const Chunk& chunk = (*iter);
        PalmLib::FlatFile::ListView lv;

//        if (chunk.size() < (2 + 2 + 32))
//            throw PalmLib::error("list view is corrupt");

        pi_uint16_t flags = PalmLib::get_short(chunk.data());
        pi_uint16_t num_cols = PalmLib::get_short(chunk.data() + 2);

        lv.editoruse = false;
        if (flags & VIEWFLAG_USE_IN_EDITVIEW)
          lv.editoruse = true;

//        if (chunk.size() != static_cast<unsigned> (2 + 2 + 32 + num_cols * 4))
//            throw PalmLib::error("list view is corrupt");

        // Determine the length of the name string.
        pi_char_t* null_ptr = reinterpret_cast<pi_char_t*>
            (memchr(chunk.data() + 4, 0, 32));
        if (null_ptr)
            lv.name = std::string((char *) (chunk.data() + 4),
                                  null_ptr - (chunk.data() + 4));
        else
            lv.name = "Unknown";

        const pi_char_t* p = chunk.data() + 2 + 2 + 32;
        for (int i = 0; i < num_cols; ++i) {
            pi_uint16_t field = PalmLib::get_short(p);
            pi_uint16_t width = PalmLib::get_short(p + 2);
            p += 2 * sizeof(pi_uint16_t);

//            if (field >= getNumOfFields())
//                throw PalmLib::error("list view is corrupt");

            PalmLib::FlatFile::ListViewColumn col(field, width);
            lv.push_back(col);
        }

        appendListView(lv);
    }
}

std::string PalmLib::FlatFile::DB::extract_fieldsdata(pi_uint16_t field_search, PalmLib::FlatFile::Field::FieldType type)
{
    std::ostringstream theReturn;

    if (!has_key(m_chunks, CHUNK_FIELD_DATA))
        return std::string(theReturn.str());

    std::vector<Chunk>& chunks = m_chunks[CHUNK_FIELD_DATA];

    pi_uint16_t field_num = 0;
    bool find = false;
    std::vector<Chunk>::const_iterator iter = chunks.begin();
    for ( ; iter != chunks.end(); ++iter) {
        const Chunk& chunk = (*iter);

            field_num = PalmLib::get_short(chunk.data());

        if (field_num == field_search) {
            find = true;
            break;
        }
    }

    if (find) {
        const Chunk& chunk = (*iter);

        switch (type) {

            case PalmLib::FlatFile::Field::STRING:
            theReturn << std::string((const char *)chunk.data()+2, chunk.size() - 2);
        break;

        case PalmLib::FlatFile::Field::BOOLEAN:
        break;

        case PalmLib::FlatFile::Field::INTEGER:
            theReturn << PalmLib::get_long(chunk.data() + sizeof(pi_uint16_t));
            theReturn << charSeperator;
            theReturn << PalmLib::get_short(chunk.data() + sizeof(pi_uint16_t) + sizeof(pi_uint32_t));
        break;

        case PalmLib::FlatFile::Field::FLOAT: {
            pi_double_t value;
            value.words.hi = PalmLib::get_long(chunk.data() + 2);
            value.words.lo = PalmLib::get_long(chunk.data() + 6);

            theReturn << value.number;
        }
        break;

        case PalmLib::FlatFile::Field::DATE:
            if (*(chunk.data() + sizeof(pi_uint16_t)) == NOW_DEFAULT)
                theReturn << "now";
            else if (*(chunk.data() + sizeof(pi_uint16_t)) == CONSTANT_DEFAULT) {
                const pi_char_t * ptr = chunk.data() + sizeof(pi_uint16_t) + 1;
                struct tm date;
                            date.tm_year = PalmLib::get_short(ptr) - 1900;
                            date.tm_mon = (static_cast<int> (*(ptr + 2))) - 1;
                            date.tm_mday = static_cast<int> (*(ptr + 3));

                        (void) mktime(&date);

                        char buf[1024];

                        // Clear out the output buffer.
                        memset(buf, 0, sizeof(buf));

                        // Convert and output the date using the format.
                        strftime(buf, sizeof(buf), "%Y/%m/%d", &date);

                theReturn << buf;
            }
        break;

        case PalmLib::FlatFile::Field::TIME:
            if (*(chunk.data() + sizeof(pi_uint16_t)) == NOW_DEFAULT)
                theReturn << "now";
            else if (*(chunk.data() + sizeof(pi_uint16_t)) == CONSTANT_DEFAULT) {
                const pi_char_t * ptr = chunk.data() + sizeof(pi_uint16_t) + 1;
                struct tm t;
                const struct tm * tm_ptr;
                time_t now;

                time(&now);
                tm_ptr = localtime(&now);
                memcpy(&t, tm_ptr, sizeof(tm));

                t.tm_hour = static_cast<int> (*(ptr));
                t.tm_min = static_cast<int> (*(ptr + 1));
                t.tm_sec = 0;

                char buf[1024];

                // Clear out the output buffer.
                memset(buf, 0, sizeof(buf));

                // Convert and output the date using the format.
                strftime(buf, sizeof(buf), "%H:%M", &t);

                theReturn << buf;
            }
        break;

        case PalmLib::FlatFile::Field::NOTE:
        break;

        case PalmLib::FlatFile::Field::LIST: {
            unsigned short numItems = PalmLib::get_short(chunk.data() + sizeof(pi_uint16_t));
            int prevLength = 0;
            std::string item;

            if (numItems > 0) {
                for (unsigned short i = 0; i < numItems - 1; i++) {
                    item = std::string((const char *)chunk.data() + 3 * sizeof(pi_uint16_t) + prevLength);
                    theReturn << item << charSeperator;
                    prevLength += item.length() + 1;
                }
                item = std::string((const char *)chunk.data() + 3 * sizeof(pi_uint16_t) + prevLength);
                theReturn << item;
            }
        }
        break;

        case PalmLib::FlatFile::Field::LINK:
            theReturn << std::string((const char *)chunk.data()+sizeof(pi_uint16_t));
//            theReturn << std::string((const char *)chunk.data()+sizeof(pi_uint16_t), chunk.size() - 2);
            theReturn << charSeperator;
            theReturn << PalmLib::get_short(chunk.data() + sizeof(pi_uint16_t) + 32 * sizeof(pi_char_t));
        break;

        case PalmLib::FlatFile::Field::LINKED:
            theReturn << PalmLib::get_short(chunk.data() + sizeof(pi_uint16_t));
            theReturn << charSeperator;
            theReturn << PalmLib::get_short(chunk.data() + 2 * sizeof(pi_uint16_t));
        break;

        case PalmLib::FlatFile::Field::CALCULATED:
        break;

        default:
//            throw PalmLib::error("unknown field type");
            break;
        }
    }
    return std::string(theReturn.str());
}

void PalmLib::FlatFile::DB::extract_aboutinfo()
{
    if (!has_key(m_chunks, CHUNK_ABOUT))
        return;

    Chunk chunk = m_chunks[CHUNK_ABOUT][0];
    pi_char_t* header = chunk.data();
    pi_char_t* q = chunk.data() + PalmLib::get_short(header);

    setAboutInformation( (char*)q);
}

void PalmLib::FlatFile::DB::parse_record(PalmLib::Record& record,
                                         std::vector<pi_char_t *>& ptrs,
                                         std::vector<size_t>& sizes)
{
    unsigned i;

    // Ensure that enough space for the offset table exists.
//    if (record.size() < getNumOfFields() * sizeof(pi_uint16_t))
//        throw PalmLib::error("record is corrupt");

    // Extract the offsets from the record. Determine field pointers.
    std::vector<pi_uint16_t> offsets(getNumOfFields());
    for (i = 0; i < getNumOfFields(); ++i) {
        offsets[i] = get_short(record.data() + i * sizeof(pi_uint16_t));
//        if (offsets[i] >= record.size())
//            throw PalmLib::error("record is corrupt");
        ptrs.push_back(record.data() + offsets[i]);
    }

    // Determine the field sizes.
    for (i = 0; i < getNumOfFields() - 1; ++i) {
        sizes.push_back(offsets[i + 1] - offsets[i]);
    }
    sizes.push_back(record.size() - offsets[getNumOfFields() - 1]);
}

PalmLib::FlatFile::DB::DB(PalmLib::Database& pdb)
    : Database("db", pdb), m_flags(0)
{
    // Split the application information block into its component chunks.
    extract_chunks(pdb.getAppInfoBlock());

    // Pull the header fields and schema out of the databasse.
    m_flags = get_short(pdb.getAppInfoBlock().data());
    unsigned numFields = get_short(pdb.getAppInfoBlock().data() + 2);
    extract_schema(numFields);

    // Extract all of the list views.
    extract_listviews();

    extract_aboutinfo();

    for (unsigned i = 0; i < pdb.getNumRecords(); ++i) {
        PalmLib::Record record = pdb.getRecord(i);
        Record rec;

            std::vector<pi_char_t *> ptrs;
            std::vector<size_t> sizes;
            parse_record(record, ptrs, sizes);
            for (unsigned j = 0; j < getNumOfFields(); ++j) {
                PalmLib::FlatFile::Field f;
        f.type = field_type(j);

                switch (field_type(j)) {
                case PalmLib::FlatFile::Field::STRING:
                            f.type = PalmLib::FlatFile::Field::STRING;
                            f.v_string = std::string((char *) ptrs[j], sizes[j] - 1);
                    break;

                case PalmLib::FlatFile::Field::BOOLEAN:
                            f.type = PalmLib::FlatFile::Field::BOOLEAN;
                            if (*(ptrs[j]))
                                f.v_boolean = true;
                            else
                                f.v_boolean = false;
                    break;

                case PalmLib::FlatFile::Field::INTEGER:
                            f.type = PalmLib::FlatFile::Field::INTEGER;
                            f.v_integer = PalmLib::get_long(ptrs[j]);
                break;

                case PalmLib::FlatFile::Field::FLOAT: {
                            // Place data from database in a union for conversion.
                            pi_double_t value;
                            value.words.hi = PalmLib::get_long(ptrs[j]);
                            value.words.lo = PalmLib::get_long(ptrs[j] + 4);

                            // Fill out the information for this field.
                            f.type = PalmLib::FlatFile::Field::FLOAT;
                            f.v_float = value.number;
                    }
                break;

                    case PalmLib::FlatFile::Field::DATE:
                            f.type = PalmLib::FlatFile::Field::DATE;
                            f.v_date.year = PalmLib::get_short(ptrs[j]);
                            f.v_date.month = static_cast<int> (*(ptrs[j] + 2));
                            f.v_date.day = static_cast<int> (*(ptrs[j] + 3));
                break;

                    case PalmLib::FlatFile::Field::TIME:
                            f.type = PalmLib::FlatFile::Field::TIME;
                            f.v_time.hour = static_cast<int> (*(ptrs[j]));
                            f.v_time.minute = static_cast<int> (*(ptrs[j] + 1));
                break;

                case PalmLib::FlatFile::Field::NOTE:
                            f.type = PalmLib::FlatFile::Field::NOTE;
                            f.v_string = std::string((char *) ptrs[j], sizes[j] - 3);
                            f.v_note = std::string((char *) (record.data() + get_short(ptrs[j] + strlen(f.v_string.c_str()) + 1)));
                break;

                case PalmLib::FlatFile::Field::LIST:
                            f.type = PalmLib::FlatFile::Field::LIST;
                if (!field(j).argument().empty()) {
                    std::string data = field(j).argument();
                    unsigned int k, pos = 0;
                    pi_uint16_t itemID = *ptrs[j]; // TR: a list value is stored on 1 byte

                    for (k = 0; k < itemID; k++) {
                        if ((pos = data.find(charSeperator, pos)) == std::string::npos) {
                            break;
                        }
                        pos++;
                    }
                    if (pos == std::string::npos) {
                        f.v_string = "N/A";
                    } else {
                        if (data.find(charSeperator, pos) == std::string::npos) {
                            f.v_string = data.substr( pos, std::string::npos);
                        } else {
                            f.v_string = data.substr( pos, data.find(charSeperator, pos) - pos);
            }
                    }
                }
                break;

                case PalmLib::FlatFile::Field::LINK:
                            f.type = PalmLib::FlatFile::Field::LINK;
                            f.v_integer = PalmLib::get_long(ptrs[j]);
                            f.v_string = std::string((char *) (ptrs[j] + 4), sizes[j] - 5);
                break;

                case PalmLib::FlatFile::Field::LINKED:
                            f.type = PalmLib::FlatFile::Field::LINKED;
                            f.v_string = std::string((char *) ptrs[j], sizes[j] - 1);
                break;

                case PalmLib::FlatFile::Field::CALCULATED: {
                std::ostringstream value;
                            f.type = PalmLib::FlatFile::Field::CALCULATED;
                switch (ptrs[j][0]) {
                case 1: //string
                    value << std::string((char *) ptrs[j] + 1, sizes[j] - 2);
                break;
                case 2: //integer
                    value << PalmLib::get_long(ptrs[j] + 1);
                break;
                case 9: //float
                {
                    pi_double_t fvalue;
                    fvalue.words.hi = PalmLib::get_long(ptrs[j] + 1);
                    fvalue.words.lo = PalmLib::get_long(ptrs[j] + 5);

                    value << fvalue.number;
                    }
                default:
                    value << "N/A";
                }
                f.v_string = value.str();
                } break;

                    default:
//                            throw PalmLib::error("unknown field type");
                            break;
            }

            // Append this field to the record.
            rec.appendField(f);
        }
        rec.unique_id(record.unique_id());
        // Append this record to the database.
        appendRecord(rec);
    }
}

void PalmLib::FlatFile::DB::make_record(PalmLib::Record& pdb_record,
                                        const Record& record) const
{
  unsigned int i;

    // Determine the packed size of this record.
    size_t size = getNumOfFields() * sizeof(pi_uint16_t);
    for (i = 0; i < getNumOfFields(); i++) {
#ifdef HAVE_VECTOR_AT
  const Field field = record.fields().at(i);
#else
  const Field field = record.fields()[i];
#endif
        switch (field.type) {
        case PalmLib::FlatFile::Field::STRING:
            size += field.v_string.length() + 1;
            break;

        case PalmLib::FlatFile::Field::NOTE:
            size += field.v_string.length() + 3;
        size += field.v_note.length() + 1;
            break;

        case PalmLib::FlatFile::Field::BOOLEAN:
            size += 1;
            break;

        case PalmLib::FlatFile::Field::INTEGER:
            size += 4;
            break;

        case PalmLib::FlatFile::Field::FLOAT:
            size += 8;
            break;

        case PalmLib::FlatFile::Field::DATE:
            size += sizeof(pi_uint16_t) + 2 * sizeof(pi_char_t);
            break;

        case PalmLib::FlatFile::Field::TIME:
            size += 2 * sizeof(pi_char_t);
            break;

        case PalmLib::FlatFile::Field::LIST:
            size += sizeof(pi_char_t);
            break;

        case PalmLib::FlatFile::Field::LINK:
            size += sizeof(pi_int32_t);
            size += field.v_string.length() + 1;
            break;

        case PalmLib::FlatFile::Field::LINKED:
            size += field.v_string.length() + 1;
            break;

    case PalmLib::FlatFile::Field::CALCULATED:
        size += 1;
        break;

        default:
//            throw PalmLib::error("unsupported field type");
            break;
        }
    }

    // Allocate a block for the packed record and setup the pointers.
    pi_char_t* buf = new pi_char_t[size];
    pi_char_t* p = buf + getNumOfFields() * sizeof(pi_uint16_t);
    pi_char_t* offsets = buf;

    // Pack the fields into the buffer.
    for (i = 0; i < getNumOfFields(); i++) {
    pi_char_t* noteOffsetOffset = 0;
    bool setNote = false;
#ifdef HAVE_VECTOR_AT
    const Field fieldData = record.fields().at(i);
#else
    const Field fieldData = record.fields()[i];
#endif

        // Mark the offset to the start of this field in the offests table.
        PalmLib::set_short(offsets, static_cast<pi_uint16_t> (p - buf));
        offsets += sizeof(pi_uint16_t);

        // Pack the field.
        switch (fieldData.type) {
        case PalmLib::FlatFile::Field::STRING:
            memcpy(p, fieldData.v_string.c_str(), fieldData.v_string.length() + 1);
            p += fieldData.v_string.length() + 1;
            break;

        case PalmLib::FlatFile::Field::NOTE:
//          if (setNote)
//            throw PalmLib::error("unsupported field type");
            memcpy(p, fieldData.v_string.c_str(), fieldData.v_string.length() + 1);
            p += fieldData.v_string.length() + 1;
            noteOffsetOffset = p;
            p += 2;
            setNote = true;
            break;

        case PalmLib::FlatFile::Field::BOOLEAN:
            *p++ = ((fieldData.v_boolean) ? 1 : 0);
            break;

        case PalmLib::FlatFile::Field::INTEGER:
            PalmLib::set_long(p, fieldData.v_integer);
            p += sizeof(pi_int32_t);
            break;

        case PalmLib::FlatFile::Field::FLOAT: {
            // Place data the data in a union for easy conversion.
            pi_double_t value;
            value.number = fieldData.v_float;
            PalmLib::set_long(p, value.words.hi);
            p += sizeof(pi_uint32_t);
            PalmLib::set_long(p, value.words.lo);
            p += sizeof(pi_uint32_t);
            break;
        }

        case PalmLib::FlatFile::Field::DATE:
            PalmLib::set_short(p, fieldData.v_date.year);
            p += sizeof(pi_uint16_t);
            *p++ = static_cast<pi_char_t> (fieldData.v_date.month & 0xFF);
            *p++ = static_cast<pi_char_t> (fieldData.v_date.day & 0xFF);
            break;

        case PalmLib::FlatFile::Field::TIME:
            *p++ = static_cast<pi_char_t> (fieldData.v_time.hour & 0xFF);
            *p++ = static_cast<pi_char_t> (fieldData.v_time.minute & 0xFF);
            break;

        case PalmLib::FlatFile::Field::LIST:
        if (!field(i).argument().empty()) {
            std::string data = field(i).argument();
            unsigned int pos = 0, next, j = 0;
            pi_int16_t itemID = -1;

            while ( (next = data.find(charSeperator, pos)) != std::string::npos) {
                if (fieldData.v_string == data.substr( pos, next - pos)) {
                    itemID = j;
                    break;
                }
                j++;
                pos = next + 1;
            }
      // TR: the following test handles the case where the field value
      // equals the last item in list (bugfix)
            if (itemID == -1 && fieldData.v_string == data.substr( pos, std::string::npos)) {
                itemID = j;
            }
            p[0] = itemID; // TR: a list value is stored on 1 byte
            p += sizeof(pi_char_t);
        }
            break;

        case PalmLib::FlatFile::Field::LINK:
            PalmLib::set_long(p, fieldData.v_integer);
            p += sizeof(pi_int32_t);
            memcpy(p, fieldData.v_string.c_str(), fieldData.v_string.length() + 1);
            p += fieldData.v_string.length() + 1;
            break;

        case PalmLib::FlatFile::Field::LINKED:
            memcpy(p, fieldData.v_string.c_str(), fieldData.v_string.length() + 1);
            p += fieldData.v_string.length() + 1;
            break;

    case PalmLib::FlatFile::Field::CALCULATED:
        *p = 13;
        p++;
        break;

        default:
//            throw PalmLib::error("unsupported field type");
            break;
        }
    if (setNote) {
        if (fieldData.v_note.length()) {
                memcpy(p, fieldData.v_note.c_str(), fieldData.v_note.length() + 1);
                PalmLib::set_short(noteOffsetOffset, (pi_uint16_t)(p - buf));
                p += fieldData.v_note.length() + 1;
        } else {
                PalmLib::set_short(noteOffsetOffset, 0);
        }
    }
    }

    // Place the packed data into the PalmOS record.
    pdb_record.set_raw(buf, size);
    delete [] buf;
}

void PalmLib::FlatFile::DB::build_fieldsdata_chunks(std::vector<DB::Chunk>& chunks) const
{
    pi_char_t * buf = 0, * p;
    unsigned int size, i;

    for (i = 0; i < getNumOfFields(); ++i) {
        size = 0;
        switch (field_type(i)) {
        case PalmLib::FlatFile::Field::STRING:
            if (!field(i).argument().empty()) {
                size = (field(i).argument().length() + 1) + 2;
                buf = new pi_char_t[size];
                PalmLib::set_short(buf, i);
                strcpy((char *) (buf + 2), field(i).argument().c_str());
            }
        break;

        case PalmLib::FlatFile::Field::BOOLEAN:
        break;

        case PalmLib::FlatFile::Field::INTEGER:
            if (!field(i).argument().empty()) {
                std::string data = field(i).argument();
                std::pair< PalmLib::pi_int32_t, PalmLib::pi_int16_t> values(0, 0);

                if ( data.find(charSeperator) != std::string::npos) {
                    StrOps::convert_string(data.substr( 0, data.find(charSeperator)), values.first);
                    StrOps::convert_string(data.substr( data.find(charSeperator) + 1, std::string::npos), values.second);
                } else
                    StrOps::convert_string(data, values.first);

                size = 2 + sizeof(pi_uint32_t) + sizeof(pi_uint16_t);
                buf = new pi_char_t[size];
                p = buf;
                PalmLib::set_short(p, i);
                p += sizeof(pi_uint16_t);
                    PalmLib::set_long(p, values.first);
                    p += sizeof(pi_uint32_t);
                    PalmLib::set_short(p, values.second);
                    p += sizeof(pi_uint16_t);
            }
        break;

        case PalmLib::FlatFile::Field::FLOAT:
            if (!field(i).argument().empty()) {
                std::string data = field(i).argument();
                pi_double_t value;

                StrOps::convert_string(data, value.number);

                size = 2 + 2 * sizeof(pi_uint32_t);
                buf = new pi_char_t[size];
                p = buf;
                PalmLib::set_short(p, i);
                p += sizeof(pi_uint16_t);
                PalmLib::set_long(p, value.words.hi);
                p += sizeof(pi_uint32_t);
                PalmLib::set_long(p, value.words.lo);
                p += sizeof(pi_uint32_t);
            }
        break;

        case PalmLib::FlatFile::Field::DATE:
            if (!field(i).argument().empty()) {
                std::string data = field(i).argument();
                struct tm date;
                pi_char_t type;

                if (data.substr(0, 3) == "now") {
                    type = NOW_DEFAULT;
                            const struct tm * tm_ptr;
                            time_t now;

                            time(&now);
                            tm_ptr = localtime(&now);
                            memcpy(&date, tm_ptr, sizeof(tm));
                } else
#ifdef strptime
                    if (strptime(data.c_str(), "%Y/%m/%d", &date))
#else
                    if (StrOps::strptime(data.c_str(), "%Y/%m/%d", &date))
#endif
                    type = CONSTANT_DEFAULT;
                else
                    type = INVALID_DEFAULT;

                if (type != INVALID_DEFAULT) {
                    size = sizeof(pi_uint16_t) + 1 + sizeof(pi_uint16_t) + 2;
                    buf = new pi_char_t[size];
                    p = buf;
                    PalmLib::set_short(p, i);
                    p += sizeof(pi_uint16_t);
                    *p++ = static_cast<pi_char_t> (type & 0xFF);
                    PalmLib::set_short(p, date.tm_year + 1900);
                    p += sizeof(pi_uint16_t);
                    *p++ = static_cast<pi_char_t> ((date.tm_mon + 1) & 0xFF);
                    *p++ = static_cast<pi_char_t> (date.tm_mday & 0xFF);
                }

            }
        break;

        case PalmLib::FlatFile::Field::TIME:
            if (!field(i).argument().empty()) {
                std::string data = field(i).argument();
                struct tm t;
                pi_char_t type;

                if (data == "now") {
                    type = NOW_DEFAULT;
                    const struct tm * tm_ptr;
                    time_t now;

                    time(&now);
                    tm_ptr = localtime(&now);
                    memcpy(&t, tm_ptr, sizeof(tm));
                } else
#ifdef strptime
                if (!strptime(data.c_str(), "%H/%M", &t))
#else
                if (!StrOps::strptime(data.c_str(), "%H/%M", &t))
#endif
                    type = CONSTANT_DEFAULT;
                else
                    type = INVALID_DEFAULT;

                if (type != INVALID_DEFAULT) {
                    size = sizeof(pi_uint16_t) + 1 + sizeof(pi_uint16_t) + 2;
                    buf = new pi_char_t[size];
                    p = buf;
                    PalmLib::set_short(p, i);
                    p += sizeof(pi_uint16_t);
                    *p++ = static_cast<pi_char_t> (type & 0xFF);
                    *p++ = static_cast<pi_char_t> (t.tm_hour & 0xFF);
                    *p++ = static_cast<pi_char_t> (t.tm_min & 0xFF);
                }

            }
        break;

        case PalmLib::FlatFile::Field::NOTE:
        break;

        case PalmLib::FlatFile::Field::LIST:
            if (!field(i).argument().empty()) {
                std::string data = field(i).argument();
                std::vector<std::string> items;
                unsigned int pos = 0, next;
                std::vector<std::string>::iterator iter;
                size = 2 + 2 * sizeof(pi_uint16_t);
                while ( (next = data.find(charSeperator, pos)) != std::string::npos) {
                    std::string item = data.substr( pos, next - pos);
                    items.push_back(item);
                    size += item.length() + 1;
                    pos = next + 1;
                }
                if (pos != std::string::npos) {
                    std::string item = data.substr( pos, std::string::npos);
                    items.push_back(item);
                    size += item.length() + 1;
                }

                buf = new pi_char_t[size];
                p = buf;
                PalmLib::set_short(p, i);
                p += sizeof(pi_uint16_t);
                PalmLib::set_short(p, items.size());
                p += sizeof(pi_uint16_t);
                p += sizeof(pi_uint16_t);
                for (iter = items.begin(); iter != items.end(); ++iter) {
                    std::string& item = (*iter);
                    strcpy((char *) p, item.c_str());
                    p[item.length()] = 0;
                    p += item.length() + 1;
                }

            }
        break;

        case PalmLib::FlatFile::Field::LINK:
            if (!field(i).argument().empty()) {
                std::string data = field(i).argument();
                std::string databasename;
                pi_uint16_t fieldnum;

                if ( data.find(charSeperator) != std::string::npos) {
                    databasename = data.substr( 0, data.find(charSeperator));
                    StrOps::convert_string(data.substr( data.find(charSeperator) + 1, std::string::npos), fieldnum);
                } else {
                    databasename = data;
                    fieldnum = 0;
                }

                size = 2 + 32 * sizeof(pi_char_t) + sizeof(pi_uint16_t);
                buf = new pi_char_t[size];
                p = buf;
                PalmLib::set_short(p, i);
                p += sizeof(pi_uint16_t);
                strcpy((char *) p, databasename.c_str());
                p += 32 * sizeof(pi_char_t);
                PalmLib::set_short(p, fieldnum);
                p += sizeof(pi_uint16_t);
            }
        break;

        case PalmLib::FlatFile::Field::LINKED:
            if (!field(i).argument().empty()) {
                std::string data = field(i).argument();
                pi_uint16_t linknum;
                pi_uint16_t fieldnum;

                if ( data.find(charSeperator) != std::string::npos) {
                    StrOps::convert_string(data.substr( 0, data.find(charSeperator)), linknum);
                    StrOps::convert_string(data.substr( data.find(charSeperator) + 1, std::string::npos), fieldnum);
                    if (field_type(linknum) != PalmLib::FlatFile::Field::LINK) {
                        unsigned int j = 0;
                        while (field_type(j) != PalmLib::FlatFile::Field::LINK && j < i) j++;
                        linknum = j;
                    }
                } else {
                    unsigned int j = 0;
                    while (field_type(j) != PalmLib::FlatFile::Field::LINK && j < i) j++;
                    linknum = j;
                    fieldnum = 0;
                }

                size = 2 + 2 *  sizeof(pi_uint16_t);
                buf = new pi_char_t[size];
                p = buf;
                PalmLib::set_short(p, i);
                p += sizeof(pi_uint16_t);
                       PalmLib::set_short(p, linknum);
                    p += sizeof(pi_uint16_t);
                       PalmLib::set_short(p, fieldnum);
                    p += sizeof(pi_uint16_t);
            }
        break;

        case PalmLib::FlatFile::Field::CALCULATED:
        break;

        default:
//            throw PalmLib::error("unknown field type");
            break;
        }

        if (size) {
            Chunk data_chunk(buf, size);
            data_chunk.chunk_type = CHUNK_FIELD_DATA;
            delete [] buf;
            chunks.push_back(data_chunk);
        }
    }
}

void PalmLib::FlatFile::DB::build_about_chunk(std::vector<DB::Chunk>& chunks) const
{
    pi_char_t* buf;
    pi_char_t* p;
    int headersize = 2*sizeof(pi_uint16_t);
    std::string information = getAboutInformation();

    if (!information.length())
        return;
    // Build the names chunk.
    buf = new pi_char_t[headersize + information.length() + 1];
    p = buf;

    PalmLib::set_short(p, headersize);
    p += 2;
    PalmLib::set_short(p, 1); //about type version
    p += 2;
    memcpy(p, information.c_str(), information.length() + 1);
    p += information.length() + 1;
    Chunk chunk(buf, headersize + information.length() + 1);
    chunk.chunk_type = CHUNK_ABOUT;
    delete [] buf;
    chunks.push_back(chunk);

}

void PalmLib::FlatFile::DB::build_standard_chunks(std::vector<DB::Chunk>& chunks) const
{
    pi_char_t* buf;
    pi_char_t* p;
    unsigned i;

    // Determine the size needed for the names chunk.
    size_t names_chunk_size = 0;
    for (i = 0; i < getNumOfFields(); ++i) {
        names_chunk_size += field_name(i).length() + 1;
    }

    // Build the names chunk.
    buf = new pi_char_t[names_chunk_size];
    p = buf;
    for (i = 0; i < getNumOfFields(); ++i) {
        const std::string name = field_name(i);
        memcpy(p, name.c_str(), name.length() + 1);
        p += name.length() + 1;
    }
    Chunk names_chunk(buf, names_chunk_size);
    names_chunk.chunk_type = CHUNK_FIELD_NAMES;
    delete [] buf;

    // Build the types chunk.
    buf = new pi_char_t[getNumOfFields() * sizeof(pi_uint16_t)];
    p = buf;
    for (i = 0; i < getNumOfFields(); ++i) {
        // Pack the type of the current field.
        switch (field_type(i)) {
        case PalmLib::FlatFile::Field::STRING:
            PalmLib::set_short(p, 0);
            break;

        case PalmLib::FlatFile::Field::BOOLEAN:
            PalmLib::set_short(p, 1);
            break;

        case PalmLib::FlatFile::Field::INTEGER:
            PalmLib::set_short(p, 2);
            break;

        case PalmLib::FlatFile::Field::DATE:
            PalmLib::set_short(p, 3);
            break;

        case PalmLib::FlatFile::Field::TIME:
            PalmLib::set_short(p, 4);
            break;

        case PalmLib::FlatFile::Field::NOTE:
            PalmLib::set_short(p, 5);
            break;

        case PalmLib::FlatFile::Field::LIST:
            PalmLib::set_short(p, 6);
            break;

        case PalmLib::FlatFile::Field::LINK:
            PalmLib::set_short(p, 7);
            break;

        case PalmLib::FlatFile::Field::FLOAT:
            PalmLib::set_short(p, 8);
            break;

        case PalmLib::FlatFile::Field::CALCULATED:
            PalmLib::set_short(p, 9);
            break;

        case PalmLib::FlatFile::Field::LINKED:
            PalmLib::set_short(p, 10);
            break;

        default:
//            throw PalmLib::error("unsupported field type");
            break;
        }

        // Advance to the next position.
        p += sizeof(pi_uint16_t);
    }
    Chunk types_chunk(buf, getNumOfFields() * sizeof(pi_uint16_t));
    types_chunk.chunk_type = CHUNK_FIELD_TYPES;
    delete [] buf;

    // Build the list view options chunk.
    buf = new pi_char_t[2 * sizeof(pi_uint16_t)];
    PalmLib::set_short(buf, 0);
    PalmLib::set_short(buf + sizeof(pi_uint16_t), 0);
    Chunk listview_options_chunk(buf, 2 * sizeof(pi_uint16_t));
    listview_options_chunk.chunk_type = CHUNK_LISTVIEW_OPTIONS;
    delete [] buf;

    // Build the local find options chunk.
    buf = new pi_char_t[sizeof(pi_uint16_t)];
    PalmLib::set_short(buf, 0);
    Chunk lfind_options_chunk(buf, 1 * sizeof(pi_uint16_t));
    lfind_options_chunk.chunk_type = CHUNK_LFIND_OPTIONS;
    delete [] buf;

    // Add all the chunks to the chunk list.
    chunks.push_back(names_chunk);
    chunks.push_back(types_chunk);
    chunks.push_back(listview_options_chunk);
    chunks.push_back(lfind_options_chunk);
}

void PalmLib::FlatFile::DB::build_listview_chunk(std::vector<DB::Chunk>& chunks,
                                                 const ListView& lv) const
{
    // Calculate size and allocate space for the temporary buffer.
    size_t size = 2 * sizeof(pi_uint16_t) + 32
        + lv.size() * (2 * sizeof(pi_uint16_t));
    pi_char_t* buf = new pi_char_t[size];

    // Fill in the header details.
  pi_uint16_t flags = 0;
  if (lv.editoruse) {
    std::cout << "editoruse\n";
    flags |= VIEWFLAG_USE_IN_EDITVIEW;
  }
    PalmLib::set_short(buf, flags);
    PalmLib::set_short(buf + sizeof(pi_uint16_t), lv.size());
    memset((char *) (buf + 4), 0, 32);
    strncpy((char *) (buf + 4), lv.name.c_str(), 32);

    // Fill in the column details.
    pi_char_t* p = buf + 4 + 32;
    for (ListView::const_iterator i = lv.begin(); i != lv.end(); ++i) {
        const ListViewColumn& col = (*i);
        PalmLib::set_short(p, col.field);
        PalmLib::set_short(p + sizeof(pi_uint16_t), col.width);
        p += 2 * sizeof(pi_uint16_t);
    }

    // Create the chunk and place it in the chunks list.
    Chunk chunk(buf, size);
    chunk.chunk_type = CHUNK_LISTVIEW_DEFINITION;
    delete [] buf;
    chunks.push_back(chunk);
}

void PalmLib::FlatFile::DB::build_appinfo_block(const std::vector<DB::Chunk>& chunks, PalmLib::Block& appinfo) const
{
    std::vector<Chunk>::const_iterator iter;

    // Determine the size of the final app info block.
    size_t size = 2 * sizeof(pi_uint16_t);
    for (iter = chunks.begin(); iter != chunks.end(); ++iter) {
        const Chunk& chunk = (*iter);
        size += 2 * sizeof(pi_uint16_t) + chunk.size();
    }

    // Allocate the temporary buffer. Fill in the header.
    pi_char_t* buf = new pi_char_t[size];
    PalmLib::set_short(buf, m_flags);
    PalmLib::set_short(buf + sizeof(pi_uint16_t), getNumOfFields());

    // Pack the chunks into the buffer.
    size_t i = 4;
    for (iter = chunks.begin(); iter != chunks.end(); ++iter) {
        const Chunk& chunk = (*iter);
        // Set the chunk type and size.
        PalmLib::set_short(buf + i, chunk.chunk_type);
        PalmLib::set_short(buf + i + 2, chunk.size());
        i += 4;

        // Copy the chunk data into the buffer.
        memcpy(buf + i, chunk.data(), chunk.size());
        i += chunk.size();
    }

    // Finally, move the buffer into the provided appinfo block.
    appinfo.set_raw(buf, size);
    delete [] buf;
}

void PalmLib::FlatFile::DB::outputPDB(PalmLib::Database& pdb) const
{
    unsigned i;

    // Let the superclass have a chance.
    SUPERCLASS(PalmLib::FlatFile, Database, outputPDB, (pdb));

    // Set the database's type and creator.
    pdb.type(PalmLib::mktag('D','B','0','0'));
    pdb.creator(PalmLib::mktag('D','B','O','S'));

    // Create the app info block.
    std::vector<Chunk> chunks;
    build_standard_chunks(chunks);
    for (i = 0; i < getNumOfListViews(); ++i) {
        build_listview_chunk(chunks, getListView(i));
    }
    build_fieldsdata_chunks(chunks);
    build_about_chunk(chunks);

    PalmLib::Block appinfo;
    build_appinfo_block(chunks, appinfo);
    pdb.setAppInfoBlock(appinfo);

    // Output each record to the PalmOS database.
    for (i = 0; i < getNumRecords(); ++i) {
        Record record = getRecord(i);
        PalmLib::Record pdb_record;

        make_record(pdb_record, record);
        pdb.appendRecord(pdb_record);
    }
}

unsigned PalmLib::FlatFile::DB::getMaxNumOfFields() const
{
    return 0;
}

bool
PalmLib::FlatFile::DB::supportsFieldType(const Field::FieldType& type) const
{
    switch (type) {
        case PalmLib::FlatFile::Field::STRING:
        case PalmLib::FlatFile::Field::BOOLEAN:
        case PalmLib::FlatFile::Field::INTEGER:
        case PalmLib::FlatFile::Field::FLOAT:
        case PalmLib::FlatFile::Field::DATE:
        case PalmLib::FlatFile::Field::TIME:
        case PalmLib::FlatFile::Field::NOTE:
        case PalmLib::FlatFile::Field::LIST:
        case PalmLib::FlatFile::Field::LINK:
        case PalmLib::FlatFile::Field::LINKED:
        case PalmLib::FlatFile::Field::CALCULATED:
        return true;
    default:
        return false;
    }
}

std::vector<std::string>
PalmLib::FlatFile::DB::field_argumentf(int i, std::string& format)
{
    std::vector<std::string> vtitles(0, std::string(""));
        int j;

    switch (field_type(i)) {
    case PalmLib::FlatFile::Field::STRING:
        format = std::string("%s");
        vtitles.push_back(std::string("default value"));
    break;
    case PalmLib::FlatFile::Field::INTEGER:
        format = std::string("%ld/%d");
        vtitles.push_back(std::string("default value"));
        vtitles.push_back(std::string("increment"));
    break;
    case PalmLib::FlatFile::Field::FLOAT:
        format = std::string("%f");
        vtitles.push_back(std::string("default value"));
    break;
    case PalmLib::FlatFile::Field::DATE:
        format = std::string("%d/%d/%d");
        vtitles.push_back(std::string("Year (or now)"));
        vtitles.push_back(std::string("Month"));
        vtitles.push_back(std::string("Day in the month"));
    break;
    case PalmLib::FlatFile::Field::TIME:
        format = std::string("%d/%d");
        vtitles.push_back(std::string("Hour (or now)"));
        vtitles.push_back(std::string("Minute"));
    break;
        case PalmLib::FlatFile::Field::LIST:
        format = std::string("");
        for (j = 0; j < 31; i++) {
            format += std::string("%s/");
            std::ostringstream  title;
            title << "item " << j;
            vtitles.push_back(title.str());
        }
        format += std::string("%s");
        vtitles.push_back(std::string("item 32"));
    break;
        case PalmLib::FlatFile::Field::LINK:
        format = std::string("%s/%d");
        vtitles.push_back(std::string("database"));
        vtitles.push_back(std::string("field number"));
    break;
        case PalmLib::FlatFile::Field::LINKED:
        format = std::string("%d/%d");
        vtitles.push_back(std::string("link field number"));
        vtitles.push_back(std::string("field number"));
    break;
        case PalmLib::FlatFile::Field::CALCULATED:
    case PalmLib::FlatFile::Field::BOOLEAN:
        case PalmLib::FlatFile::Field::NOTE:
    default:
        format = std::string("");
    break;
    }
    return vtitles;
}

unsigned PalmLib::FlatFile::DB::getMaxNumOfListViews() const
{
    return 0;
}

void PalmLib::FlatFile::DB::doneWithSchema()
{
    // Let the superclass have a chance.
    SUPERCLASS(PalmLib::FlatFile, Database, doneWithSchema, ());
/* false from the 0.3.3 version
    if (getNumOfListViews() < 1)
        throw PalmLib::error("at least one list view must be specified");
*/
}

void PalmLib::FlatFile::DB::setOption(const std::string& name,
                                      const std::string& value)
{
    if (name == "find") {
        if (!StrOps::string2boolean(value))
            m_flags &= ~(1);
        else
            m_flags |= 1;
    } else if (name == "read-only"
                || name == "readonly") {
        if (!StrOps::string2boolean(value))
            m_flags &= ~(0x8000);
        else
            m_flags |= 0x8000;
    } else {
        SUPERCLASS(PalmLib::FlatFile, Database, setOption, (name, value));
    }
}

PalmLib::FlatFile::Database::options_list_t
PalmLib::FlatFile::DB::getOptions(void) const
{
    typedef PalmLib::FlatFile::Database::options_list_t::value_type value;
    PalmLib::FlatFile::Database::options_list_t result;

    result = SUPERCLASS(PalmLib::FlatFile, Database, getOptions, ());

    if (m_flags & 1)
        result.push_back(value("find", "true"));

    if (m_flags & 0x8000)
        result.push_back(value("read-only", "true"));

    return result;
}
