/*
 * This class provides access to DB-format databases.
 */

#ifndef __PALMLIB_FLATFILE_DB_H__
#define __PALMLIB_FLATFILE_DB_H__

#include <map>
#include <string>

#include "../libpalm/Block.h"
#include "../libpalm/Database.h"
#include "Database.h"

namespace PalmLib {
    namespace FlatFile {

  class DB : public Database {
  public:
      /**
       * Return true if this class can handle the given PalmOS
       * database.
       *
       * @param pdb PalmOS database to check for support.
       */
      static bool classify(PalmLib::Database& pdb);

      /**
       * Return true if this class is the database identified by
       * name.
       *
       * @param name A database type name to check.
       */
      static bool match_name(const std::string& name);

      /**
       * Default constructor for an initially empty database.
       */
      DB():Database("db"), m_flags(0) { }

      /**
       * Constructor which fills the flat-file structure from a
       * PalmOS database.
       */
      DB(PalmLib::Database&);

      // destructor
      virtual ~DB() { }

        /**
         * After all processing to add fields and records is done,
         * outputPDB is called to create the actual file format
         * used by the flat-file database product.
         *
         * @param pdb An instance of PalmLib::Database.
       */
        virtual void outputPDB(PalmLib::Database& pdb) const;

        /**
         * Return the maximum number of fields allowed in the
         * database. This class returns 0 since there is no limit.
       */
        virtual unsigned getMaxNumOfFields() const;

      /**
       * Return true for the field types that this class
       * currently supports. Returns false otherwise.
       *
       * @param type The field type to check for support.
       */
      virtual bool supportsFieldType(const Field::FieldType& type) const;

        /**
         * write the format of the field's argument in format,
         * and return a strings' vector with name of each argument part.
         * the format use the same display as used by printf
         */
        virtual std::vector<std::string> field_argumentf(int i, std::string& format);

        /**
         * Return the maximum number of views supported by this
         * type of flat-file database.
       */
      virtual unsigned getMaxNumOfListViews() const;

      /**
       * Hook the end of the schema processing.
       */
      virtual void doneWithSchema();

      /**
       * Set a extra option.
       *
       * @param opt_name  The name of the option to set.
       * @param opt_value The value to assign to this option.
       */
      virtual void setOption(const std::string& name,
           const std::string& value);

      /**
       * Get a list of extra options.
       */
      virtual options_list_t getOptions(void) const;

      // Produce a PalmOS record from a flat-file record.
      void make_record(PalmLib::Record& pdb_record,
           const PalmLib::FlatFile::Record& record) const;

  private:
      pi_uint16_t m_flags;

      class Chunk : public PalmLib::Block {
      public:
    Chunk() : PalmLib::Block(), chunk_type(0) { }
    Chunk(const Chunk& rhs)
        : PalmLib::Block(rhs), chunk_type(rhs.chunk_type) { }
    Chunk(PalmLib::Block::const_pointer data,
          const PalmLib::Block::size_type size)
        : PalmLib::Block(data, size), chunk_type(0) { }
    Chunk& operator = (const Chunk& rhs) {
        Block::operator = (rhs);
        chunk_type = rhs.chunk_type;
        return *this;
    }

    pi_uint16_t chunk_type;
      };

      typedef std::map<pi_uint16_t, std::vector<Chunk> > chunks_t;
      chunks_t m_chunks;

      // Extract the chunks from an app info block to m_chunks.
      void extract_chunks(const PalmLib::Block&);

      // Extract the schema.
      void extract_schema(unsigned numFields);

      // Extract the list views from the app info block.
      void extract_listviews();

        //extract the field data
        std::string extract_fieldsdata(pi_uint16_t field_search,
                  PalmLib::FlatFile::Field::FieldType type);

        void extract_aboutinfo();

      // Determine location and size of each field.
      void parse_record(PalmLib::Record& record,
            std::vector<pi_char_t *>& ptrs,
            std::vector<size_t>& sizes);

      // The following routines build various types of chunks
      // for the app info block and assemble them all.
        void build_fieldsdata_chunks(std::vector<Chunk>& chunks) const;
      void build_standard_chunks(std::vector<Chunk>&) const;
      void build_listview_chunk(std::vector<Chunk>&,
              const ListView&) const;
  void build_about_chunk(std::vector<Chunk>& chunks) const;
      void build_appinfo_block(const std::vector<Chunk>&,
             PalmLib::Block&) const;
  };

    }
}

#endif
