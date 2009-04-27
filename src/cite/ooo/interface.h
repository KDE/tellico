/***************************************************************************
    Copyright (C) 2006-2009 Robby Stephenson <robby@periapsis.org>

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
