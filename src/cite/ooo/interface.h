/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_CITE_OOOHANDLER_INTERFACE_H
#define TELLICO_CITE_OOOHANDLER_INTERFACE_H

#include "ooohandler.h"

#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XDocumentIndex.hpp>
#include <com/sun/star/text/XTextViewCursor.hpp>

namespace Tellico {
  namespace Cite {

class OOOHandler::Interface {
  friend class OOOHandler;

  Interface();
  ~Interface();

  bool isConnected() const;
  bool connect(const std::string& host, int port, const std::string& pipe);
  bool disconnect();
  bool createDocument();
  bool updateBibliography();
  bool insertCitations(Cite::Map& fields);
  bool insertRecords(Cite::Map& fields);

private:
  void createBibliography();
  com::sun::star::beans::PropertyValue propValue(const std::string& field, const std::string& value);
  com::sun::star::uno::Any fieldValue(const std::string& field, const std::string& value);
  com::sun::star::uno::Any typeValue(const std::string& value);

  // global service manager
  com::sun::star::uno::Reference<com::sun::star::lang::XMultiServiceFactory> m_gsmgr;
  // document service manager
  com::sun::star::uno::Reference<com::sun::star::lang::XMultiServiceFactory> m_dsmgr;
  com::sun::star::uno::Reference<com::sun::star::text::XTextDocument> m_doc;
  com::sun::star::uno::Reference<com::sun::star::text::XDocumentIndex> m_bib;
  com::sun::star::uno::Reference<com::sun::star::text::XTextViewCursor> m_cursor;

  class EventListener;
  EventListener* m_listener;
};

  } // end namespace
} // end namespace

#endif
