/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
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
#include "../progressmanager.h"
#include "../filehandler.h"
#include "../tellico_debug.h"

#include <kapplication.h>
#include <kconfig.h>

#include <qptrlist.h>
#include <qregexp.h>
#include <qlayout.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>
#include <qtextcodec.h>

using Tellico::Import::BibtexImporter;

BibtexImporter::BibtexImporter(const KURL::List& urls_) : Importer(urls_)
    , m_coll(0), m_widget(0), m_readUTF8(0), m_readLocale(0), m_cancelled(false) {
  bt_initialize();
}

BibtexImporter::BibtexImporter(const QString& text_) : Importer(text_)
    , m_coll(0), m_widget(0), m_readUTF8(0), m_readLocale(0), m_cancelled(false) {
  bt_initialize();
}

BibtexImporter::~BibtexImporter() {
  bt_cleanup();
  if(m_readUTF8) {
    KConfigGroup config(kapp->config(), "Import Options");
    config.writeEntry("Bibtex UTF8", m_readUTF8->isChecked());
  }
}

bool BibtexImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr BibtexImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(urls().count() * 100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  bool useUTF8 = m_widget && m_readUTF8->isChecked();

  m_coll = new Data::BibtexCollection(true);

  int count = 0;
  KURL::List urls = this->urls();
  // might be importing text only
  if(urls.isEmpty()) {
    QString text = this->text();
    Data::CollPtr coll = readCollection(text, count);
    if(!coll || coll->entryCount() == 0) {
      setStatusMessage(i18n("No valid bibtex entries were found"));
    } else {
      m_coll->addEntries(coll->entries());
    }
  } else for(KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it, ++count) {
    if(m_cancelled) {
      return 0;
    }
    if(!(*it).isValid()) {
      continue;
    }
    QString text = FileHandler::readTextFile(*it, false, useUTF8);
    if(text.isEmpty()) {
      continue;
    }
    Data::CollPtr coll = readCollection(text, count);
    if(!coll || coll->entryCount() == 0) {
      setStatusMessage(i18n("No valid bibtex entries were found in file - %1").arg(url().fileName()));
      continue;
    }
    m_coll->addEntries(coll->entries());
  }

  if(m_cancelled) {
    return 0;
  }

  return m_coll;
}

Tellico::Data::CollPtr BibtexImporter::readCollection(const QString& text, int n) {
  if(text.isEmpty()) {
    return 0;
  }
  Data::CollPtr ptr = new Data::BibtexCollection(true);
  Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(ptr.data());

  parseText(text); // populates m_nodes
  if(m_cancelled) {
    return 0;
  }

  if(m_nodes.isEmpty()) {
    return 0;
  }

  QString str;
  const uint count = m_nodes.count();
  const uint stepSize = QMAX(s_stepSize, count/100);
  const bool showProgress = options() & ImportProgress;

  uint j = 0;
  for(ASTListIterator it(m_nodes); !m_cancelled && it.current(); ++it, ++j) {
    // if we're parsing a macro string, comment or preamble, skip it for now
    if(bt_entry_metatype(it.current()) == BTE_PREAMBLE) {
      char* preamble = bt_get_text(it.current());
      if(preamble) {
        c->setPreamble(QString::fromUtf8(preamble));
      }
      continue;
    }

    if(bt_entry_metatype(it.current()) == BTE_MACRODEF) {
      char* macro;
      (void) bt_next_field(it.current(), 0, &macro);
      // FIXME: replace macros within macro definitions!
      // lookup lowercase macro in map
      c->addMacro(m_macros[QString::fromUtf8(macro)], QString::fromUtf8(bt_macro_text(macro, 0, 0)));
      continue;
    }

    if(bt_entry_metatype(it.current()) == BTE_COMMENT) {
      continue;
    }

    // now we're parsing a regular entry
    Data::EntryPtr entry = new Data::Entry(ptr);

    str = QString::fromUtf8(bt_entry_type(it.current()));
//    kdDebug() << "entry type: " << str << endl;
    // text is automatically put into lower-case by btparse
    BibtexHandler::setFieldValue(entry, QString::fromLatin1("entry-type"), str);

    str = QString::fromUtf8(bt_entry_key(it.current()));
//    kdDebug() << "entry key: " << str << endl;
    BibtexHandler::setFieldValue(entry, QString::fromLatin1("key"), str);

    char* name;
    AST* field = 0;
    while((field = bt_next_field(it.current(), field, &name))) {
//      kdDebug() << "\tfound: " << name << endl;
//      str = QString::fromLatin1(bt_get_text(field));
      str.truncate(0);
      AST* value = 0;
      bt_nodetype type;
      char* svalue;
      bool end_macro = false;
      while((value = bt_next_value(field, value, &type, &svalue))) {
        switch(type) {
          case BTAST_STRING:
          case BTAST_NUMBER:
            str += BibtexHandler::importText(svalue).simplifyWhiteSpace();
            end_macro = false;
            break;
          case BTAST_MACRO:
            str += QString::fromUtf8(svalue) + QString::fromLatin1("#");
            end_macro = true;
            break;
          default:
            break;
        }
      }
      if(end_macro) {
        // remove last character '#'
        str.truncate(str.length() - 1);
      }
      QString fieldName = QString::fromUtf8(name);
      if(fieldName == Latin1Literal("author") || fieldName == Latin1Literal("editor")) {
        str.replace(QRegExp(QString::fromLatin1("\\sand\\s")), QString::fromLatin1("; "));
      }
      BibtexHandler::setFieldValue(entry, fieldName, str);
    }

    ptr->addEntries(entry);

    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, n*100 + 100*j/count);
      kapp->processEvents();
    }
  }

  if(m_cancelled) {
    ptr = 0;
  }

  // clean-up
  for(ASTListIterator it(m_nodes); it.current(); ++it) {
    bt_free_ast(it.current());
  }

  return ptr;
}

void BibtexImporter::parseText(const QString& text) {
  m_nodes.clear();
  m_macros.clear();

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

  bool needsCleanup = false;
  int brace = 0;
  int startpos = 0;
  int pos = text.find(rx, 0);
  while(pos > 0 && !m_cancelled) {
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
        needsCleanup = true;
      }
      startpos = pos+1;
    }
    pos = text.find(rx, pos+1);
  }
  if(needsCleanup) {
    // clean up some structures
    bt_parse_entry_s(0, 0, 1, 0, 0);
  }
}

void BibtexImporter::slotCancel() {
  m_cancelled = true;
}

QWidget* BibtexImporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QButtonGroup* box = new QVButtonGroup(i18n("Bibtex Options"), m_widget);
  m_readUTF8 = new QRadioButton(i18n("Use Unicode (UTF-8) encoding"), box);
  QWhatsThis::add(m_readUTF8, i18n("Read the imported file in Unicode (UTF-8)."));
  QString localStr = i18n("Use user locale (%1) encoding").arg(
                     QString::fromLatin1(QTextCodec::codecForLocale()->name()));
  m_readLocale = new QRadioButton(localStr, box);
  m_readLocale->setChecked(true);
  QWhatsThis::add(m_readLocale, i18n("Read the imported file in the local encoding."));

  KConfigGroup config(kapp->config(), "Import Options");
  bool useUTF8 = config.readBoolEntry("Bibtex UTF8", false);
  if(useUTF8) {
    m_readUTF8->setChecked(true);
  } else {
    m_readLocale->setChecked(true);
  }

  l->addWidget(box);
  l->addStretch(1);
  return m_widget;
}


#include "bibteximporter.moc"
