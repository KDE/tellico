#ifndef __PALMLIB_FLATFILE_LISTVIEWCOLUMN_H__
#define __PALMLIB_FLATFILE_LISTVIEWCOLUMN_H__

namespace PalmLib {
  namespace FlatFile {

    // The ListViewColumn class stores the field number and column
    // width for a column in a list view.

    struct ListViewColumn {
      ListViewColumn() : field(0), width(80) { }
      ListViewColumn(unsigned a1, unsigned a2) : field(a1), width(a2) { }
      unsigned field;
      unsigned width;
    };
  }
}

#endif
