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

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "strop.h"
#include <kdebug.h>

extern std::ostream* err;

void StrOps::lower(std::string& str)
{
    for (std::string::iterator p = str.begin(); p != str.end(); ++p) {
        if (isupper(*p))
            *p = tolower(*p);
    }
}

bool StrOps::string2boolean(const std::string& str)
{
    std::string value(str);

    StrOps::lower(value);

    if (value == "on")       return true;
    else if (str == "off")   return false;
    else if (str == "true")  return true;
    else if (str == "t")     return true;
    else if (str == "false") return false;
    else if (str == "f")     return false;
    else {
        int num = 0;

        std::istringstream(str.c_str()) >> num;
        return num != 0 ? true : false;
    }
}

std::string StrOps::type2string(PalmLib::FlatFile::Field::FieldType t)
{
    switch (t) {
    case PalmLib::FlatFile::Field::STRING:
        return "string";

    case PalmLib::FlatFile::Field::BOOLEAN:
        return "boolean";

    case PalmLib::FlatFile::Field::INTEGER:
        return "integer";

    case PalmLib::FlatFile::Field::FLOAT:
        return "float";

    case PalmLib::FlatFile::Field::DATE:
        return "date";

    case PalmLib::FlatFile::Field::TIME:
        return "time";

    case PalmLib::FlatFile::Field::DATETIME:
        return "datetime";

    case PalmLib::FlatFile::Field::NOTE:
        return "note";

    case PalmLib::FlatFile::Field::LIST:
        return "list";

    case PalmLib::FlatFile::Field::LINK:
        return "link";

    case PalmLib::FlatFile::Field::CALCULATED:
        return "calculated";

    case PalmLib::FlatFile::Field::LINKED:
        return "linked";

    default:
        // If we don't support the field type, then fake it as a string.
        return "string";
    }
}

PalmLib::FlatFile::Field::FieldType StrOps::string2type(std::string typestr)
{
    StrOps::lower(typestr);
    if (typestr == "string")
        return PalmLib::FlatFile::Field::STRING;
    else if (typestr == "str")
        return PalmLib::FlatFile::Field::STRING;
    else if (typestr == "note")
        return PalmLib::FlatFile::Field::NOTE;
    else if (typestr == "bool")
        return PalmLib::FlatFile::Field::BOOLEAN;
    else if (typestr == "boolean")
        return PalmLib::FlatFile::Field::BOOLEAN;
    else if (typestr == "integer")
        return PalmLib::FlatFile::Field::INTEGER;
    else if (typestr == "int")
        return PalmLib::FlatFile::Field::INTEGER;
    else if (typestr == "float")
        return PalmLib::FlatFile::Field::FLOAT;
    else if (typestr == "date")
        return PalmLib::FlatFile::Field::DATE;
    else if (typestr == "time")
        return PalmLib::FlatFile::Field::TIME;
    else if (typestr == "datetime")
        return PalmLib::FlatFile::Field::DATETIME;
    else if (typestr == "list")
        return PalmLib::FlatFile::Field::LIST;
    else if (typestr == "link")
        return PalmLib::FlatFile::Field::LINK;
    else if (typestr == "linked")
        return PalmLib::FlatFile::Field::LINKED;
    else if (typestr == "calculated")
        return PalmLib::FlatFile::Field::CALCULATED;
    else
        kdDebug() << "unknown field type" << endl;
    return PalmLib::FlatFile::Field::STRING;
}

std::string StrOps::strip_back(const std::string& str, const std::string& what)
{
    std::string result(str);
    std::string::reverse_iterator p = result.rbegin();

    while (p != result.rend()
           && (std::find(what.begin(), what.end(), *p) != what.end())) ++p;

    result.erase(p.base(), result.end());

    return result;
}

std::string StrOps::strip_front(const std::string& str,const std::string& what)
{
    std::string result(str);
    std::string::iterator p = result.begin();

    while (p != result.end()
           && (std::find(what.begin(), what.end(), *p) != what.end())) ++p;

    result.erase(result.begin(), p);

    return result;
}

StrOps::string_list_t StrOps::csv_to_array(const std::string& str, char delim, bool quoted_string)
{
    enum { STATE_NORMAL, STATE_QUOTES } state;
    StrOps::string_list_t result;
    std::string elem;

    state = STATE_NORMAL;
    for (std::string::const_iterator p = str.begin(); p != str.end(); ++p) {
        switch (state) {
        case STATE_NORMAL:
            if (quoted_string && *p == '"') {
                state = STATE_QUOTES;
            } else if (*p == delim) {
                result.push_back(elem);
                elem = "";
            } else {
                elem += *p;
            }
        break;

        case STATE_QUOTES:
            if (quoted_string && *p == '"') {
                if ((p + 1) != str.end() && *(p+1) == '"') {
                    ++p;
                    elem += '"';
                } else {
                    state = STATE_NORMAL;
                }
            } else {
                elem += *p;
            }
        break;
        }
    }

    switch (state) {
    case STATE_NORMAL:
        result.push_back(elem);
    break;
    case STATE_QUOTES:
        kdDebug() << "unterminated quotes" << endl;
    break;
    }

    return result;
}

StrOps::string_list_t
StrOps::str_to_array(const std::string& str, const std::string& delim,
                     bool multiple_delim, bool handle_comments)
{
    enum { STATE_NORMAL, STATE_COMMENT, STATE_QUOTE_DOUBLE, STATE_QUOTE_SINGLE,
           STATE_BACKSLASH, STATE_BACKSLASH_DOUBLEQUOTE } state;
    StrOps::string_list_t result;
    std::string elem;

    state = STATE_NORMAL;
    for (std::string::const_iterator p = str.begin(); p != str.end(); ++p) {
        switch (state) {
        case STATE_NORMAL:
            if (*p == '"') {
                state = STATE_QUOTE_DOUBLE;
            } else if (*p == '\'') {
                state = STATE_QUOTE_SINGLE;
            } else if (std::find(delim.begin(), delim.end(), *p) != delim.end()) {
                if (multiple_delim) {
                    ++p;
                    while (p != str.end()
                       && std::find(delim.begin(), delim.end(), *p) != delim.end()) {
                        ++p;
                    }
                    --p;
                }
                result.push_back(elem);
                elem = "";
            } else if (*p == '\\') {
                state = STATE_BACKSLASH;
            } else if (handle_comments && *p == '#') {
                state = STATE_COMMENT;
            } else {
                elem += *p;
            }
        break;

        case STATE_COMMENT:
        break;

        case STATE_QUOTE_DOUBLE:
            if (*p == '"')
                state = STATE_NORMAL;
            else if (*p == '\\')
                state = STATE_BACKSLASH_DOUBLEQUOTE;
            else
                elem += *p;
        break;

        case STATE_QUOTE_SINGLE:
            if (*p == '\'')
                state = STATE_NORMAL;
            else
                elem += *p;
            break;

        case STATE_BACKSLASH:
            elem += *p;
            state = STATE_NORMAL;
        break;

        case STATE_BACKSLASH_DOUBLEQUOTE:
            switch (*p) {
            case '\\':
                elem += '\\';
            break;

            case 'n':
                elem += '\n';
            break;

            case 'r':
                elem += '\r';
            break;

            case 't':
                elem += '\t';
            break;

            case 'v':
                elem += '\v';
            break;

            case '"':
                elem += '"';
            break;

            case 'x':
                {
                char buf[3];

                // Extract and check the first hexadecimal character.
                if ((p + 1) == str.end())
                    kdDebug() << "truncated escape" << endl;
                if (! isxdigit(*(p + 1)))
                    kdDebug() << "invalid hex character" << endl;
                buf[0] = *++p;

                // Extract and check the second (if any) hex character.
                if ((p + 1) != str.end() && isxdigit(*(p + 1))) {
                    buf[1] = *++p;
                    buf[2] = '\0';
                } else {
                    buf[1] = buf[2] = '\0';
                }

                std::istringstream stream(buf);
                stream.setf(std::ios::hex, std::ios::basefield);
                unsigned value;
                stream >> value;

                elem += static_cast<char> (value & 0xFFu);
            }
            break;
            }

            // Escape is done. Go back to the normal double quote state.
            state = STATE_QUOTE_DOUBLE;
        break;
        }
    }

    switch (state) {
    case STATE_NORMAL:
        result.push_back(elem);
    break;

    case STATE_QUOTE_DOUBLE:
        kdDebug() << "unterminated double quotes" << endl;
    break;

    case STATE_QUOTE_SINGLE:
        kdDebug() << "unterminated single quotes" << endl;
    break;

    case STATE_BACKSLASH:
    case STATE_BACKSLASH_DOUBLEQUOTE:
        kdDebug() << "an escape character must follow a backslash" << endl;
    break;

    default:
    break;
    }

    return result;
}

PalmLib::pi_uint32_t
StrOps::get_current_time(void)
{
    time_t now;

    time(&now);
    return static_cast<PalmLib::pi_uint32_t> (now) + PalmLib::pi_uint32_t(2082844800);
}

char *
StrOps::strptime(const char *s, const char  *format,  struct tm *tm)
{
    char *data = (char *)s;
    char option = false;

    while (*format != 0) {
        if (*data == 0)
            return NULL;
        switch (*format) {
        case '%':
            option = true;
            format++;
        break;
        case 'd':
            if (option) {
                tm->tm_mday = strtol(data, &data, 10);
                if (tm->tm_mday < 1 || tm->tm_mday > 31)
                    return NULL;
            } else if (*data != 'd') {
                return  data;
            }
            option = false;
            format++;
        break;
        case 'm':
            if (option) {
                /* tm_mon between 0 and 11 */
                tm->tm_mon = strtol(data, &data, 10) - 1;
                if (tm->tm_mon < 0 || tm->tm_mon > 11)
                    return NULL;
            } else if (*data != 'm') {
                return  data;
            }
            option = false;
            format++;
        break;
        case 'y':
            if (option) {
                tm->tm_year = strtol(data, &data, 10);
                if (tm->tm_year < 60) tm->tm_year += 100;
            } else if (*data != 'y') {
                return  data;
            }
            option = false;
            format++;
        break;
        case 'Y':
            if (option) {
                tm->tm_year = strtol(data, &data, 10) - 1900;
            } else if (*data != 'Y') {
                return  data;
            }
            option = false;
            format++;
        break;
        case 'H':
            if (option) {
                tm->tm_hour = strtol(data, &data, 10);
                if (tm->tm_hour < 0 || tm->tm_hour > 23)
                    return NULL;
            } else if (*data != 'H') {
                return  data;
            }
            option = false;
            format++;
        break;
        case 'M':
            if (option) {
                tm->tm_min = strtol(data, &data, 10);
                if (tm->tm_min < 0 || tm->tm_min > 59)
                    return NULL;
            } else if (*data != 'M') {
                return  data;
            }
            option = false;
            format++;
        break;
        default:
            if (option)
                return data;
            if (*data != *format)
                return data;
            format++;
            data++;
        }
    }
    return data;
}

// Read a line from an istream w/o concern for buffer limitations.
std::string
StrOps::readline(std::istream& stream)
{
    std::string line;
    char buf[1024];

    while (1) {
        // Read the next line (or part thereof) from the stream.
        stream.getline(buf, sizeof(buf));
        // Bail out of the loop if the stream has reached end-of-file.
        if ((stream.eof() && !buf[0]) || stream.bad())
            break;

        // Append the data read to the result string.
        line.append(buf);

        // If the stream is good, then stop. Otherwise, clear the
        // error indicator so that getline will work again.
        if ((stream.eof() && buf[0]) || stream.good())
            break;
        stream.clear();
    }

    return line;
}

std::string
StrOps::quote_string(std::string str, bool extended_mode)
{
    std::string result;
    std::ostringstream error;

    if (extended_mode) {
        result += '"';
        for (std::string::iterator c = str.begin(); c != str.end(); ++c) {
            switch (*c) {
            case '\\':
                result += '\\';
                result += '\\';
            break;

            case '\r':
                result += '\\';
                result += 'r';
            break;

            case '\n':
                result += '\\';
                result += 'n';
            break;

            case '\t':
                result += '\\';
                result += 't';
            break;

            case '\v':
                result += '\\';
                result += 'v';
            break;

            case '"':
                result += '\\';
                result += '"';
            break;

            default:
                if (isprint(*c)) {
                    result += *c;
                } else {
                    std::ostringstream buf;
                    buf.width(2);
                    buf.setf(std::ios::hex, std::ios::basefield);
                    buf.setf(std::ios::left);
                    buf << ((static_cast<unsigned> (*c)) & 0xFF) << std::ends;

                    result += "\\x";
                    result += buf.str();
                }
            break;
            }
        }
        result += '"';
    } else {
        result += '"';
        for (std::string::iterator c = str.begin(); c != str.end(); ++c) {
            if (*c == '"') {
                result += "\"\"";
            } else if (*c == '\n' || *c == '\r') {
                error << "use extended csv mode for newlines\n";
                *err << error.str();
               kdDebug() << error.str().c_str() << endl;
            } else {
                result += *c;
            }
        }
        result += '"';
    }

    return result;
}

std::string
StrOps::concatenatepath(std::string p_Path,
        std::string p_FileName, std::string p_Ext)
{
    std::string l_FilePath;
#ifdef WIN32
    if (p_FileName[1] == ':' || p_FileName[0] == '\\')
        return p_FileName;
    else if (p_Path.empty())
      l_FilePath = p_FileName;
    else
        l_FilePath = p_Path + std::string("\\") + p_FileName;
#else
    if (p_FileName[0] == '/')
        return p_FileName;
    else if (p_Path.empty())
      l_FilePath = p_FileName;
    else
        l_FilePath = p_Path + std::string("/") + p_FileName;

#endif
    if (!p_Ext.empty() && (p_FileName.rfind(p_Ext) == std::string::npos))
        l_FilePath += p_Ext;

    return l_FilePath;
}
