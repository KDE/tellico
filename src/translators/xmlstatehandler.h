/***************************************************************************
    Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_IMPORT_XMLSTATEHANDLER_H
#define TELLICO_IMPORT_XMLSTATEHANDLER_H

#include <QXmlStreamAttributes>
#include <QUrl>

#include "../datavectors.h"

namespace Tellico {
  namespace Import {
    namespace SAX {

class StateData {
public:
  StateData() : syntaxVersion(0), collType(0), defaultFields(false), loadImages(false), hasImages(false), showImageLoadErrors(true) {}
  QString text;
  QString error;
  QString ns; // namespace
  QString textBuffer;
  uint syntaxVersion;
  QString collTitle;
  int collType;
  QString entryName;
  Data::CollPtr coll;
  Data::FieldList fields;
  Data::FieldPtr currentField;
  Data::EntryList entries;
  QString modifiedDate;
  FilterPtr filter;
  Data::BorrowerPtr borrower;
  bool defaultFields;
  bool loadImages;
  bool hasImages;
  bool showImageLoadErrors;
  QUrl baseUrl;
};

class StateHandler {
public:
  StateHandler(StateData* data) : d(data) {}
  virtual ~StateHandler() {}

  virtual bool start(const QStringRef& nsUri, const QStringRef& localName, const QXmlStreamAttributes& attributes) = 0;
  virtual bool   end(const QStringRef& nsUri, const QStringRef& localName) = 0;

  StateHandler* nextHandler(const QStringRef& nsUri, const QStringRef& localName);
protected:
  StateData* d;
private:
  Q_DISABLE_COPY(StateHandler)
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&)  { return nullptr; }
};

class NullHandler : public StateHandler {
public:
  NullHandler(StateData* data) : StateHandler(data) {}
  virtual ~NullHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE { return true; }
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE { return true; }
};

class RootHandler : public StateHandler {
public:
  RootHandler(StateData* data) : StateHandler(data) {}
  virtual ~RootHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE { return true; }
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE { return true; }

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class DocumentHandler : public StateHandler {
public:
  DocumentHandler(StateData* data) : StateHandler(data) {}
  virtual ~DocumentHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class CollectionHandler : public StateHandler {
public:
  CollectionHandler(StateData* data) : StateHandler(data) {}
  virtual ~CollectionHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class FieldsHandler : public StateHandler {
public:
  FieldsHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldsHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class FieldHandler : public StateHandler {
public:
  FieldHandler(StateData* data) : StateHandler(data), isI18n(false) {}
  virtual ~FieldHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
  bool isI18n;
};

class FieldPropertyHandler : public StateHandler {
public:
  FieldPropertyHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldPropertyHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  QString m_propertyName;
};

class BibtexPreambleHandler : public StateHandler {
public:
  BibtexPreambleHandler(StateData* data) : StateHandler(data) {}
  virtual ~BibtexPreambleHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class BibtexMacrosHandler : public StateHandler {
public:
  BibtexMacrosHandler(StateData* data) : StateHandler(data) {}
  virtual ~BibtexMacrosHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class BibtexMacroHandler : public StateHandler {
public:
  BibtexMacroHandler(StateData* data) : StateHandler(data) {}
  virtual ~BibtexMacroHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  QString m_macroName;
};

class EntryHandler : public StateHandler {
public:
  EntryHandler(StateData* data) : StateHandler(data) {}
  virtual ~EntryHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class FieldValueContainerHandler : public StateHandler {
public:
  FieldValueContainerHandler(StateData* data) : StateHandler(data) {}
  virtual ~FieldValueContainerHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class FieldValueHandler : public StateHandler {
public:
  FieldValueHandler(StateData* data) : StateHandler(data)
    , m_i18n(false), m_validateISBN(false) {}
  virtual ~FieldValueHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
  bool m_i18n;
  bool m_validateISBN;
};

class DateValueHandler : public StateHandler {
public:
  DateValueHandler(StateData* data) : StateHandler(data) {}
  virtual ~DateValueHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class TableColumnHandler : public StateHandler {
public:
  TableColumnHandler(StateData* data) : StateHandler(data) {}
  virtual ~TableColumnHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class ImagesHandler : public StateHandler {
public:
  ImagesHandler(StateData* data) : StateHandler(data) {}
  virtual ~ImagesHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class ImageHandler : public StateHandler {
public:
  ImageHandler(StateData* data) : StateHandler(data)
    , m_link(false), m_width(0), m_height(0) {}
  virtual ~ImageHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

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

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class FilterHandler : public StateHandler {
public:
  FilterHandler(StateData* data) : StateHandler(data) {}
  virtual ~FilterHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class FilterRuleHandler : public StateHandler {
public:
  FilterRuleHandler(StateData* data) : StateHandler(data) {}
  virtual ~FilterRuleHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class BorrowersHandler : public StateHandler {
public:
  BorrowersHandler(StateData* data) : StateHandler(data) {}
  virtual ~BorrowersHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class BorrowerHandler : public StateHandler {
public:
  BorrowerHandler(StateData* data) : StateHandler(data) {}
  virtual ~BorrowerHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

private:
  virtual StateHandler* nextHandlerImpl(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;
};

class LoanHandler : public StateHandler {
public:
  LoanHandler(StateData* data) : StateHandler(data)
    , m_id(-1), m_inCalendar(false) {}
  virtual ~LoanHandler() {}

  virtual bool start(const QStringRef&, const QStringRef&, const QXmlStreamAttributes&) Q_DECL_OVERRIDE;
  virtual bool   end(const QStringRef&, const QStringRef&) Q_DECL_OVERRIDE;

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
