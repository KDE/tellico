/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_HTMLEXPORTER_H
#define TELLICO_HTMLEXPORTER_H

#include "exporter.h"
#include "../utils/stringset.h"

#include <QStringList>
#include <QHash>

#include <libxml/xmlstring.h>

class QCheckBox;

extern "C" {
  struct _xmlNode;
}

class HtmlExporterTest;

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

friend class ::HtmlExporterTest;

public:
  HTMLExporter(Data::CollPtr coll);
  ~HTMLExporter();

  virtual bool exec() override;
  virtual void reset() override;
  virtual QString formatString() const override;
  virtual QString fileFilter() const override;

  virtual QWidget* widget(QWidget* parent) override;
  virtual void readOptions(KSharedConfigPtr) override;
  virtual void saveOptions(KSharedConfigPtr) override;

  void setCollectionURL(const QUrl& url) { m_collectionURL = url; m_links.clear(); }
  void setXSLTFile(const QString& filename);
  void setEntryXSLTFile(const QString& filename);
  void setPrintHeaders(bool printHeaders) { m_printHeaders = printHeaders; }
  void setPrintGrouped(bool printGrouped) { m_printGrouped = printGrouped; }
  void setMaxImageSize(int w, int h) { m_imageWidth = w; m_imageHeight = h; }
  void setGroupBy(const QStringList& groupBy) { m_groupBy = groupBy; }
  void setSortTitles(const QStringList& l)
    { m_sort1 = l[0]; m_sort2 = l[1]; m_sort3 = l[2]; }
  void setColumns(const QStringList& columns) { m_columns = columns; }
  void setParseDOM(bool parseDOM) { m_parseDOM = parseDOM; reset(); }
  void setExportEntryFiles(bool exportEntryFiles) { m_exportEntryFiles = exportEntryFiles; }
  void setCustomHtml(const QString& html_) { m_customHtml = html_; }

  QString text();

public Q_SLOTS:
  void slotCancel();

private:
  void setFormattingOptions(Data::CollPtr coll);
  void writeImages(Data::CollPtr coll);
  bool writeEntryFiles();
  QUrl fileDir() const;
  QString fileDirName() const;

  void parseDOM(_xmlNode* node);
  QString handleLink(const QString& link);
  const xmlChar* handleLink(const xmlChar* link);
  QString analyzeInternalCSS(const QString& string);
  const xmlChar* analyzeInternalCSS(const xmlChar* string);
  bool copyFiles();
  bool loadXSLTFile();
  void createDir();

  XSLTHandler* m_handler;
  bool m_printHeaders : 1;
  bool m_printGrouped : 1;
  bool m_exportEntryFiles : 1;
  bool m_cancelled : 1;
  bool m_parseDOM : 1;
  bool m_checkCreateDir : 1;
  bool m_checkCommonFile : 1;
  int m_imageWidth;
  int m_imageHeight;

  QWidget* m_widget;
  QCheckBox* m_checkPrintHeaders;
  QCheckBox* m_checkPrintGrouped;
  QCheckBox* m_checkExportEntryFiles;
  QCheckBox* m_checkExportImages;

  QUrl m_collectionURL;
  QString m_xsltFile;
  QString m_xsltFilePath;
  QString m_dataDir;
  QStringList m_groupBy;
  QString m_sort1;
  QString m_sort2;
  QString m_sort3;
  QStringList m_columns;
  QString m_entryXSLTFile;

  QList<QUrl> m_files;
  QHash<QString, QString> m_links;
  StringSet m_copiedFiles;
  QString m_customHtml;
};

  } // end namespace
} // end namespace
#endif
