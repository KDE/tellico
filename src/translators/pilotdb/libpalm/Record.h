/*
 * palm-db-tools: Raw PalmOS Records
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 */

#ifndef __PALMLIB_RECORD_H__
#define __PALMLIB_RECORD_H__

#include "Block.h"

namespace PalmLib {

    class Record : public Block {
    public:
#ifdef __GNUG__
  static const pi_char_t FLAG_ATTR_DELETED = 0x80;
  static const pi_char_t FLAG_ATTR_DIRTY   = 0x40;
  static const pi_char_t FLAG_ATTR_BUSY    = 0x20;
  static const pi_char_t FLAG_ATTR_SECRET  = 0x10;
#else
  static const pi_char_t FLAG_ATTR_DELETED;
  static const pi_char_t FLAG_ATTR_DIRTY;
  static const pi_char_t FLAG_ATTR_BUSY;
  static const pi_char_t FLAG_ATTR_SECRET;
#endif

  /**
   * Default constructor.
   */
  Record() : Block(), m_attrs(0), m_unique_id(0) { }

  /**
   * Copy constructor.
   */
  Record(const Record& rhs) : Block(rhs.data(), rhs.size()) {
      m_attrs = rhs.attrs();
      m_unique_id = rhs.unique_id();
  }

  /**
   * Destructor.
   */
  virtual ~Record() { }

  /**
   * Constructor which lets the caller specify all the
   * parameters.
   *
   * @param attrs     Attribute byte (flags + category).
   * @param unique_id Unique ID for this record.
   * @param data      Start of buffer to copy (or 0 for empty).
   * @param size      Size of the buffer to copy.
   */
  Record(pi_char_t attrs, pi_uint32_t unique_id,
         Block::const_pointer data, const Block::size_type size)
      : Block(data, size), m_attrs(attrs), m_unique_id(unique_id) { }

  /**
   * Constructor which lets the caller use the default fill
   * constructor.
   * @param attrs     Attribute byte (flags + category).
   * @param unique_id Unique ID for this record.
   * @param size      Size of buffer to generate.
   * @param value     Value to fill buffer with.
   */
  Record(pi_char_t attrs, pi_uint32_t unique_id,
         const size_type size, const value_type value = 0)
      : Block(size, value), m_attrs(attrs), m_unique_id(unique_id) { }

  /**
   * Assignment operator.
   *
   * @param rhs The PalmLib::Record we should become.  */
  Record& operator = (const Record& rhs) {
      Block::operator = (rhs);
      m_attrs = rhs.attrs();
      m_unique_id = rhs.unique_id();
      return *this;
  }

  /**
   * Return the attributes byte which contains the category and
   * flags.
   */
  pi_char_t attrs() const { return m_attrs; }

  /**
   * Return the state of the record's "deleted" flag.
   */
  bool deleted() const { return (m_attrs & FLAG_ATTR_DELETED) != 0; }

  /**
   * Set the state of the record's "deleted" flag.
   *
   * @param state New state of the "deleted" flag.
   */
  void deleted(bool state) {
      if (state)
    m_attrs |= FLAG_ATTR_DELETED;
      else
    m_attrs &= ~(FLAG_ATTR_DELETED);
  }

  /**
   * Return the state of the record's "dirty" flag.
   */
  bool dirty() const { return (m_attrs & FLAG_ATTR_DIRTY) != 0; }

  /**
   * Set the state of the record's "dirty" flag.
   *
   * @param state New state of the "dirty" flag.
   */
  void dirty(bool state) {
      if (state)
    m_attrs |= FLAG_ATTR_DIRTY;
      else
    m_attrs &= ~(FLAG_ATTR_DIRTY);
  }

  /**
   * Return the state of the record's "secret" flag.
   */
  bool secret() const { return (m_attrs & FLAG_ATTR_SECRET) != 0; }

  /**
   * Set the state of the record's "secret" flag.
   *
   * @param state New state of the "secret" flag.
   */
  void secret(bool state) {
      if (state)
    m_attrs |= FLAG_ATTR_SECRET;
      else
    m_attrs &= ~(FLAG_ATTR_SECRET);
  }

  /**
   * Return the category of this record.
   */
  pi_char_t category() const { return (m_attrs & 0x0F); }

  /**
   * Set the category of this record.
   */
  void category(pi_char_t cat)
      { m_attrs &= ~(0x0F); m_attrs |= (cat & 0x0F); }

  /**
   * Return the unique ID of this record.
   */
  pi_uint32_t unique_id() const { return m_unique_id; }

  /**
   * Set the unique ID of this record to uid.
   *
   * @param uid New unique ID for this record.
   */
  void unique_id(pi_uint32_t uid) { m_unique_id = uid; }

    private:
  pi_char_t m_attrs;
  pi_uint32_t m_unique_id;
    };

}

#endif
