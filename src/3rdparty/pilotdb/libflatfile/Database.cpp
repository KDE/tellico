/*
 * palm-db-tools: Abstract adaptor for flat-file databases.
 * Copyright (C) 1999-2000 by Tom Dyas (tdyas@users.sourceforge.net)
 */

#include "Database.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <cctype>

#include <kdebug.h>

PalmLib::FlatFile::Database::Database(std::string p_Type, const PalmLib::Database& pdb)
    : m_Type(p_Type)
{
    title(pdb.name());
    m_backup = pdb.backup();
    m_readonly = pdb.readonly();
    m_copy_prevention = pdb.copy_prevention();
}

void
PalmLib::FlatFile::Database::outputPDB(PalmLib::Database& pdb) const
{
    pdb.name(title());
    pdb.backup(m_backup);
    pdb.readonly(m_readonly);
    pdb.copy_prevention(m_copy_prevention);
}

std::string
PalmLib::FlatFile::Database::title() const
{
    return m_title;
}

void
PalmLib::FlatFile::Database::title(const std::string& title)
{
    m_title = title;
}

unsigned
PalmLib::FlatFile::Database::getNumOfFields() const
{
    return m_fields.size();
}

std::string
PalmLib::FlatFile::Database::field_name(int i) const
{
    return m_fields[i].title();
/*    return m_fields[i].first;*/
}

PalmLib::FlatFile::Field::FieldType
PalmLib::FlatFile::Database::field_type(int i) const
{
    return m_fields[i].type();
/*    return m_fields[i].second;*/
}

PalmLib::FlatFile::FType
PalmLib::FlatFile::Database::field(int i) const
{
    return m_fields[i];
}

void
PalmLib::FlatFile::Database::appendField(PalmLib::FlatFile::FType field)
{
    if (! supportsFieldType(field.type())) {
//        throw PalmLib::error("unsupported field type");
          kDebug() << "unsupported field type";
          return;
    }
    if (getMaxNumOfFields() != 0 && getNumOfFields() + 1 > getMaxNumOfFields()) {
//        throw PalmLib::error("maximum number of fields reached");
          kDebug() << "maximum number of fields reached";
          return;
    }
    m_fields.push_back(field);
}

void
PalmLib::FlatFile::Database::appendField(const std::string& name,
                                         Field::FieldType type, std::string data)
{
    if (! supportsFieldType(type)) {
      kDebug() << "unsupported field type";
        return;
    }
//        throw PalmLib::error("unsupported field type");
    if (getMaxNumOfFields() != 0 && getNumOfFields() + 1 > getMaxNumOfFields()) {
      kDebug() << "maximum number of fields reached";
        return;
      }
//        throw PalmLib::error("maximum number of fields reached");

/*    m_fields.push_back(std::make_pair(name, type));*/
/*    m_fields.push_back(PalmLib::FlatFile::make_ftype(name, type));*/
    m_fields.push_back(PalmLib::FlatFile::FType(name, type, data));
}

void
PalmLib::FlatFile::Database::insertField(int i, PalmLib::FlatFile::FType field)
{
    if (! supportsFieldType(field.type())) {
//        throw PalmLib::error("unsupported field type");
        kDebug() << "unsupported field type";
        return;
    }
    if (getMaxNumOfFields() != 0 && getNumOfFields() + 1 > getMaxNumOfFields()) {
//        throw PalmLib::error("maximum number of fields reached");
        kDebug() << "maximum number of fields reached";
        return;
      }
/*    m_fields.push_back(std::make_pair(name, type));*/
/*    m_fields.push_back(PalmLib::FlatFile::make_ftype(name, type));*/
    m_fields.insert(m_fields.begin() + i, field);
}

void
PalmLib::FlatFile::Database::insertField(int i, const std::string& name,
                                         Field::FieldType type, std::string data)
{
    if (! supportsFieldType(type)) {
//        throw PalmLib::error("unsupported field type");
        kDebug() << "unsupported field type";
        return;
    }
    if (getMaxNumOfFields() != 0 && getNumOfFields() + 1 > getMaxNumOfFields()) {
//        throw PalmLib::error("maximum number of fields reached");
        kDebug() << "maximum number of fields reached";
        return;
      }
/*    m_fields.push_back(std::make_pair(name, type));*/
/*    m_fields.push_back(PalmLib::FlatFile::make_ftype(name, type));*/
    m_fields.insert(m_fields.begin() + i, PalmLib::FlatFile::FType(name, type, data));
}

void
PalmLib::FlatFile::Database::removeField(int i)
{
    m_fields.erase(m_fields.begin() + i);
}

unsigned
PalmLib::FlatFile::Database::getNumRecords() const
{
    return m_records.size();
}

PalmLib::FlatFile::Record
PalmLib::FlatFile::Database::getRecord(unsigned index) const
{
    if (index >= getNumRecords()) {
      kDebug() << "invalid index";
     //throw std::out_of_range("invalid index");
    }
    return m_records[index];
}

void
PalmLib::FlatFile::Database::appendRecord(PalmLib::FlatFile::Record rec)
{
    if (rec.fields().size() != getNumOfFields()) {
//        throw PalmLib::error("the number of fields mismatch");
        kDebug() << "the number of fields mismatch";
        return;
    }
    for (unsigned int i = 0; i < getNumOfFields(); i++) {
        const Field field = rec.fields().at(i);
        if (field.type != field_type(i)) {
          kDebug() << "field " << i << " type " << field_type(i) << " mismatch: " << field.type;
          return;
//            throw PalmLib::error(buffer.str());
        }
    }
    m_records.push_back(rec);
}

void
PalmLib::FlatFile::Database::deleteRecord(unsigned index)
{
    m_records.erase(m_records.begin() + index);
}

void
PalmLib::FlatFile::Database::clearRecords()
{
    m_records.clear();
}

unsigned
PalmLib::FlatFile::Database::getNumOfListViews() const
{
    return m_listviews.size();
}

PalmLib::FlatFile::ListView
PalmLib::FlatFile::Database::getListView(unsigned index) const
{
    return m_listviews[index];
}

void
PalmLib::FlatFile::Database::setListView(unsigned index,
                                         const PalmLib::FlatFile::ListView& lv)
{
    // Ensure that the field numbers are within range.
    for (PalmLib::FlatFile::ListView::const_iterator i = lv.begin();
         i != lv.end(); ++i) {
        if ((*i).field >= getNumOfFields())
            return;
    }

    m_listviews[index] = lv;
}

void
PalmLib::FlatFile::Database::appendListView(const ListView& lv)
{
    // Enforce any limit of the maximum number of list views.
    if (getMaxNumOfListViews() != 0
        && getNumOfListViews() + 1 > getMaxNumOfListViews())
        return;
//        throw PalmLib::error("too many list views for this database type");

    // Ensure that the field numbers are within range.
    for (PalmLib::FlatFile::ListView::const_iterator i = lv.begin();
         i != lv.end(); ++i) {
        if ((*i).field >= getNumOfFields())
            return;
    }
    m_listviews.push_back(lv);
}

void
PalmLib::FlatFile::Database::removeListView(unsigned index)
{
    if (index < getNumOfListViews())
        m_listviews.erase( m_listviews.begin()+index);
}

static void
strlower(std::string& str)
{
    for (std::string::iterator p = str.begin(); p != str.end(); ++p) {
        if (isupper(*p))
            *p = tolower(*p);
    }
}

static bool
string2boolean(std::string str)
{
    strlower(str);
    if (str == "on")
        return true;
    else if (str == "off")
        return false;
    else if (str == "true")
        return true;
    else if (str == "t")
        return true;
    else if (str == "false")
        return false;
    else if (str == "f")
        return false;
    else {
        int num = 0;

        std::istringstream(str.c_str()) >> num;
        return num != 0 ? true : false;
    }
}

void
PalmLib::FlatFile::Database::setOption(const std::string& name,
                                       const std::string& value)
{
    if (name == "backup")
        m_backup = string2boolean(value);
    else if (name == "inROM")
        m_readonly = string2boolean(value);
    else if (name == "copy-prevention")
        m_copy_prevention = string2boolean(value);
}

PalmLib::FlatFile::Database::options_list_t
PalmLib::FlatFile::Database::getOptions() const
{
    PalmLib::FlatFile::Database::options_list_t set;
    typedef PalmLib::FlatFile::Database::options_list_t::value_type value;

    if (m_backup)
        set.push_back(value("backup", "true"));
    else
        set.push_back(value("backup", "false"));

    if (m_readonly)
        set.push_back(value("inROM", "true"));

    if (m_copy_prevention)
        set.push_back(value("copy-prevention", "true"));

    return set;
}

void
PalmLib::FlatFile::Database::doneWithSchema()
{
    // Ensure that the database has at least one field.
    if (getNumOfFields() == 0)
        return;
//        throw PalmLib::error("at least one field must be specified");

    // Ensure that the database has a title.
    if (title().empty())
        return;
//        throw PalmLib::error("a title must be specified");
}
