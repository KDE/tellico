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

#ifndef TELLICO_EXPORTER_H
#define TELLICO_EXPORTER_H

#include <QObject>

#include "../entry.h"
#include "../datavectors.h"

#include <KSharedConfig>

#include <QUrl>

class KConfig;

class QWidget;
class QString;

namespace Tellico {
  namespace Export {
    enum Options {
      ExportFormatted     = 1 << 0,   // format entries when exported
      ExportUTF8          = 1 << 1,   // valid for some text files, export as utf-8
      ExportImages        = 1 << 2,   // should the images be included?
      ExportForce         = 1 << 3,   // force the export, no confirmation of overwriting
      ExportComplete      = 1 << 4,   // export complete document, including loans, etc.
      ExportProgress      = 1 << 5,   // show progress bar
      ExportClean         = 1 << 6,   // specifically for bibliographies, remove latex commands
      ExportVerifyImages  = 1 << 7,   // don't put in an image link that's not in the cache
      ExportImageSize     = 1 << 8,   // include image size in the generated XML
      ExportAbsoluteLinks = 1 << 9    // convert relative Url links to absolute
    };

/**
 * @author Robby Stephenson
 */
class Exporter : public QObject {
Q_OBJECT

public:
  Exporter(Data::CollPtr coll, const QUrl& baseUrl=QUrl());
  virtual ~Exporter();

  Data::CollPtr collection() const;

  void setURL(const QUrl& url_) { m_targetUrl = url_; }
  void setEntries(const Data::EntryList& entries) { m_entries = entries; }
  void setFields(const Data::FieldList& fields) { m_fields = fields; }
  void setOptions(long options) { m_options = options; reset(); }

  // used for saving config options, do not translate
  virtual QString formatString() const = 0;
  virtual QString fileFilter() const = 0;
  const QUrl& baseUrl() const { return m_baseUrl; }
  const QUrl& url() const { return m_targetUrl; }
  const Data::EntryList& entries() const { return m_entries; }
  const Data::FieldList& fields() const;
  long options() const { return m_options; }

  /**
   * Do the export
   */
  virtual bool exec() = 0;
  /**
   * If changing options in the exporter should cause member variables to reset, implement
   * that here
   */
  virtual void reset() {}

  virtual QWidget* widget(QWidget* parent) = 0;
  virtual void readOptions(KSharedConfigPtr) {}
  virtual void saveOptions(KSharedConfigPtr) {}

private:
  long m_options;
  Data::CollPtr m_coll;
  Data::EntryList m_entries;
  Data::FieldList m_fields;
  QUrl m_baseUrl;
  QUrl m_targetUrl;
};

  } // end namespace
} // end namespace
#endif
