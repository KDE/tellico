/***************************************************************************
                            bookcasexmlexporter.h
                             -------------------
    begin                : Wed Sep 10 2003
    copyright            : (C) 2003 by Robby Stephenson
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
class BCAttribute;
class BCUnit;

#include "exporter.h"

/**
 * @author Robby Stephenson
 * @version $Id: xsltexporter.h 105 2003-09-04 04:54:50Z robby $
 */
class BookcaseXMLExporter : public Exporter {
public:
  BookcaseXMLExporter(const BCCollection* coll, const BCUnitList& list) : Exporter(coll, list) {}

  virtual QWidget* widget(QWidget*, const char*) { return 0; }
  virtual QString formatString() const;
  virtual QString text(bool format, bool encodeUTF8);
  virtual QString fileFilter() const;
  virtual void readOptions(KConfig*) {}
  virtual void saveOptions(KConfig*) {}
  QDomDocument exportXML(bool format, bool encodeUTF8) const;

  /**
   * An integer indicating format version.
   */
  static const unsigned syntaxVersion;

private:
  void exportCollectionXML(QDomDocument& doc, QDomElement& parent, bool format) const;
  void exportAttributeXML(QDomDocument& doc, QDomElement& parent, BCAttribute* att) const;
  void exportUnitXML(QDomDocument& doc, QDomElement& parent, BCUnit* unit, bool format) const;
};

#endif
