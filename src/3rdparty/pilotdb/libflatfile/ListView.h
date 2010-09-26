#ifndef FLATFILE_LISTVIEW_H__
#define FLATFILE_LISTVIEW_H__

#include <string>
#include <vector>

#include "ListViewColumn.h"

namespace PalmLib {
    namespace FlatFile {

  // The ListView class represents the a "list view" as
  // implemented by the major PalmOS flat-file programs. The
  // main idea is a series of columns that display a field of
  // the database.
  //
  // For fun, this class exports the STL interface of the STL
  // class it uses to store the ListViewColumn classes.

  class ListView {
  private:
      typedef std::vector<ListViewColumn> rep_type;
      rep_type rep;

  public:
      typedef rep_type::value_type value_type;
      typedef rep_type::iterator iterator;
      typedef rep_type::const_iterator const_iterator;
      typedef rep_type::reference reference;
      typedef rep_type::const_reference const_reference;
      typedef rep_type::size_type size_type;
      typedef rep_type::difference_type difference_type;
      typedef rep_type::reverse_iterator reverse_iterator;
      typedef rep_type::const_reverse_iterator const_reverse_iterator;

      // global fields
      std::string name;
      bool editoruse;

      // STL pull-up interface (probably not complete)
      iterator begin() { return rep.begin(); }
      const_iterator begin() const { return rep.begin(); }
      iterator end() { return rep.end(); }
      const_iterator end() const { return rep.end(); }
      reverse_iterator rbegin() { return rep.rbegin(); }
      const_reverse_iterator rbegin() const { return rep.rbegin(); }
      reverse_iterator rend() { return rep.rend(); }
      const_reverse_iterator rend() const { return rep.rend(); }
      size_type size() const { return rep.size(); }
      size_type max_size() const { return rep.max_size(); }
      bool empty() const { return rep.empty(); }
      reference front() { return rep.front(); }
      const_reference front() const { return rep.front(); }
      reference back() { return rep.back(); }
      const_reference back() const { return rep.back(); }
      void push_back(const ListViewColumn& x) { rep.push_back(x); }
      void pop_back() { rep.pop_back(); }
      void clear() { rep.clear(); }
      void resize(size_type new_size, const ListViewColumn& x)
        { rep.resize(new_size, x); }
      void resize(size_type new_size)
        { rep.resize(new_size, ListViewColumn()); }

      ListView() : rep(), name(""), editoruse(false) { }
      ListView(const ListView& rhs) : rep(rhs.rep), name(rhs.name), editoruse(false) { }
      ListView& operator = (const ListView& rhs) {
        name = rhs.name;
        rep = rhs.rep;
        return *this;
      }

    };

  }
}

#endif
