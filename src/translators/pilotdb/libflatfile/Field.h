/*
 * palm-db-tools: Field definitions for flat-file database objects.
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 */

#ifndef __PALMLIB_FLATFILE_FIELD_H__
#define __PALMLIB_FLATFILE_FIELD_H__

#include <string>

#include "../libpalm/palmtypes.h"

namespace PalmLib {
    namespace FlatFile {

  class Field {
  public:
      Field() : no_value(false), type(STRING), v_boolean(false),
    v_integer(0), v_float(0) { }
    ~Field() { }

      // True if this field has no value. (NULL in SQL terms)
      bool no_value;

      enum FieldType {
        STRING,
        BOOLEAN,
        INTEGER,
        FLOAT,
        DATE,
        TIME,
        DATETIME,
        LIST,
        LINK,
        NOTE,
        CALCULATED,
        LINKED,
        LAST
      };

      enum FieldType type;

      std::string v_string;
      std::string v_note;
      bool v_boolean;
      PalmLib::pi_int32_t v_integer;
      long double v_float;
      struct {
    int month;
    int day;
    int year;
      } v_date;          // valid for DATE and DATETIME
      struct {
    int hour;
    int minute;
      } v_time;          // valid for TIME and DATETIME

  /*
    friend Field operator = (const Field& y)
    {
      this.v_string = y.v_string;
      this.v_boolean = y.v_boolean;
      this.v_integer = y.v_integer;
      this.v_float = y.v_float;
      this.v_date.month = y.v_date.month;
            this.v_date.day = y.v_date.day;
            this.v_date.year = y.v_date.year;
      this.v_time.hour = y.v_time.hour;
            this.v_time.minute = y.v_time.minute;
      this.v_note = y.v_note;
    }
*/
        friend bool operator==(const Field& x, const Field& y)
        {
            if (x.type != y.type)
                return false;
            switch (x.type) {
            case STRING:
                return (x.v_string == y.v_string);
            case BOOLEAN:
                return (x.v_boolean == y.v_boolean);
            case INTEGER:
                return (x.v_integer == y.v_integer);
            case FLOAT:
                return (x.v_float == y.v_float);
            case DATE:
                return (x.v_date.month == y.v_date.month
                        && x.v_date.day == y.v_date.day
                        && x.v_date.year == y.v_date.year);
            case TIME:
                return (x.v_time.hour == y.v_time.hour
                        && x.v_time.minute == y.v_time.minute);
            case DATETIME:
                return (x.v_date.month == y.v_date.month
                        && x.v_date.day == y.v_date.day
                        && x.v_date.year == y.v_date.year
                        && x.v_time.hour == y.v_time.hour
                        && x.v_time.minute == y.v_time.minute);
            case LIST:
                return (x.v_string == y.v_string);
            case LINK:
                return (x.v_string == y.v_string);
            case NOTE:
                return (x.v_string == y.v_string
                        && x.v_note == y.v_note);
            case CALCULATED:
                return true; //a calculated field could be recalculate at each time
            case LINKED:
                return (x.v_string == y.v_string);
            default:
                return (x.v_string == y.v_string);
            }
    }
  };

  }
}

#endif
