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

#ifndef TELLICOIMPORTER_H
#define TELLICOIMPORTER_H

#include "dataimporter.h"

#include <qdom.h>

namespace Tellico {
  namespace Import {

/**
 * Reading the @ref Tellico data files is done by the TellicoImporter.
 *
 * @author Robby Stephenson
 * @version $Id: tellicoimporter.h 921 2004-10-13 06:36:33Z robby $
 */
class TellicoImporter : public DataImporter {
Q_OBJECT

public:
  /**
   * @param url The tellico data file.
   */
  TellicoImporter(const KURL& url);
  /**
   * Constructor used to convert arbitrary text to a @ref Collection
   *
   * @param text The text
   */
  TellicoImporter(const QString& text);

  /**
   */
  virtual Data::Collection* collection();

private:
  void loadXMLData(const QByteArray& data, bool loadImages);
  void loadZipData(const QByteArray& data);

  void readField(unsigned syntaxVersion, const QDomElement& elem);
  void readEntry(unsigned syntaxVersion, const QDomNode& elem);
  void readImage(const QDomElement& elem);

  Data::Collection* m_coll;
  QString m_namespace;
};

  } // end namespace
} // end namespace
#endif
