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

#ifndef BOOKCASEXMLEXPORTER_H
#define BOOKCASEXMLEXPORTER_H

class QDomDocument;
class QDomElement;
class QCheckBox;

#include "textexporter.h"
#include "../image.h"

#include "qstringlist.h"

namespace Bookcase {
  namespace Data {
    class Field;
    class Entry;
  }

  namespace Export {

/**
 * @author Robby Stephenson
 * @version $Id: bookcasexmlexporter.h 386 2004-01-24 05:12:28Z robby $
 */
class BookcaseXMLExporter : public TextExporter {
public:
  BookcaseXMLExporter(const Data::Collection* coll, const Data::EntryList& list) : TextExporter(coll, list),
     m_exportImages(false), m_widget(0) {}

  virtual QWidget* widget(QWidget*, const char*);
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig* cfg);
  virtual void saveOptions(KConfig* cfg);
  QDomDocument exportXML(bool format, bool encodeUTF8) const;
  void exportImages(bool b) { m_exportImages = b; }

  /**
   * An integer indicating format version.
   */
  static const unsigned syntaxVersion;

private:
  void exportCollectionXML(QDomDocument& doc, QDomElement& parent, bool format) const;
  void exportFieldXML(QDomDocument& doc, QDomElement& parent, Data::Field* field) const;
  void exportEntryXML(QDomDocument& doc, QDomElement& parent, Data::Entry* unit, bool format) const;
  void exportImageXML(QDomDocument& doc, QDomElement& parent, const Data::Image& image) const;

  // keep track of which images were written, since some entries could have same image
  mutable QStringList m_imageList;
  bool m_exportImages;

  QWidget* m_widget;
  QCheckBox* m_checkExportImages;
};

  } // end namespace
} // end namespace
#endif
