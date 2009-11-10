/*
 * palm-db-tools: Encapsulate "blocks" of data.
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 *
 * The PalmLib::Block class represents a generic block of data. It is
 * used to make passing pi_char_t buffers around very easy. The Record
 * and Resource classes both inherit from this class. A STL interface
 * is also attempted though it is probably not complete.
 */

#ifndef __PALMLIB_BLOCK_H__
#define __PALMLIB_BLOCK_H__

#include <algorithm>
#include <iterator>

#include "palmtypes.h"

namespace PalmLib {

    class Block {
    public:
      // STL: container type definitions
     typedef PalmLib::pi_char_t value_type;
     typedef value_type* pointer;
     typedef const value_type* const_pointer;
     typedef value_type* iterator;
     typedef const value_type* const_iterator;
     typedef value_type& reference;
     typedef const value_type& const_reference;
     typedef size_t size_type;
     typedef ptrdiff_t difference_type;

     // STL: reverisible container type definitions
#ifdef __GNUG__
     typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
     typedef std::reverse_iterator<iterator> reverse_iterator;
#endif

  /**
   * Default constructor.
   */
  Block() : m_data(0), m_size(0) { }

  /**
   * Constructor which fills the block from buffer "raw" with
   * length "len".
   */
  Block(const_pointer raw, const size_type len) : m_data(0), m_size(0) {
      assign(raw, len);
  }

  /**
   * Constructor which takes a size and allocates a zero'ed out
   * buffer of that size. (STL: Sequence: default fill
   * constructor)
   */
  explicit Block(const size_type size, const value_type value = 0)
      : m_data(0), m_size(0) {
      assign(size, value);
  }

  /**
   * Constructor which takes two iterators and builds the block
   * from the region between the iterators. (STL: Sequence:
   * range constructor)
   */
  Block(const_iterator a, const_iterator b) : m_data(0), m_size(0) {
      assign(a, b - a);
  }

  /**
   * Copy constructor. Just copies the data from the other block
   * into this block.
   */
  Block(const Block& rhs) : m_data(0), m_size(0) {
      assign(rhs.data(), rhs.size());
  }

  /**
   * Destructor. Just frees the buffer if it exists.
   */
  virtual ~Block() { clear(); }

  /**
   * Assignment operator.
   *
   * @param rhs The block whose contents should be copied.
   */
  Block& operator = (const Block& rhs) {
      assign(rhs.data(), rhs.size());
      return *this;
  }

  // STL: Container
  iterator begin() { return m_data; }
  const_iterator begin() const { return m_data; }
  iterator end() { return (m_data != 0) ? (m_data + m_size) : (0); }
  const_iterator end() const
      { return (m_data != 0) ? (m_data + m_size) : (0); }
  size_type size() const { return m_size; }
  size_type max_size() const {
      return size_type(-1) / sizeof(value_type);
  }
  bool empty() const { return m_size == 0; }

  // STL: Reversible Container
#ifdef __GNUG__
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
      return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
      return const_reverse_iterator(begin());
  }
#endif

  // STL: Random Access Container
  reference operator [] (size_type index) { return m_data[index]; }
  const_reference operator [] (size_type index) const
      { return m_data[index]; }

  // STL: Sequence (not complete)
  void clear() {
      if (m_data) {
    delete [] m_data;
    m_data = 0;
    m_size = 0;
      }
  }
  void resize(size_type n);
  reference front() { return m_data[0]; }
  const_reference front() const { return m_data[0]; }

  // STL: (present in vector but not part of a interface spec)
  size_type capacity() const { return m_size; }
  void reserve(size_type size);

  /**
   * Return a pointer to the data area. If there are no
   * contents, then the return value will be NULL. This is not
   * an STL method but goes with this class as a singular data
   * block and not a container (even though it is).
   */
  iterator data() { return m_data; }
  const_iterator data() const { return m_data; }

  /**
   * Replace the existing contents of the Block with the buffer
   * that starts at raw of size len.
   *
   * @param raw Pointer to the new contents.
   * @param len Size of the new contents.
   */
  void assign(const_pointer data, const size_type size);

  /**
   * Replace the existing contents of the Block with a buffer
   * consisting of size elements equal to fill.
   *
   * @param size The size of the new contents.
   * @param value Value to fill the contents with.
   */
     void assign(const size_type size, const value_type value = 0);

     // compatiblity functions (remove before final 0.3.0 release)
     const_pointer raw_data() const { return data(); }
     pointer raw_data() { return data(); }
     size_type raw_size() const { return size(); }
     void set_raw(const_pointer raw, const size_type len)
         { assign(raw, len); }

    private:
     pointer m_data;
     size_type m_size;
    };

}

bool operator == (const PalmLib::Block& lhs, const PalmLib::Block& rhs);

inline bool operator != (const PalmLib::Block& lhs, const PalmLib::Block& rhs)
{ return ! (lhs == rhs); }

#endif
