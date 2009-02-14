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

#ifndef TELLICO_IMPORT_XMLSTATEHANDLER_H
#define TELLICO_IMPORT_XMLSTATEHANDLER_H

#ifdef QT_NO_CAST_ASCII
#define HAD_QT_NO_CAST_ASCII
#undef QT_NO_CAST_ASCII
#endif

#include <qxml.h>

#ifdef HAD_QT_NO_CAST_ASCII
#define QT_NO_CAST_ASCII
#undef HAD_QT_NO_CAST_ASCII
#endif

#include "../datavectors.h"

namespace Tellico {
  namespace Import {
    namespace SAX {

class StateData {
public:
  QString text;
  QString error;
  QString ns; // namespace
  QString textBuffer;
  uint syntaxVersion;
  QString collTitle;
  int collType;
  QString entryName;
  Data::CollPtr coll;
  Data::FieldVec fields;
  Data::FieldPtr currentField;
  Data::EntryVec entries;
  FilterPtr filter;
  Data::BorrowerPtr borrower;
  bool defaultFields;
  bool loadImages;
  bool hasImages;
};

class StateHandler {
public:
  StateHandler(StateData* data) : d(data) {}
  virtual ~StateHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&) = 0;
  virtual bool   end(const QString&, const QString&, const QString&) = 0;

  StateHandler* nextHandler(const QString&, const QString&, const QString&);
protected:
  StateData* d;
private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&)  { return 0; }
};

class NullHandler : public StateHandler {
public:
  NullHandler(StateData* data) : StateHandler(data) {}
  virtual ~NullHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&) { return true; }
  virtual bool   end(const QString&, const QString&, const QString&) { return true; }
};

class RootHandler : public StateHandler {
public:
  RootHandler(StateData* data) : StateHandler(data) {}
  virtual ~RootHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&) { return true; }
  virtual bool   end(const QString&, const QString&, const QString&) { return true; }

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class DocumentHandler : public StateHandler {
public:
  DocumentHandler(StateData* data) : StateHandler(data) {}
  virtual ~DocumentHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class CollectionHandler : public StateHandler {
public:
  CollectionHandler(StateData* data) : StateHandler(data) {}
  virtual ~CollectionHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class FieldsHandler : public StateHandler {
public:
  FieldsHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldsHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class FieldHandler : public StateHandler {
public:
  FieldHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class FieldPropertyHandler : public StateHandler {
public:
  FieldPropertyHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldPropertyHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  QString m_propertyName;
};

class BibtexPreambleHandler : public StateHandler {
public:
  BibtexPreambleHandler(StateData* data) : StateHandler(data) {}
  virtual ~BibtexPreambleHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);
};

class BibtexMacrosHandler : public StateHandler {
public:
  BibtexMacrosHandler(StateData* data) : StateHandler(data) {}
  virtual ~BibtexMacrosHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class BibtexMacroHandler : public StateHandler {
public:
  BibtexMacroHandler(StateData* data) : StateHandler(data) {}
  virtual ~BibtexMacroHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  QString m_macroName;
};

class EntryHandler : public StateHandler {
public:
  EntryHandler(StateData* data) : StateHandler(data) {}
  virtual ~EntryHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class FieldValueContainerHandler : public StateHandler {
public:
  FieldValueContainerHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldValueContainerHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class FieldValueHandler : public StateHandler {
public:
  FieldValueHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldValueHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
  bool m_i18n;
  bool m_validateISBN;
};

class DateValueHandler : public StateHandler {
public:
  DateValueHandler(StateData* data) : StateHandler(data) {}
  virtual ~DateValueHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);
};

class TableColumnHandler : public StateHandler {
public:
  TableColumnHandler(StateData* data) : StateHandler(data) {}
  virtual ~TableColumnHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);
};

class ImagesHandler : public StateHandler {
public:
  ImagesHandler(StateData* data) : StateHandler(data) {}
  virtual ~ImagesHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class ImageHandler : public StateHandler {
public:
  ImageHandler(StateData* data) : StateHandler(data) {}
  virtual ~ImageHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  QString m_format;
  bool m_link;
  QString m_imageId;
  int m_width;
  int m_height;
};

class FiltersHandler : public StateHandler {
public:
  FiltersHandler(StateData* data) : StateHandler(data) {}
  virtual ~FiltersHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class FilterHandler : public StateHandler {
public:
  FilterHandler(StateData* data) : StateHandler(data) {}
  virtual ~FilterHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class FilterRuleHandler : public StateHandler {
public:
  FilterRuleHandler(StateData* data) : StateHandler(data) {}
  virtual ~FilterRuleHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);
};

class BorrowersHandler : public StateHandler {
public:
  BorrowersHandler(StateData* data) : StateHandler(data) {}
  virtual ~BorrowersHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class BorrowerHandler : public StateHandler {
public:
  BorrowerHandler(StateData* data) : StateHandler(data) {}
  virtual ~BorrowerHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  virtual StateHandler* nextHandlerImpl(const QString&, const QString&, const QString&);
};

class LoanHandler : public StateHandler {
public:
  LoanHandler(StateData* data) : StateHandler(data) {}
  virtual ~LoanHandler() {}

  virtual bool start(const QString&, const QString&, const QString&, const QXmlAttributes&);
  virtual bool   end(const QString&, const QString&, const QString&);

private:
  int m_id;
  QString m_uid;
  QString m_loanDate;
  QString m_dueDate;
  bool m_inCalendar;
};

    }
  }
}
#endif
