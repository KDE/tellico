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

#include "interface.h"

#include <cppuhelper/bootstrap.hxx>
#include <cppuhelper/implbase1.hxx>
#include <com/sun/star/bridge/XUnoUrlResolver.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/text/ControlCharacter.hpp>
#include <com/sun/star/text/XDocumentIndexesSupplier.hpp>
#include <com/sun/star/text/XDocumentIndex.hpp>
#include <com/sun/star/text/XTextField.hpp>
#include <com/sun/star/text/XTextViewCursorSupplier.hpp>
#include <com/sun/star/text/BibliographyDataType.hpp>
#include <com/sun/star/sdbc/XRowSet.hpp>
#include <com/sun/star/sdbc/XResultSetMetaDataSupplier.hpp>
#include <com/sun/star/sdbc/XResultSetUpdate.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XRowUpdate.hpp>
#include <com/sun/star/sdbc/SQLException.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/document/XEventListener.hpp>
#include <com/sun/star/document/XEventBroadcaster.hpp>

#include <iostream>

#define DEBUG(s) std::cout << s << std::endl
#define OUSTR(s) ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(s))
#define OU2O(s) OUStringToOString(s, RTL_TEXTENCODING_ASCII_US)
#define O2OU(s) OStringToOUString(s.c_str(), RTL_TEXTENCODING_UTF8)

using Tellico::Cite::OOOHandler;

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace rtl;
using namespace cppu;

namespace Tellico {
  namespace Cite {
    typedef cppu::WeakImplHelper1<document::XEventListener> EventListenerHelper;
  }
}

class OOOHandler::Interface::EventListener : public cppu::WeakImplHelper1<document::XEventListener> {
public:
  EventListener(OOOHandler::Interface* i) : EventListenerHelper(), m_interface(i) {}
  virtual void SAL_CALL disposing(const lang::EventObject&) throw(RuntimeException) {
    DEBUG("Document is being disposed");
    m_interface->disconnect();
  }
  virtual void SAL_CALL notifyEvent(const document::EventObject&) throw(RuntimeException) {
//    std::cout << "Event: " << rtl::OUStringToOString(aEvent.EventName,RTL_TEXTENCODING_ISO_8859_1).getStr() << std::endl;
  }
private:
  OOOHandler::Interface* m_interface;
};

OOOHandler::Interface::Interface() : m_listener(0) {
}

OOOHandler::Interface::~Interface() {
  delete m_listener;
  m_listener = 0;
}

bool OOOHandler::Interface::isConnected() const {
  return m_gsmgr.is();
}

bool OOOHandler::Interface::connect(const std::string& host_, int port_, const std::string& pipe_) {
  if(isConnected()) {
    return true;
  }

  // create the initial component context
  Reference<uno::XComponentContext> context;
  try {
    context = defaultBootstrap_InitialComponentContext();
  } catch(Exception& e) {
    OString o = OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US);
    std::cout << "Unable to get initial component context: " << o << std::endl;
    return false;
  } catch(...) {
    DEBUG("Unable to get initial component context.");
    return false;
  }

  // retrieve the servicemanager from the context
  Reference<lang::XMultiComponentFactory> rServiceManager;
  try {
    rServiceManager = context->getServiceManager();
  } catch(...) {
    DEBUG("Unable to get initial service manager.");
    return false;
  }

  // instantiate a sample service with the servicemanager.
  OUString s = OUString::createFromAscii("com.sun.star.bridge.UnoUrlResolver");
  Reference<uno::XInterface> rInstance;
  try {
    rInstance = rServiceManager->createInstanceWithContext(s, context);
  } catch(...) {
    DEBUG("Unable to get initial instance.");
    return false;
  }

  // Query for the XUnoUrlResolver interface
  Reference<bridge::XUnoUrlResolver> rResolver(rInstance, UNO_QUERY);
  if(!rResolver.is()) {
    DEBUG("Error: Couldn't instantiate com.sun.star.bridge.UnoUrlResolver service");
    return false;
  }

  // "uno:socket,host=%s,port=%s;urp;StarOffice.ComponentContext"%(host,port)
  // "uno:pipe,name=%s;urp;StarOffice.ComponentContext"%pipe
  if(pipe_.empty()) {
    s = OUSTR("socket,host=") + O2OU(host_) + OUSTR(",port=") + OUString::valueOf((sal_Int32)port_);
  } else {
    s = OUSTR("pipe,name=") + O2OU(pipe_);
  }
  std::cout << "Connection string: " << OU2O(s) << std::endl;
  s = OUSTR("uno:") + s + OUSTR(";urp;StarOffice.ServiceManager");

  try {
    rInstance = rResolver->resolve(s);
    if(!rInstance.is()) {
      DEBUG("StarOffice.ServiceManager is not exported from remote counterpart");
      return false;
    }

    m_gsmgr = Reference<lang::XMultiServiceFactory>(rInstance, UNO_QUERY);
    if(m_gsmgr.is()) {
      DEBUG("Connected sucessfully to office");
    } else {
      DEBUG("XMultiServiceFactory interface is not exported");
    }
  } catch(Exception& e) {
    std::cout << "Error: " << OU2O(e.Message) << std::endl;
  } catch(...) {
    DEBUG("Unable to resolve connection.");
    return false;
  }
  return m_gsmgr.is();
}

bool OOOHandler::Interface::disconnect() {
  m_gsmgr = 0;
  m_dsmgr = 0;
  m_doc = 0;
  m_bib = 0;
  m_cursor = 0;
  return true;
}

bool OOOHandler::Interface::createDocument() {
  if(!m_gsmgr.is()) {
    return false;
  }

  if(m_doc.is()) {
    return true;
  }

  // get the desktop service using createInstance, returns an XInterface type
  Reference<uno::XInterface> xInstance = m_gsmgr->createInstance(OUString::createFromAscii("com.sun.star.frame.Desktop"));
  Reference<frame::XDesktop> desktop(xInstance, UNO_QUERY);

  Reference<lang::XComponent> writer = desktop->getCurrentComponent();
  Reference<lang::XServiceInfo> info(writer, UNO_QUERY);
  if(info.is() && info->getImplementationName() == OUString::createFromAscii("SwXTextDocument")) {
    DEBUG("Document already open");
  } else {
    DEBUG("Opening a new document");
    //query for the XComponentLoader interface
    Reference<frame::XComponentLoader> rComponentLoader(desktop, UNO_QUERY);
    if(!rComponentLoader.is()){
      DEBUG("XComponentloader failed to instantiate");
      return 0;
    }

    //get an instance of the OOowriter document
    writer = rComponentLoader->loadComponentFromURL(OUSTR("private:factory/swriter"),
                                                    OUSTR("_default"),
                                                    0,
                                                    Sequence<beans::PropertyValue>());
  }

  //Manage many events with EventListener
  Reference<document::XEventBroadcaster> eventBroadcast(writer, UNO_QUERY);
  m_listener = new EventListener(this);
  Reference<document::XEventListener> xEventListener = static_cast<document::XEventListener*>(m_listener);
  eventBroadcast->addEventListener(xEventListener);

  Reference<frame::XController> controller = Reference<frame::XModel>(writer, UNO_QUERY)->getCurrentController();
  m_cursor = Reference<text::XTextViewCursorSupplier>(controller, UNO_QUERY)->getViewCursor();
  m_doc = Reference<text::XTextDocument>(writer, UNO_QUERY);
  if(m_doc.is()) {
    m_dsmgr = Reference<lang::XMultiServiceFactory>(m_doc, UNO_QUERY);
  }
  return m_doc.is();
}

bool OOOHandler::Interface::updateBibliography() {
  if(!m_bib.is()) {
    createBibliography();
    if(!m_bib.is()) {
      DEBUG("ERROR: could not create or find bibliography index");
      return false;
    }
  }

  m_bib->update();
  return true;
}

void OOOHandler::Interface::createBibliography() {
  Reference<container::XIndexAccess> indexes(Reference<text::XDocumentIndexesSupplier>(m_doc, UNO_QUERY)->getDocumentIndexes(), UNO_QUERY);
  for(int i = 0; i < indexes->getCount(); ++i) {
    Reference<lang::XServiceInfo> info(indexes->getByIndex(i), UNO_QUERY);
    if(info->supportsService(OUSTR("com.sun.star.text.Bibliography"))) {
      DEBUG("Found existing bibliography...");
      m_bib = Reference<text::XDocumentIndex>(indexes->getByIndex(i), UNO_QUERY);
      break;
    }
  }

  if(!m_bib.is()) {
    DEBUG("Creating new bibliography...");
    Reference<text::XText> text = m_doc->getText();
    Reference<text::XTextRange> textRange(text->createTextCursor(), UNO_QUERY);
    Reference<text::XTextCursor> cursor(textRange, UNO_QUERY);
    cursor->gotoEnd(false);
    text->insertControlCharacter(textRange, text::ControlCharacter::PARAGRAPH_BREAK, false);
    m_bib = Reference<text::XDocumentIndex>(m_dsmgr->createInstance(OUSTR("com.sun.star.text.Bibliography")), UNO_QUERY);
    Reference<text::XTextContent> textContent(m_bib, UNO_QUERY);
    text->insertTextContent(textRange, textContent, false);
  }
}

bool OOOHandler::Interface::insertCitations(Cite::Map& fields) {
  Reference<text::XTextField> entry(m_dsmgr->createInstance(OUString::createFromAscii("com.sun.star.text.TextField.Bibliography")), UNO_QUERY);
  if(!entry.is()) {
    DEBUG("Interface::insertCitation - can't create TextField");
    return false;
  }
  Sequence<beans::PropertyValue> values(fields.size());
  int i = 0;
  for(Cite::Map::iterator it = fields.begin(); it != fields.end(); ++it, ++i) {
    values[i] = propValue(it->first, it->second);
    std::cout << "Setting " << OU2O(values[i].Name) << " = " << it->second << std::endl;
  }
  Reference<beans::XPropertySet>(entry, UNO_QUERY)->setPropertyValue(OUSTR("Fields"), Any(values));

  Reference<text::XText> text = m_doc->getText();
  Reference<text::XTextCursor> cursor = text->createTextCursorByRange(Reference<text::XTextRange>(m_cursor, UNO_QUERY));
  Reference<text::XTextRange> textRange(cursor, UNO_QUERY);
  Reference<text::XTextContent> textContent(entry, UNO_QUERY);
  text->insertTextContent(textRange, textContent, false);
  return true;
}

beans::PropertyValue OOOHandler::Interface::propValue(const std::string& field, const std::string& value) {
  return beans::PropertyValue(O2OU(field), 0, fieldValue(field, value), beans::PropertyState_DIRECT_VALUE);
}

uno::Any OOOHandler::Interface::fieldValue(const std::string& field, const std::string& value) {
  if(field == "BibiliographicType" || field == "BibliographicType") { // in case the typo gets fixed
    return typeValue(value);
  }
  return Any(O2OU(value));
}

uno::Any OOOHandler::Interface::typeValue(const std::string& value) {
  if(value == "article") {
    return Any(text::BibliographyDataType::ARTICLE);
  } else if(value == "book") {
    return Any(text::BibliographyDataType::BOOK);
  } else if(value == "booklet") {
    return Any(text::BibliographyDataType::BOOKLET);
  } else if(value == "conference") {
    return Any(text::BibliographyDataType::CONFERENCE);
  } else if(value == "inbook") {
    return Any(text::BibliographyDataType::INBOOK);
  } else if(value == "incollection") {
    return Any(text::BibliographyDataType::INCOLLECTION);
  } else if(value == "inproceedings") {
    return Any(text::BibliographyDataType::INPROCEEDINGS);
  } else if(value == "journal") {
    return Any(text::BibliographyDataType::JOURNAL);
  } else if(value == "manual") {
    return Any(text::BibliographyDataType::MANUAL);
  } else if(value == "mastersthesis") {
    return Any(text::BibliographyDataType::MASTERSTHESIS);
  } else if(value == "misc") {
    return Any(text::BibliographyDataType::MISC);
  } else if(value == "phdthesis") {
    return Any(text::BibliographyDataType::PHDTHESIS);
  } else if(value == "proceedings") {
    return Any(text::BibliographyDataType::PROCEEDINGS);
  } else if(value == "techreport") {
    return Any(text::BibliographyDataType::TECHREPORT);
  } else if(value == "unpublished") {
    return Any(text::BibliographyDataType::UNPUBLISHED);
  } else {
    // periodical ?
    return Any(text::BibliographyDataType::BOOK);
  }
}

bool OOOHandler::Interface::insertRecords(Cite::Map& fields) {
  Reference<uno::XInterface> interface;
  try {
    interface = m_gsmgr->createInstance(OUString::createFromAscii("com.sun.star.sdb.RowSet"));
  } catch(Exception& e) {
    std::cout << "Error: " << OU2O(e.Message) << std::endl;
  }
  if(!interface.is()) {
    DEBUG("Could not create rowset interface");
    return false;
  }

  Reference<sdbc::XRowSet> rowSet(interface, UNO_QUERY);
  if(!rowSet.is()) {
    DEBUG("Could not create rowset interface");
    return false;
  }

  Reference<beans::XPropertySet> props(rowSet, UNO_QUERY);
  props->setPropertyValue(OUSTR("DataSourceName"), Any(OUSTR("Bibliography")));
  props->setPropertyValue(OUSTR("CommandType"),    Any(sdb::CommandType::COMMAND));
  OUString s = OUSTR("SELECT COUNT(*) FROM \"biblio\" WHERE identifier='") + O2OU(fields["Identifier"]) + OUSTR("'");
  props->setPropertyValue(OUSTR("Command"),        Any(s));

  try {
    rowSet->execute();
  } catch(sdbc::SQLException& e) {
    DEBUG(OU2O(s));
    DEBUG(OUSTR("SQL error - ") + e.SQLState);
    return false;
  } catch(Exception& e) {
    DEBUG(OU2O(s));
    DEBUG(OUSTR("General error - ") + e.Message);
    return false;
  }

  Reference<sdbc::XRow> row(rowSet, UNO_QUERY);
  int count = 0;
  if(rowSet->next()) {
    count = row->getString(1).toInt32();
  }

  if(count > 0) {
    DEBUG("Found existing bibliographic entries, updating...");
  } else {
    DEBUG("Inserting new bibliographic entry...");
  }

  s = OUSTR("SELECT * FROM \"biblio\"");
  if(count > 0) {
    s += OUSTR(" WHERE identifier='") + O2OU(fields["Identifier"]) + OUSTR("'");
  }
  props->setPropertyValue(OUSTR("Command"),        Any(s));

  try {
    rowSet->execute();
  } catch(sdbc::SQLException& e) {
    DEBUG(OU2O(s));
    DEBUG(OUSTR("SQL error(2) - ") + e.SQLState);
    return false;
  } catch(Exception& e) {
    DEBUG(OU2O(s));
    DEBUG(OUSTR("General error(2) - ") + e.Message);
    return false;
  }

  Reference<sdbc::XResultSet> resultSet(rowSet, UNO_QUERY);
  Reference<sdbc::XResultSetMetaDataSupplier> mdSupplier(resultSet, UNO_QUERY);
  Reference<sdbc::XResultSetMetaData> metaData = mdSupplier->getMetaData();

  Reference<sdbc::XRowUpdate> rowUpdate(rowSet, UNO_QUERY);
  Reference<sdbc::XResultSetUpdate> update(rowSet, UNO_QUERY);
  if(count > 0) {
    resultSet->last();
  } else {
    update->moveToInsertRow();
  }

  const long colCount = metaData->getColumnCount();
  // column numbers start with 1
  for(long i = 1; i <= colCount; ++i) {
    std::string s = OU2O(metaData->getColumnName(i)).getStr();
    std::string value = fields[s];
    if(!value.empty()) {
      std::cout << "column " << i << ": " << OU2O(metaData->getColumnName(i)) << "..." << std::endl;
      std::cout << s << " = " << value << std::endl;
      try {
        rowUpdate->updateString(i, O2OU(value));
      } catch(sdbc::SQLException& e) {
        DEBUG(OUSTR("SQL error(3) - ") + e.SQLState);
      } catch(Exception& e) {
        DEBUG(OUSTR("General error(3) - ") + e.Message);
      }
    }
  }
  if(count > 0) {
    update->updateRow();
  } else {
    update->insertRow();
  }

  Reference<lang::XComponent>(rowSet, UNO_QUERY)->dispose();
  return true;
}
