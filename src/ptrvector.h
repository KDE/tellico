/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_PTRVECTOR_H
#define TELLICO_PTRVECTOR_H

#include "tellico_debug.h"

#include <ksharedptr.h>

#include <qvaluevector.h>

namespace Tellico {

template <class T> class Vector;
template <class T> class VectorIterator;
template <class T> class VectorConstIterator;
template <class T> bool operator==(const VectorIterator<T>& left, const VectorIterator<T>& right);
template <class T> bool operator!=(const VectorIterator<T>& left, const VectorIterator<T>& right);
template <class T> bool operator==(const VectorConstIterator<T>& left, const VectorConstIterator<T>& right);
template <class T> bool operator!=(const VectorConstIterator<T>& left, const VectorConstIterator<T>& right);

template <class T>
class VectorIterator {
public:
  VectorIterator() : m_vector(0), m_index(0) {}
  VectorIterator(Vector<T>* vector, size_t index) : m_vector(vector), m_index(index) {}
  VectorIterator(const VectorIterator<T>& other) : m_vector(other.m_vector), m_index(other.m_index) {}

  operator T*() { return m_vector->at(m_index).data(); }
  T* operator->() { return m_vector->at(m_index).data(); }
  T& operator*() { return *m_vector->at(m_index); }

  VectorIterator& operator++() { ++m_index; return *this; }
  VectorIterator& operator--() { --m_index; return *this; }

  friend bool operator==(const VectorIterator<T>& left, const VectorIterator<T>& right)
    { return left.m_vector == right.m_vector && left.m_index == right.m_index; }
  friend bool operator!=(const VectorIterator<T>& left, const VectorIterator<T>& right)
    { return left.m_vector != right.m_vector || left.m_index != right.m_index; }

  bool nextEnd() const { return m_index == m_vector->count()-1; }

private:
  friend class Vector<T>;
  Vector<T>* m_vector;
  size_t m_index;
};

template <class T>
class VectorConstIterator {
public:
  VectorConstIterator() : m_vector(0), m_index(0) {}
  VectorConstIterator(const Vector<T>* vector, size_t index) : m_vector(vector), m_index(index) {}
  VectorConstIterator(const VectorIterator<T>& other) : m_vector(other.m_vector), m_index(other.m_index) {}

  operator const T*() { return m_vector->at(m_index).data(); }
  const T* operator->() { return m_vector->at(m_index).data(); }
  const T& operator*() { return *m_vector->at(m_index); }

  VectorConstIterator& operator++() { ++m_index; return *this; }
  VectorConstIterator& operator--() { --m_index; return *this; }

  friend bool operator==(const VectorConstIterator<T>& left, const VectorConstIterator<T>& right)
    { return left.m_vector == right.m_vector && left.m_index == right.m_index; }
  friend bool operator!=(const VectorConstIterator<T>& left, const VectorConstIterator<T>& right)
    { return left.m_vector != right.m_vector || left.m_index != right.m_index; }

  bool nextEnd() const { return m_index == m_vector->count()-1; }

private:
  friend class Vector<T>;
  const Vector<T>* m_vector;
  size_t m_index;
};

template <class T>
class Vector {
public:
  typedef KSharedPtr<T> Ptr;
  typedef VectorIterator<T> Iterator;
  typedef VectorConstIterator<T> ConstIterator;

  Vector() {}
  Vector(const Vector<T>& v) : m_baseVector(v.m_baseVector) {}
  Vector& operator=(const Vector<T>& other) {
    if(this != &other) {
      m_baseVector = other.m_baseVector;
    }
    return *this;
  }

  bool isEmpty() const { return m_baseVector.empty(); }
  size_t count() const { return m_baseVector.size(); }

  Ptr& operator[](size_t i) { return m_baseVector[i]; }
  const Ptr& operator[](size_t i) const { return m_baseVector[i]; }

  Ptr& at(size_t i, bool* ok = 0) { return m_baseVector.at(i, ok); }
  const Ptr& at(size_t i, bool* ok = 0) const { return m_baseVector.at(i, ok); }

  Iterator begin() { return Iterator(this, 0); }
  ConstIterator begin() const { return ConstIterator(this, 0); }
  ConstIterator constBegin() const { return ConstIterator(this, 0); }
  Iterator end() { return Iterator(this, count()); }
  ConstIterator end() const { return ConstIterator(this, count()); }
  ConstIterator constEnd() const { return ConstIterator(this, count()); }

  void clear() { m_baseVector.clear(); }
  void append(T* t) { m_baseVector.append(t); }

  void insert(Iterator pos, T* t) {
    m_baseVector.insert(&m_baseVector[pos.m_index], t);
  }

  Iterator find(T* t) {
    for(size_t i = 0; i < count(); ++i) {
      if(m_baseVector[i].data() == t) {
        return Iterator(this, i);
      }
    }
    return end();
  }

  bool contains(T* t) const { return qFind(m_baseVector.begin(), m_baseVector.end(), Ptr(t)) != m_baseVector.end(); }
  bool remove(T* t) { return remove(Ptr(t)); }
  bool remove(const Ptr& t) {
    Ptr* it = qFind(m_baseVector.begin(), m_baseVector.end(), t);
    if(it == m_baseVector.end()) return false;
    m_baseVector.erase(it);
    return true;
  }

private:
  QValueVector<Ptr> m_baseVector;
};

template <class T> class PtrVector;
template <class T> class PtrVectorIterator;
template <class T> class PtrVectorConstIterator;
template <class T> bool operator==(const PtrVectorIterator<T>& left, const PtrVectorIterator<T>& right);
template <class T> bool operator!=(const PtrVectorIterator<T>& left, const PtrVectorIterator<T>& right);
template <class T> bool operator==(const PtrVectorConstIterator<T>& left, const PtrVectorConstIterator<T>& right);
template <class T> bool operator!=(const PtrVectorConstIterator<T>& left, const PtrVectorConstIterator<T>& right);

template <class T>
class PtrVectorIterator {
public:
  PtrVectorIterator() : m_vector(0), m_index(0) {}
  PtrVectorIterator(const PtrVector<T>* vector, size_t index) : m_vector(vector), m_index(index) {}
  PtrVectorIterator(const PtrVectorIterator<T>& other) : m_vector(other.m_vector), m_index(other.m_index) {}

  T* operator->() { return &m_vector->at(m_index); }
  T& operator*() { return m_vector->at(m_index); }

  T* ptr() { return &m_vector->at(m_index); }

  PtrVectorIterator& operator++() { ++m_index; return *this; }
  PtrVectorIterator& operator--() { --m_index; return *this; }

  friend bool operator==(const PtrVectorIterator<T>& left, const PtrVectorIterator<T>& right)
    { return left.m_vector == right.m_vector && left.m_index == right.m_index; }
  friend bool operator!=(const PtrVectorIterator<T>& left, const PtrVectorIterator<T>& right)
    { return left.m_vector != right.m_vector || left.m_index != right.m_index; }

private:
  const PtrVector<T>* m_vector;
  size_t m_index;
};

template <class T>
class PtrVectorConstIterator {
public:
  PtrVectorConstIterator() : m_vector(0), m_index(0) {}
  PtrVectorConstIterator(const PtrVector<T>* vector, size_t index) : m_vector(vector), m_index(index) {}
  PtrVectorConstIterator(const PtrVectorConstIterator<T>& other) : m_vector(other.m_vector), m_index(other.m_index) {}

  const T* operator->() const { return &m_vector->at(m_index); }
  const T& operator*() const { return m_vector->at(m_index); }

  const T* ptr() const { return &m_vector->at(m_index); }

  PtrVectorConstIterator& operator++() { ++m_index; return *this; }
  PtrVectorConstIterator& operator--() { --m_index; return *this; }

  friend bool operator==(const PtrVectorConstIterator<T>& left, const PtrVectorConstIterator<T>& right)
    { return left.m_vector == right.m_vector && left.m_index == right.m_index; }
  friend bool operator!=(const PtrVectorConstIterator<T>& left, const PtrVectorConstIterator<T>& right)
    { return left.m_vector != right.m_vector || left.m_index != right.m_index; }

private:
  const PtrVector<T>* m_vector;
  size_t m_index;
};

/**
 * @author Robby Stephenson
 */
template <class T>
class PtrVector {

public:
  typedef Tellico::PtrVectorIterator<T> Iterator;
  typedef Tellico::PtrVectorConstIterator<T> ConstIterator;

  PtrVector() : m_autoDelete(false) {}
  PtrVector(const PtrVector<T>& other) : m_baseVector(other.m_baseVector), m_autoDelete(false) {}
  PtrVector& operator=(const PtrVector<T>& other) {
    if(this != &other) {
      m_baseVector = other.m_baseVector;
      m_autoDelete = false;
    }
    return *this;
  }
  ~PtrVector() { if(m_autoDelete) clear(); }

  size_t count() const { return m_baseVector.size(); }
  bool isEmpty() const { return m_baseVector.empty(); }
  bool autoDelete() const { return m_autoDelete; }
  void setAutoDelete(bool b) { m_autoDelete = b; }

  T& operator[](size_t n) const { check(n); return *m_baseVector[n]; }
  T& at(size_t n) const { check(n); return *m_baseVector.at(n); }

  Iterator begin() { return Iterator(this, 0); }
  ConstIterator begin() const { return ConstIterator(this, 0); }
  ConstIterator constBegin() const { return ConstIterator(this, 0); }
  Iterator end() { return Iterator(this, count()); }
  ConstIterator end() const { return ConstIterator(this, count()); }
  ConstIterator constEnd() const { return ConstIterator(this, count()); }

  T* front() { return count() > 0 ? m_baseVector.at(0) : 0; }
  const T* front() const { return count() > 0 ? m_baseVector.at(0) : 0; }

  void clear() { while(remove(begin())) { ; } }

  void push_back(T* ptr) { m_baseVector.push_back(ptr); }
  bool remove(T* ptr);
  bool remove(Iterator it);
  bool contains(const T* ptr) const;

private:
#ifndef NDEBUG
  void check(size_t n) const { if(n >= count()) kdDebug() << "PtrVector() - bad index" << endl; }
#else
  void check(size_t) const {}
#endif

  QValueVector<T*> m_baseVector;
  bool m_autoDelete : 1;
};

}

template <class T>
bool Tellico::PtrVector<T>::remove(T* t) {
  if(!t) {
    return false;
  }
  T** ptr = qFind(m_baseVector.begin(), m_baseVector.end(), t);
  if(ptr == m_baseVector.end()) {
    return false;
  }
  m_baseVector.erase(ptr);
  if(m_autoDelete) {
    delete *ptr;
  }
  return true;
}

template <class T>
bool Tellico::PtrVector<T>::remove(Iterator it) {
  if(it == end()) {
    return false;
  }
  T** ptr = qFind(m_baseVector.begin(), m_baseVector.end(), it.ptr());
  if(ptr == m_baseVector.end()) {
    return false;
  }
  m_baseVector.erase(ptr);
  if(m_autoDelete) {
    delete *ptr;
  }
  return true;
}

template <class T>
bool Tellico::PtrVector<T>::contains(const T* t) const {
  if(!t) {
    return false;
  }
  const T* const* ptr = qFind(m_baseVector.begin(), m_baseVector.end(), t);
  return ptr != m_baseVector.end();
}

#endif
