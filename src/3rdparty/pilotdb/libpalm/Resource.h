/*
 * palm-db-tools: PalmOS Resources
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 */

#ifndef __PALMLIB_RESOURCE_H__
#define __PALMLIB_RESOURCE_H__

#include "Block.h"
#include "palmtypes.h"

namespace PalmLib {

    class Resource : public Block {
    public:
  /**
   * Default constructor.
   */
  Resource() : Block(), m_type(0), m_id(0) { }

  /**
   * Copy constructor.
   */
  Resource(const Resource& rhs) : Block(rhs.data(), rhs.size()) {
      m_type = rhs.type();
      m_id = rhs.id();
  }

  /**
   * Destructor.
   */
  virtual ~Resource() { }

  /**
         * Constructor which lets the caller specify all the
         * parameters.
         *
         * @param type      Resource type
         * @param id        Resource ID
         * @param data      Start of buffer to copy.
         * @param size      Size of the buffer to copy.
         */
  Resource(pi_uint32_t type, pi_uint32_t id,
     const_pointer data, const size_type size)
      : Block(data, size), m_type(type), m_id(id) { }

  /**
         * Constructor which lets the caller use the default fill
         * constructor.
         *
         * @param type      Resource type
         * @param id        Resource ID
   * @param size      Size of buffer to generate.
         * @param value     Value to fill buffer with.
   */
  Resource(pi_uint32_t type, pi_uint32_t id,
     const size_type size, const value_type value = 0)
      : Block(size, value), m_type(type), m_id(id) { }

  /**
   * Assignment operator.
   */
  Resource& operator = (const Resource& rhs) {
      Block::operator = (rhs);
      m_type = rhs.type();
      m_id = rhs.id();
      return *this;
  }

  // Accessor functions for the resource type.
  pi_uint32_t type() const { return m_type; }
  void type(const pi_uint32_t _type) { m_type = _type; }

  // Accessor functions for the resource ID.
  pi_uint32_t id() const { return m_id; }
  void id(const pi_uint32_t _id) { m_id = _id; }

    private:
  pi_uint32_t m_type;
  pi_uint32_t m_id;
    };

}

#endif
