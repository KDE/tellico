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
  HTMLExporter(Data::CollPtr coll, const QUrl& baseUrl);
  ~HTMLExporter();

  virtual bool exec() override;
  virtual void reset() override;
  virtual QString formatString() const override;
  virtual QString fileFilter() const override;

  virtual QWidget* widget(QWidget* parent) override;
  virtual void readOptions(KSharedConfigPtr) override;
  virtual void saveOptions(KSharedConfigPtr) override;

  void setCollectionURL(const QUrl& url);
  void setXSLTFile(const QString& filename);
  void setEntryXSLTFile(const QString& filename);
  void setPrintHeaders(bool printHeaders);
  void setPrintGrouped(bool printGrouped);
  void setMaxImageSize(int w, int h);
  void setGroupBy(const QStringList& groupBy);
  void setSortTitles(const QStringList& l);
  void setColumns(const QStringList& columns);
  void setParseDOM(bool parseDOM);
  void setExportEntryFiles(bool exportEntryFiles);
  void setCustomHtml(const QString& html_);

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
  QString handleLink(const xmlChar* link);
  QString handleLink(const QString& link);
  QByteArray analyzeInternalCSS(const xmlChar* string);
  bool copyFiles();
  bool loadXSLTFile();
  void createDir();

  XSLTHandler* m_handler;
  bool m_printHeaders;
  bool m_printGrouped;
  bool m_exportEntryFiles;
  bool m_cancelled;
  bool m_parseDOM;
  bool m_checkCreateDir;
  bool m_checkCommonFile;
  int m_imageWidth;
  int m_imageHeight;

  QWidget* m_widget;
  QCheckBox* m_checkPrintHeaders;
  QCheckBox* m_checkPrintGrouped;
  QCheckBox* m_checkExportEntryFiles;
  QCheckBox* m_checkExportImages;

  QUrl m_baseUrl;
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
