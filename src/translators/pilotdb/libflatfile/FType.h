/*
 * palm-db-tools: Field Type definitions for flat-file database objects.
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 */

#ifndef __PALMLIB_FLATFILE_FTYPE_H__
#define __PALMLIB_FLATFILE_FTYPE_H__

#include <string>
#include <utility>

#include "../libpalm/palmtypes.h"
#include "Field.h"

namespace PalmLib {
    namespace FlatFile {

  class FType {
  public:
        friend class PalmLib::FlatFile::Field;
        FType(std::string title, PalmLib::FlatFile::Field::FieldType type) : 
            m_title(title), m_type(type), m_data("") { }

        FType(std::string title, PalmLib::FlatFile::Field::FieldType type, std::string data) : 
            m_title(title), m_type(type), m_data(data) { }

        virtual ~FType() { }
        
        std::string title() const {return m_title;}
        virtual PalmLib::FlatFile::Field::FieldType type() const
            { return m_type;}

        virtual std::string argument() const { return m_data;}

        void set_argument( const std::string data) { m_data = data;}

        void setTitle( const std::string value) { m_title = value;}
        void setType( const PalmLib::FlatFile::Field::FieldType value) { m_type = value;}
    private:
        std::string         m_title;
        PalmLib::FlatFile::Field::FieldType    m_type;
        
        std::string         m_data;
    };
  }
}

#endif
