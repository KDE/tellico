/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
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
  namespace Data {
    class Field;
    class Entry;
    class Borrower;
  }
  class Filter;
}

class QDomDocument;
class QDomElement;
class QCheckBox;

#include "exporter.h"
#include "../image.h"

#include "qstringlist.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 */
class TellicoXMLExporter : public Exporter {
public:
  TellicoXMLExporter() : Exporter(),
      m_includeImages(false), m_includeGroups(false), m_widget(0) {}
  TellicoXMLExporter(const Data::Collection* coll) : Exporter(coll),
      m_includeImages(false), m_includeGroups(false), m_widget(0) {}

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
  void exportFieldXML(QDomDocument& doc, QDomElement& parent, Data::Field* field) const;
  void exportEntryXML(QDomDocument& doc, QDomElement& parent, const Data::Entry* entry, bool format) const;
  void exportImageXML(QDomDocument& doc, QDomElement& parent, const Data::Image& image) const;
  void exportGroupXML(QDomDocument& doc, QDomElement& parent) const;
  void exportFilterXML(QDomDocument& doc, QDomElement& parent, const Filter* filter) const;
  void exportBorrowerXML(QDomDocument& doc, QDomElement& parent, const Data::Borrower* borrower) const;

  // keep track of which images were written, since some entries could have same image
  mutable QStringList m_imageList;
  bool m_includeImages;
  bool m_includeGroups;

  QWidget* m_widget;
  QCheckBox* m_checkIncludeImages;
};

  } // end namespace
} // end namespace
#endif
