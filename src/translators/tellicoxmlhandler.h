/***************************************************************************
    copyright            : (C) 2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_IMPORT_TELLICOXMLHANDLER_H
#define TELLICO_IMPORT_TELLICOXMLHANDLER_H

#include "xmlstatehandler.h"

#include <Q3PtrStack>

namespace Tellico {
  namespace Import {

class TellicoXMLHandler : public QXmlDefaultHandler {
public:
  TellicoXMLHandler();
  ~TellicoXMLHandler();

  virtual bool startElement(const QString& namespaceURI, const QString& localName,
                            const QString& qName, const QXmlAttributes& atts);
  virtual bool endElement(const QString& namespaceURI, const QString& localName,
                          const QString& qName);
  virtual bool characters(const QString& ch);

  virtual QString errorString() const;

  Data::CollPtr collection() const;
  bool hasImages() const;

  void setLoadImages(bool loadImages);

private:
  Q3PtrStack<SAX::StateHandler> m_handlers;
  SAX::StateData* m_data;
};

  }
}
#endif
