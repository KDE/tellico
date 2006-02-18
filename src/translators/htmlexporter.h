/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
class KHTMLPart;
namespace DOM {
  class Node;
}

#include "exporter.h"
#include "../stringset.h"

#include <qstringlist.h>

namespace Tellico {
  namespace Data {
    class Collection;
  }
  class XSLTHandler;

  namespace Export {

/**
 * @author Robby Stephenson
 */
class HTMLExporter : public Exporter {
Q_OBJECT

public:
  HTMLExporter();
  HTMLExporter(Data::CollPtr coll);
  ~HTMLExporter();

  virtual bool exec();
  virtual void reset();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  virtual QWidget* widget(QWidget* parent, const char* name=0);
  virtual void readOptions(KConfig*);
  virtual void saveOptions(KConfig*);

  void setCollectionURL(const KURL& url) { m_collectionURL = url; m_links.clear(); }
  void setXSLTFile(const QString& filename);
  void setPrintHeaders(bool printHeaders) { m_printHeaders = printHeaders; }
  void setPrintGrouped(bool printGrouped) { m_printGrouped = printGrouped; }
  void setMaxImageSize(int w, int h) { m_imageWidth = w; m_imageHeight = h; }
  void setGroupBy(const QStringList& groupBy) { m_groupBy = groupBy; }
  void setSortTitles(const QStringList& l)
    { m_sort1 = l[0]; m_sort2 = l[1]; m_sort3 = l[2]; }
  void setColumns(const QStringList& columns) { m_columns = columns; }
  void setParseDOM(bool parseDOM) { m_parseDOM = parseDOM; }

  QString text();

public slots:
  void slotCancel();

private:
  void setFormattingOptions(Data::CollPtr coll);
  void writeImages(Data::CollPtr coll);
  bool writeEntryFiles();
  KURL fileDir() const;
  QString fileDirName() const;

  void parseNode(DOM::Node node);
  QString handleLink(const QString& link);
  QString analyzeInternalCSS(const QString& string);
  bool copyFiles();
  bool loadXSLTFile();
  bool createDir();

  KHTMLPart* m_part;
  XSLTHandler* m_handler;
  bool m_printHeaders : 1;
  bool m_printGrouped : 1;
  bool m_exportEntryFiles : 1;
  bool m_cancelled : 1;
  bool m_parseDOM : 1;
  int m_imageWidth;
  int m_imageHeight;

  QWidget* m_widget;
  QCheckBox* m_checkPrintHeaders;
  QCheckBox* m_checkPrintGrouped;
  QCheckBox* m_checkExportEntryFiles;
  QCheckBox* m_checkExportImages;

  KURL m_collectionURL;
  QString m_xsltFile;
  QString m_xsltFilePath;
  QStringList m_groupBy;
  QString m_sort1;
  QString m_sort2;
  QString m_sort3;
  QStringList m_columns;
  QString m_entryXSLTFile;

  KURL::List m_files;
  QMap<QString, QString> m_links;
  StringSet m_copiedFiles;
};

  } // end namespace
} // end namespace
#endif
