/*
 * palm-db-tools: Encapsulate "blocks" of data.
 * Copyright (C) 2000 by Tom Dyas (tdyas@users.sourceforge.net)
 *
 * The PalmLib::Block class represents a generic block of data. It is
 * used to simplify passing arrays of pi_char_t around.
 */

#include "Block.h"

#include <cstring>

void PalmLib::Block::reserve(PalmLib::Block::size_type new_size)
{
    if (new_size > capacity()) {
        // Allocate a new buffer containing a copy of the old with the
        // remainder zero'ed out.
        pointer new_data = new pi_char_t[new_size];
        memcpy(new_data, m_data, m_size);
        memset(new_data + m_size, 0, new_size - m_size);

        // Replace the existing buffer.
        delete [] m_data;
        m_data = new_data;
        m_size = new_size;
    }
}

void PalmLib::Block::resize(size_type new_size)
{
    if (new_size < m_size) {
        // Copy the data that will remain to a new buffer and switch to it.
        pointer new_data = new pi_char_t[new_size];
        memcpy(new_data, m_data, new_size);

        // Replace the existing buffer.
        delete [] m_data;
        m_data = new_data;
        m_size = new_size;
    } else if (new_size > m_size) {
        // Copy the data that will remain to a new buffer and switch to it.
        pointer new_data = new pi_char_t[new_size];
        memcpy(new_data, m_data, m_size);
        memset(new_data + m_size, 0, new_size - m_size);

        // Replace the existing buffer.
        delete [] m_data;
        m_data = new_data;
        m_size = new_size;
    }
}

void PalmLib::Block::assign(PalmLib::Block::const_pointer data,
                            const PalmLib::Block::size_type size)
{
    clear();
    if (data && size > 0) {
        m_size = size;
        m_data = new pi_char_t[m_size];
        memcpy(m_data, data, m_size);
    }
}

void PalmLib::Block::assign(const PalmLib::Block::size_type size,
                            const PalmLib::Block::value_type value)
{
    clear();
    if (size > 0) {
        m_size = size;
        m_data = new pi_char_t[m_size];
        memset(m_data, value, m_size);
    }
}

bool operator == (const PalmLib::Block& lhs, const PalmLib::Block& rhs)
{
    if (lhs.size() == rhs.size()) {
        if (lhs.data()) {
            if (memcmp(lhs.data(), rhs.data(), lhs.size()) != 0)
                return false;
        }
        return true;
    }
    return false;
}
