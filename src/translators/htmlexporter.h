/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef HTMLEXPORTER_H
#define HTMLEXPORTER_H

class QCheckBox;

#include "textexporter.h"

#include <qstringlist.h>

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: htmlexporter.h 862 2004-09-15 01:49:51Z robby $
 */
class HTMLExporter : public TextExporter {
public:
  HTMLExporter(const Data::Collection* coll);

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*);
  virtual void saveOptions(KConfig*);

  void setXSLTFile(const QString& filename) { m_xsltfile = filename; }
  void setPrintHeaders(bool printHeaders) { m_printHeaders = printHeaders; }
  void setPrintGrouped(bool printGrouped) { m_printGrouped = printGrouped; }
  void setMaxImageSize(int w, int h) { m_imageWidth = w; m_imageHeight = h; }
  void setGroupBy(const QStringList& groupBy) { m_groupBy = groupBy; }
  void setSortTitles(const QStringList& l)
    { m_sort1 = l[0]; m_sort2 = l[1]; m_sort3 = l[2]; }
  void setColumns(const QStringList& columns) { m_columns = columns; }

private:
  bool m_printHeaders;
  bool m_printGrouped;
  bool m_exportEntryFiles;
  int m_imageWidth;
  int m_imageHeight;

  QWidget* m_widget;
  QCheckBox* m_checkPrintHeaders;
  QCheckBox* m_checkPrintGrouped;
  QCheckBox* m_checkExportEntryFiles;
  QCheckBox* m_checkExportImages;

  QString m_xsltfile;
  QStringList m_groupBy;
  QString m_sort1;
  QString m_sort2;
  QString m_sort3;
  QStringList m_columns;
  QString m_entryXSLTFile;
};

  } // end namespace
} // end namespace
#endif
