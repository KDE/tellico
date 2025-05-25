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

#ifndef TELLICO_XMLEXPORTER_H
#define TELLICO_XMLEXPORTER_H

#include "exporter.h"
#include "../utils/stringset.h"

namespace Tellico {
  class Filter;
}

class QDomDocument;
class QDomElement;
class QCheckBox;

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class TellicoXMLExporter : public Exporter {
Q_OBJECT

public:
  TellicoXMLExporter(Data::CollPtr coll, const QUrl& baseUrl);
  ~TellicoXMLExporter();

  virtual bool exec() override;
  virtual QString formatString() const override;
  virtual QString fileFilter() const override;

  QString text() const;
  QDomDocument exportXML() const;

  void setIncludeImages(bool b) { m_includeImages = b; }
  void setIncludeGroups(bool b) { m_includeGroups = b; }

  virtual QWidget* widget(QWidget*) override;
  virtual void readOptions(KSharedConfigPtr cfg) override;
  virtual void saveOptions(KSharedConfigPtr cfg) override;

  /**
   * An integer indicating format version.
   */
  static const unsigned syntaxVersion;

private:
  void exportCollectionXML(QDomDocument& doc, QDomElement& parent, int format) const;
  void exportFieldXML(QDomDocument& doc, QDomElement& parent, Data::FieldPtr field) const;
  void exportEntryXML(QDomDocument& doc, QDomElement& parent, Data::EntryPtr entry, int format) const;
  void exportImageXML(QDomDocument& doc, QDomElement& parent, const QString& imageID) const;
  void exportGroupXML(QDomDocument& doc, QDomElement& parent) const;
  void exportFilterXML(QDomDocument& doc, QDomElement& parent, FilterPtr filter) const;
  void exportBorrowerXML(QDomDocument& doc, QDomElement& parent, Data::BorrowerPtr borrower) const;

  Data::EntryList sortEntries(const Data::EntryList& entries) const;
  bool version12Needed() const;

  // keep track of which images were written, since some entries could have same image
  mutable StringSet m_images;
  bool m_includeImages;
  bool m_includeGroups;

  QWidget* m_widget;
  QCheckBox* m_checkIncludeImages;
};

  } // end namespace
} // end namespace
#endif
