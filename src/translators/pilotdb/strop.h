/*
 * palm-db-tools: Support Library: String Parsing Utility Functions
 * Copyright (C) 1999-2000 by Tom Dyas (tdyas@users.sourceforge.net)
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

#ifndef __LIBSUPPORT_STROP_H__
#define __LIBSUPPORT_STROP_H__

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <time.h>
#include "libflatfile/Database.h"

namespace StrOps {

    // This exception is thrown whenever an error is encountered in
    // csv_to_array and str_to_array.
    class csv_parse_error : public std::runtime_error {
    public:
      csv_parse_error(const std::string& msg) : std::runtime_error(msg) { }
    };

    class csv_unterminated_quote : public std::runtime_error {
    public:
      csv_unterminated_quote(const std::string& msg) : std::runtime_error(msg) { }
    };

    // The results of any parse are returned as this type.
    typedef std::vector<std::string> string_list_t;


    /**
     * Convert all uppercase characters in a string to lowercase.
     */
    void lower(std::string& str);

    /**
     * Convert a string to a boolean value.
     *
     * @param str The string containing a boolean value to convert.
     */
    bool string2boolean(const std::string& str);

    /**
     * Convert a string to a FieldType value.
     *
     * @param typestr The string containing a FieldType value to convert.
     */
    PalmLib::FlatFile::Field::FieldType string2type(std::string typestr);

    /**
     * Convert a FieldType value to a string.
     *
     * @param t The FieldType value containing a string to convert.
     */
    std::string type2string(PalmLib::FlatFile::Field::FieldType t);

    /**
     * Strip trailing characters from a string.
     *
     * @param str  The string to strip characters from.
     * @param what The string containing characters to strip.
     */
    std::string strip_back(const std::string& str, const std::string& what);

    /**
     * Strip leading characters from a string.
     *
     * @param str  The string to strip characters from.
     * @param what The string containing characters to strip.
     */
    std::string strip_front(const std::string& str, const std::string& what);

    /**
     * Convert a string to a target type using a istringstream.
     */
    template<class T>
    inline void convert_string(const std::string& str, T& result) {
      std::istringstream(str.c_str()) >> result;
    }

    /**
     * Parse a string in CSV (comman-seperated values) format and
     * return it as a list.
     *
     * @param str The string containing the CSV fields.
     * @param delim The field delimiter. Defaults to a comma.
     */
    string_list_t csv_to_array(const std::string& str, char delim = ',', bool quoted_string = true);


    /**
     * Parse an argv-style array and return it as a list.
     *
     * @param str             The string to parse.
     * @param delim           String containing the delimiter characters.
     * @param multiple_delim  Should multiple delimiters count as one?
     * @param handle_comments Handle # as a comment character.
     */
    string_list_t str_to_array(const std::string& str,
             const std::string& delim,
             bool multiple_delim,
             bool handle_comments);

    /**
     * return the current date in the palm format.
     */
    PalmLib::pi_uint32_t get_current_time(void);

    /**
     * fill a char array with a tm structure in the format passed.
     * @param s the char array filled.
     * @param format the string of the format to use to print the date.
     * @param tm a pointer to time structure.
     */
    char *strptime(const char *s, const char  *format,  struct tm *tm);

    /**
     * read one line from the input stream of a file
     */
    std::string readline(std::istream& stream);

    /**
     * add the quotes to a string
     */
    std::string quote_string(std::string str, bool extended_mode);

    /**
     * concatenate the path directory, the file name and the extension
     * to give the file path to a file
     */
    std::string concatenatepath(std::string p_Path, std::string p_FileName,
        std::string p_Ext = std::string(""));

} // namespace StrOps

#endif
