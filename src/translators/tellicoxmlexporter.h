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

#ifndef TELLICOXMLEXPORTER_H
#define TELLICOXMLEXPORTER_H

namespace Tellico {
  namespace Data {
    class Field;
    class Entry;
  }
}

class QDomDocument;
class QDomElement;
class QCheckBox;

#include "textexporter.h"
#include "../image.h"

#include "qstringlist.h"

namespace Tellico {
  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: tellicoxmlexporter.h 867 2004-09-15 03:04:49Z robby $
 */
class TellicoXMLExporter : public TextExporter {
public:
  TellicoXMLExporter(const Data::Collection* coll) : TextExporter(coll),
     m_includeImages(false), m_includeGroups(false), m_widget(0) {}

  virtual QWidget* widget(QWidget*, const char*);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig* cfg);
  virtual void saveOptions(KConfig* cfg);
  QDomDocument exportXML(bool format, bool encodeUTF8) const;
  void setIncludeImages(bool b) { m_includeImages = b; }
  void setIncludeGroups(bool b) { m_includeGroups = b; }

  /**
   * An integer indicating format version.
   */
  static const unsigned syntaxVersion;

private:
  void exportCollectionXML(QDomDocument& doc, QDomElement& parent, bool format) const;
  void exportFieldXML(QDomDocument& doc, QDomElement& parent, Data::Field* field) const;
  void exportEntryXML(QDomDocument& doc, QDomElement& parent, Data::Entry* entry, bool format) const;
  void exportImageXML(QDomDocument& doc, QDomElement& parent, const Data::Image& image) const;
  void exportGroupXML(QDomDocument& doc, QDomElement& parent) const;

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
