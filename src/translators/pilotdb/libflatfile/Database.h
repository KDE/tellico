/*
 * palm-db-tools: Abstract adaptor for flat-file databases.
 * Copyright (C) 1999-2000 by Tom Dyas (tdyas@users.sourceforge.net)
 */

#ifndef __PALMLIB_FLATFILE_DATABASE_H__
#define __PALMLIB_FLATFILE_DATABASE_H__

#include <vector>
#include <string>
#include <utility>

#include "../libpalm/Database.h"
#include "Field.h"
#include "Record.h"
#include "ListView.h"
#include "FType.h"

#define NOTETITLE_LENGTH 32

namespace PalmLib {
    namespace FlatFile {

  // This class is an in-memory representation of a typical
  // PalmOS flat-file database. The caller can request write the
  // data to a real PalmLib::Database object at any time to
  // actually obtain the data in a format usable on the Palm
  // Pilot.

  class Database {
  public:
      // convenience type for the options list parsing
      typedef std::vector< std::pair< std::string, std::string> > options_list_t;

      /**
       * Default constructor which creates an empty
       * database. Subclasses should provide a default
       * constructor and an additional constructorwhich takes a
       * PalmOS::Database as an argument.
       */
      Database(std::string p_Type)
            : m_backup(false), m_readonly(false),
              m_copy_prevention(false), m_Type(p_Type)
            { }

        /**
         * Constructor which fills the flat-file structure from a
         * PalmOS database.
       *
       * @param pdb PalmOS database to read from.
         */
        Database(std::string p_Type, const PalmLib::Database& pdb);

      /**
       * The destructor is empty since we have no other objects
       * to dispose of. It is virtual since we have subclasses
       * for specific flat-file database products.
       */
      virtual ~Database() { }

      /**
       * After all processing to add fields and records is done,
       * outputPDB is called to create the actual file format
       * used by the flat-file database product. This method is
       * abstract since only subclasses know the specific file
       * formats.
       *
       * @param pdb An instance of PalmLib::Database.
       */
      virtual void outputPDB(PalmLib::Database& pdb) const;

      /**
       * Return the title of this flat-file database.
       */
      virtual std::string title() const;

      /**
       * Set the title of this database.
       *
       * @param title New title of the database.
       */
      virtual void title(const std::string& title);

      /**
       * Return the maximum number of fields allowed in the
       * database. The object will not allow the number of
       * fields to exceed the returned value. This method is
       * abstract since only the subclasses know the limit on
       * fields. 0 is returned if there is no limit.
       */
      virtual unsigned getMaxNumOfFields() const = 0;

      /**
       * Return the number of fields in the database.
       */
      virtual unsigned getNumOfFields() const;

      /**
       * Accessor function for the name of a field.
       */
      virtual std::string field_name(int i) const;

      /**
       * Accessor function for type of a field.
       */
      virtual Field::FieldType field_type(int i) const;

      /**
       * Accessor function for the field informations
       */
      virtual FType field(int i) const;

      /**
       * write the format of the field's argument in format,
       * and return a strings' vector with name of each argument part.
       * the format use the same display as used by printf
       */
      virtual std::vector<std::string> field_argumentf(int, std::string& format)
                 { format = std::string(""); return std::vector<std::string>(0, std::string(""));}

      /**
       * Add a field to the flat-file database. An exception
       * will be thrown if the maximum number of fields would be
       * exceeded or the field type is unsupported.
       *
       * @param name Name of the new field.
       * @param type The type of the new field.
       */
      virtual void appendField(FType field);
      virtual void appendField(const std::string& name,
             Field::FieldType type, std::string data = std::string(""));

      /**
       * Insert a field to the flat-file database. An exception
       * will be thrown if the maximum number of fields would be
       * exceeded or the field type is unsupported.
       *
       * @param name Name of the new field.
       * @param type The type of the new field.
       */
      virtual void insertField(int i, FType field);
      virtual void insertField(int i, const std::string& name,
             Field::FieldType type, std::string data = std::string(""));

      /**
       * Remove a Field in the flat-file database. An Exception
       * will thrown if the field doesn't exist.
       */
      virtual void removeField(int i);

      /**
       * Returns true if this database supports a specific field
       * type. This method is abstract since only the subclasses
       * know which field types are supported.
       *
       * @param type The field type that should be checked for support. 
       */
      virtual bool supportsFieldType(const Field::FieldType& type) const = 0;

      /**
       * Return the number of records in the database.
       */
      virtual unsigned getNumRecords() const;

      /**
       * Return the record with the given index. The caller gets
       * a private copy of the data and _not_ a reference to the
       * data.
       *
       * @param index Index of the record to retrieve.
       */
      virtual Record getRecord(unsigned index) const;

      /**
       * Append a record to the database. An exception will be
       * thrown if their are not enough fields or if field types
       * mismatch.
       *
       * @param rec The record to append.
       */
      virtual void appendRecord(Record rec);

      /**
       * Remove all records from the database
       */
      virtual void clearRecords();

      /**
       * Remove a record from the database
       */
      virtual void deleteRecord(unsigned index);

      /**
       * Return the maximum number of views supported by this
       * type of flat-file database. This method is abstract
       * since only the subclasses know the exact value.
       */
      virtual unsigned getMaxNumOfListViews() const = 0;

      /**
       * Return the actual number of views present in this
       * database.
       */
      virtual unsigned getNumOfListViews() const;

      /**
       * Return a copy of the list view at the given index.
       *
       * @param index Index of the list view to return.
       */
      virtual ListView getListView(unsigned index) const;

      /**
       * Set the list view at the given index to the new list
       * view. An exception may be thrown if field numbers are
       * invalid or the list view doesn't pass muster with the
       * subclass.
       *
       * @param index    Index of the list view to set.
       * @param listview The new list view.
       */
      virtual void setListView(unsigned index, const ListView& listview);

      /**
       * Append a new list view. This will fail if the maximum
       * number of list views would be exceeded.
       *
       * @param listview The new list view to append.
       */
      virtual void appendListView(const ListView& listview);

      /**
       * Remove a list view.
       *
       * @param index Index of the list view to remove.
       */
      virtual void removeListView(unsigned index);

      /**
       * Process a special option. If the option is not
       * supported, then it is silently ignored. Subclasses
       * should call the base class first so that options common
       * to all flat-file databases can be processed.
       *
       * @param name  Name of the option.
       * @param value String value assigned to the option.  */
      virtual void setOption(const std::string& name,
           const std::string& value);

      /**
       * Return a list of of all extra options supported by this
       * database. Subclasses should call the base class first
       * and then merge any extra options. Get a list of extra
       * options.
       */
      virtual options_list_t getOptions(void) const;

      /**
       * Hook function which should be invoked by a caller after
       * all calls the meta-deta functions have completed. This
       * allows the database type-specific code to do final
       * checks on the meta-data. An exception will be throw if
       * there is an error. Otherwise, nothing will happen.
       */
      virtual void doneWithSchema();

      /**
       * Change and Return the about information
       * of the database when it's supportted
       */
      virtual void setAboutInformation(std::string _string)
        {
            about.information = _string;
        }
        
      virtual std::string getAboutInformation() const
        {
            return about.information;
        }

      std::string type() const
        {
            return m_Type;
        }

  private:
      // We provide a dummy copy constructor and assignment
      // operator in order to prevent any copying of the object.
      Database(const Database&) { }
      Database& operator = (const Database&) { return *this; }

/*      typedef std::vector< std::pair< std::string, Field::FieldType > >*/
        typedef std::vector< FType>
    field_list_t;
      typedef std::vector<Record> record_list_t;
      typedef std::vector<ListView> listview_list_t;

        typedef std::vector< std::pair< std::string, std::vector< std::string > > >
        listitems_list_t;

      field_list_t m_fields;       // database schema
      record_list_t m_records;     // the database itself
        listitems_list_t m_list;     // the items lists include in the database
      listview_list_t m_listviews; // list views
      bool m_backup;               // backup flag for PDB
      bool m_readonly;             // readonly flag for PDB
      bool m_copy_prevention;      // copy prevention for PDB
      std::string m_title;         // name of database
        class About
        {
        public:
            std::string information;
        } about;
      std::string m_Type;
  };

    } // namespace FlatFile
} // namespace PalmLib

#endif
