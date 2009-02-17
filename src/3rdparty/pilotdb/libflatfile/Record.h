/*
 * palm-db-tools: Field definitions for flat-file database objects.
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 */

#ifndef __PALMLIB_FLATFILE_RECORD_H__
#define __PALMLIB_FLATFILE_RECORD_H__

#include <vector>

#include "Field.h"

namespace PalmLib {
    namespace FlatFile {
//  typedef std::vector<Field> Record;

  class Record{
        public:

      const std::vector<Field> fields() const { return m_Fields; }

      void appendField(Field newfield) { m_Fields.push_back(newfield); }
            bool created() const { return m_New;}
            void created(bool on){ m_New = on;}
            bool secret() const { return m_Secret;}
            void secret(bool on) { m_Secret = on;}

      bool dirty() const { return m_Dirty; }
      void dirty( bool on) { m_Dirty = on; }

            pi_uint32_t  unique_id() const { return m_UID; }
            void unique_id(pi_uint32_t id) { m_UID = id; }
        private:

      std::vector<Field> m_Fields;
            bool m_Secret;
            bool m_New;

      bool m_Dirty;
            pi_uint32_t m_UID;
        };
    }
}

#endif
