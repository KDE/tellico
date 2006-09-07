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

#ifndef TELLICOXMLEXPORTER_H
#define TELLICOXMLEXPORTER_H

namespace Tellico {
  class Filter;
}

class QDomDocument;
class QDomElement;
class QCheckBox;

#include "exporter.h"
#include "../stringset.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class TellicoXMLExporter : public Exporter {
Q_OBJECT

public:
  TellicoXMLExporter();
  TellicoXMLExporter(Data::CollPtr coll);

  virtual bool exec();
  virtual QString formatString() const;
  virtual QString fileFilter() const;

  QDomDocument exportXML() const;

  void setIncludeImages(bool b) { m_includeImages = b; }
  void setIncludeGroups(bool b) { m_includeGroups = b; }

  virtual QWidget* widget(QWidget*, const char*);
  virtual void readOptions(KConfig* cfg);
  virtual void saveOptions(KConfig* cfg);

  /**
   * An integer indicating format version.
   */
  static const unsigned syntaxVersion;

private:
  void exportCollectionXML(QDomDocument& doc, QDomElement& parent, bool format) const;
  void exportFieldXML(QDomDocument& doc, QDomElement& parent, Data::FieldPtr field) const;
  void exportEntryXML(QDomDocument& doc, QDomElement& parent, Data::EntryPtr entry, bool format) const;
  void exportImageXML(QDomDocument& doc, QDomElement& parent, const QString& imageID) const;
  void exportGroupXML(QDomDocument& doc, QDomElement& parent) const;
  void exportFilterXML(QDomDocument& doc, QDomElement& parent, FilterPtr filter) const;
  void exportBorrowerXML(QDomDocument& doc, QDomElement& parent, Data::BorrowerPtr borrower) const;

  // keep track of which images were written, since some entries could have same image
  mutable StringSet m_images;
  bool m_includeImages : 1;
  bool m_includeGroups : 1;

  QWidget* m_widget;
  QCheckBox* m_checkIncludeImages;
};

  } // end namespace
} // end namespace
#endif
