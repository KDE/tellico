/***************************************************************************
    copyright            : (C) 2003-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bibteximporter.h"
#include "bibtexhandler.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../latin1literal.h"

#include <kdebug.h>
#include <klocale.h>

#include <qptrlist.h>
#include <qregexp.h>

using Tellico::Import::BibtexImporter;

BibtexImporter::BibtexImporter(const KURL& url_) : Tellico::Import::TextImporter(url_), m_coll(0) {
  bt_initialize();
}

BibtexImporter::~BibtexImporter() {
  bt_cleanup();
}

bool BibtexImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::Collection* BibtexImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  parseText(text()); // populates m_nodes

  if(m_nodes.isEmpty()) {
    setStatusMessage(i18n("No valid bibtex entries were found in file - %1").arg(url().fileName()));
    return 0;
  } else {
//    kdDebug() << "BibtexImporter::collection() - found " << m_nodes.count() << " entries" << endl;
  }

  m_coll = new Data::BibtexCollection(true);
  m_coll->blockSignals(true);

  QString text;
  int j = 0;
  unsigned count = m_nodes.count();
  for(ASTListIterator it(m_nodes); it.current(); ++it, ++j) {
    // if we're parsing a macro string, comment or preamble, skip it for now
    if(bt_entry_metatype(it.current()) == BTE_PREAMBLE) {
      char* preamble = bt_get_text(it.current());
      if(preamble) {
        m_coll->setPreamble(QString::fromUtf8(preamble));
      }
      continue;
    }

    if(bt_entry_metatype(it.current()) == BTE_MACRODEF) {
      char* macro;
      (void) bt_next_field(it.current(), 0, &macro);
      // FIXME: replace macros within macro definitions!
      // lookup lowercase macro in map
      m_coll->addMacro(m_macros[QString::fromUtf8(macro)], QString::fromUtf8(bt_macro_text(macro, 0, 0)));
      continue;
    }

    if(bt_entry_metatype(it.current()) == BTE_COMMENT) {
      continue;
    }

    // now we're parsing a regular entry
    Data::Entry* entry = new Data::Entry(m_coll);

    text = QString::fromUtf8(bt_entry_type(it.current()));
//    kdDebug() << "entry type: " << text << endl;
    // text is automatically put into lower-case by btparse
    BibtexHandler::setFieldValue(entry, QString::fromLatin1("entry-type"), text);

    text = QString::fromUtf8(bt_entry_key(it.current()));
//    kdDebug() << "entry key: " << text << endl;
    BibtexHandler::setFieldValue(entry, QString::fromLatin1("key"), text);

    char* name;
    AST* field = 0;
    while((field = bt_next_field(it.current(), field, &name))) {
//      kdDebug() << "\tfound: " << name << endl;
//      text = QString::fromLatin1(bt_get_text(field));
      text.truncate(0);
      AST* value = 0;
      bt_nodetype type;
      char* svalue;
      bool end_macro = false;
      while((value = bt_next_value(field, value, &type, &svalue))) {
        switch(type) {
          case BTAST_STRING:
          case BTAST_NUMBER:
            text += BibtexHandler::importText(svalue).simplifyWhiteSpace();
            end_macro = false;
            break;
          case BTAST_MACRO:
            text += QString::fromUtf8(svalue) + QString::fromLatin1("#");
            end_macro = true;
            break;
          default:
            break;
        }
      }
      if(end_macro) {
        // remove last character '#'
        text.truncate(text.length() - 1);
      }
      QString fieldName = QString::fromUtf8(name);
      if(fieldName == Latin1Literal("author") || fieldName == Latin1Literal("editor")) {
        text.replace(QRegExp(QString::fromLatin1("\\sand\\s")), QString::fromLatin1("; "));
      }
      BibtexHandler::setFieldValue(entry, fieldName, text);
    }

    m_coll->addEntry(entry);

    if(j%s_stepSize == 0) {
      emit signalFractionDone(static_cast<float>(j)/static_cast<float>(count));
    }
  }

  // clean-up
  for(ASTListIterator it(m_nodes); it.current(); ++it) {
    bt_free_ast(it.current());
  }

  m_coll->blockSignals(false);
  return m_coll;
}

void BibtexImporter::parseText(const QString& text) {
  if(!m_nodes.isEmpty()) {
    kdWarning() << "BibtexImporter::parseText() - already parsed the text!" << endl;
  }

  ushort bt_options = 0; // ushort is defined in btparse.h
  boolean ok; // boolean is defined in btparse.h as an int

  // for regular nodes (entries), do NOT convert numbers to strings, do NOT expand macros
  bt_set_stringopts(BTE_REGULAR, 0);
  bt_set_stringopts(BTE_MACRODEF, 0);
//  bt_set_stringopts(BTE_PREAMBLE, BTO_CONVERT | BTO_EXPAND);

  QString entry;
  QRegExp rx(QString::fromLatin1("[{}]"));
  QRegExp macroName(QString::fromLatin1("@string\\s*\\{\\s*(.*)="), false /*case sensitive*/);
  macroName.setMinimal(true);

  int brace = 0;
  int startpos = 0;
  int pos = text.find(rx, 0);
  while(pos > 0) {
    if(text[pos] == '{') {
      ++brace;
    } else if(text[pos] == '}' && brace > 0) {
      --brace;
    }
    if(brace == 0) {
      entry = text.mid(startpos, pos-startpos+1).stripWhiteSpace();
      // All the downstream text processing on the AST node will assume utf-8
      AST* node = bt_parse_entry_s(const_cast<char*>(entry.utf8().data()),
                                   const_cast<char*>(url().fileName().local8Bit().data()),
                                   0, bt_options, &ok);
      if(ok && node) {
        if(bt_entry_metatype(node) == BTE_MACRODEF && macroName.search(entry) > -1) {
          char* macro;
          (void) bt_next_field(node, 0, &macro);
          m_macros.insert(QString::fromUtf8(macro), macroName.cap(1).stripWhiteSpace());
        }
        m_nodes.append(node);
      }
      startpos = pos+1;
    }
    pos = text.find(rx, pos+1);
  }
  // clean up some structures
  bt_parse_entry_s(0, 0, 1, 0, 0);
}

#include "bibteximporter.moc"
