/*
 * This file contains type definitions and helper functions to make
 * access to data in Palm Pilot order easier.
 */

#ifndef __LIBPALM_PALMTYPES_H__
#define __LIBPALM_PALMTYPES_H__

#include <stdexcept>

#include "../portability.h"

namespace PalmLib {

#if SIZEOF_UNSIGNED_CHAR == 1
    typedef unsigned char pi_char_t;
#else
#error Unable to determine the size of pi_char_t.
#endif

#if SIZEOF_UNSIGNED_LONG == 2
    typedef unsigned long pi_uint16_t;
#elif SIZEOF_UNSIGNED_INT == 2
    typedef unsigned int pi_uint16_t;
#elif SIZEOF_UNSIGNED_SHORT == 2
    typedef unsigned short pi_uint16_t;
#else
#error Unable to determine the size of pi_uint16_t.
#endif

#if SIZEOF_LONG == 2
    typedef long pi_int16_t;
#elif SIZEOF_INT == 2
    typedef int pi_int16_t;
#elif SIZEOF_SHORT == 2
    typedef short pi_int16_t;
#else
#error Unable to determine the size of pi_int16_t.
#endif

#if SIZEOF_UNSIGNED_LONG == 4
    typedef unsigned long pi_uint32_t;
#elif SIZEOF_UNSIGNED_INT == 4
    typedef unsigned int pi_uint32_t;
#elif SIZEOF_UNSIGNED_SHORT == 4
    typedef unsigned short pi_uint32_t;
#else
#error Unable to determine the size of pi_uint32_t.
#endif

#if SIZEOF_LONG == 4
    typedef long pi_int32_t;
#elif SIZEOF_INT == 4
    typedef int pi_int32_t;
#elif SIZEOF_SHORT == 4
    typedef short pi_int32_t;
#else
#error Unable to determine the size of pi_int32_t.
#endif

typedef union {
    double number;
#ifdef WORDS_BIGENDIAN
    struct {
        PalmLib::pi_uint32_t hi;
        PalmLib::pi_uint32_t lo;
    } words;
#else
    struct {
        PalmLib::pi_uint32_t lo;
        PalmLib::pi_uint32_t hi;
    } words;
#endif
} pi_double_t;

    inline pi_int32_t get_long(const pi_char_t* p) {
      return ( (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3] );
    }

    inline pi_int32_t get_treble(const pi_char_t* p) {
      return ( (p[0] << 16) || (p[1] << 8) || p[0]);
    }

    inline pi_int16_t get_short(const pi_char_t* p) {
      return ( (p[0] << 8) | p[1] );
    }

    inline void set_long(pi_char_t *p, pi_int32_t v) {
      p[0] = (v >> 24) & 0xFF;
      p[1] = (v >> 16) & 0xFF;
      p[2] = (v >> 8 ) & 0xFF;
      p[3] = (v      ) & 0xFF;
    }

    inline void set_treble(pi_char_t *p, pi_int32_t v) {
      p[0] = (v >> 16) & 0xFF;
      p[1] = (v >> 8 ) & 0xFF;
      p[2] = (v      ) & 0xFF;
    }

    inline void set_short(pi_char_t *p, pi_int16_t v) {
      p[0] = (v >> 8) & 0xFF;
      p[1] = (v     ) & 0xFF;
    }

    inline pi_uint32_t mktag(pi_char_t c1, pi_char_t c2,
           pi_char_t c3, pi_char_t c4)
    { return (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4)); }

    class error : public std::runtime_error {
    public:
      error(const std::string & what_arg) : std::runtime_error(what_arg) { }
    };

} // namespace PalmLib

#endif
