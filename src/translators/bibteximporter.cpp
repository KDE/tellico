/***************************************************************************
    copyright            : (C) 2003-2008 by Robby Stephenson
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
#include "../progressmanager.h"
#include "../filehandler.h"
#include "../tellico_debug.h"

#include <kapplication.h>
#include <KConfigGroup>
#include <kglobal.h>

#include <QRegExp>
#include <QGroupBox>
#include <QRadioButton>
#include <QTextCodec>
#include <QVBoxLayout>
#include <QButtonGroup>

using Tellico::Import::BibtexImporter;

BibtexImporter::BibtexImporter(const KUrl::List& urls_) : Importer(urls_)
    , m_widget(0), m_readUTF8(0), m_readLocale(0), m_cancelled(false) {
  bt_initialize();
}

BibtexImporter::BibtexImporter(const QString& text_) : Importer(text_)
    , m_widget(0), m_readUTF8(0), m_readLocale(0), m_cancelled(false) {
  bt_initialize();
}

BibtexImporter::~BibtexImporter() {
  bt_cleanup();
  if(m_readUTF8) {
    KConfigGroup config(KGlobal::config(), "Import Options");
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
  // might be importing text only
  if(!text().isEmpty()) {
    QString text = this->text();
    Data::CollPtr coll = readCollection(text, count);
    if(!coll || coll->entryCount() == 0) {
      setStatusMessage(i18n("No valid bibtex entries were found"));
    } else {
      m_coll->addEntries(coll->entries());
    }
  }

  const KUrl::List urls = this->urls();
  foreach(const KUrl& url, urls) {
    if(m_cancelled) {
      return Data::CollPtr();
    }
    if(!url.isValid()) {
      continue;
    }
    QString text = FileHandler::readTextFile(url, false, useUTF8);
    if(text.isEmpty()) {
      continue;
    }
    Data::CollPtr coll = readCollection(text, count);
    if(!coll || coll->entryCount() == 0) {
      setStatusMessage(i18n("No valid bibtex entries were found in file - %1", this->url().fileName()));
      continue;
    }
    m_coll->addEntries(coll->entries());
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }

  return m_coll;
}

Tellico::Data::CollPtr BibtexImporter::readCollection(const QString& text, int n) {
  if(text.isEmpty()) {
    myDebug() << "BibtexImporter::readCollection() - no text" << endl;
    return Data::CollPtr();
  }
  Data::CollPtr ptr(new Data::BibtexCollection(true));
  Data::BibtexCollection* c = static_cast<Data::BibtexCollection*>(ptr.data());

  parseText(text); // populates m_nodes
  if(m_cancelled) {
    return Data::CollPtr();
  }

  if(m_nodes.isEmpty()) {
    return Data::CollPtr();
  }

  QString str;
  const uint count = m_nodes.count();
  const uint stepSize = qMax(s_stepSize, count/100);
  const bool showProgress = options() & ImportProgress;

  uint j = 0;
  for(int i = 0; !m_cancelled && i < m_nodes.count(); ++i, ++j) {
    AST* node = m_nodes[i];
    // if we're parsing a macro string, comment or preamble, skip it for now
    if(bt_entry_metatype(node) == BTE_PREAMBLE) {
      char* preamble = bt_get_text(node);
      if(preamble) {
        c->setPreamble(QString::fromUtf8(preamble));
      }
      continue;
    }

    if(bt_entry_metatype(node) == BTE_MACRODEF) {
      char* macro;
      (void) bt_next_field(node, 0, &macro);
      // FIXME: replace macros within macro definitions!
      // lookup lowercase macro in map
      c->addMacro(m_macros[QString::fromUtf8(macro)], QString::fromUtf8(bt_macro_text(macro, 0, 0)));
      continue;
    }

    if(bt_entry_metatype(node) == BTE_COMMENT) {
      continue;
    }

    // now we're parsing a regular entry
    Data::EntryPtr entry(new Data::Entry(ptr));

    str = QString::fromUtf8(bt_entry_type(node));
//    kDebug() << "entry type: " << str;
    // text is automatically put into lower-case by btparse
    BibtexHandler::setFieldValue(entry, QLatin1String("entry-type"), str);

    str = QString::fromUtf8(bt_entry_key(node));
//    kDebug() << "entry key: " << str;
    BibtexHandler::setFieldValue(entry, QLatin1String("key"), str);

    char* name;
    AST* field = 0;
    while((field = bt_next_field(node, field, &name))) {
//      kDebug() << "\tfound: " << name;
//      str = QLatin1String(bt_get_text(field));
      str.clear();
      AST* value = 0;
      bt_nodetype type;
      char* svalue;
      bool end_macro = false;
      while((value = bt_next_value(field, value, &type, &svalue))) {
        switch(type) {
          case BTAST_STRING:
          case BTAST_NUMBER:
            str += BibtexHandler::importText(svalue).simplified();
            end_macro = false;
            break;
          case BTAST_MACRO:
            str += QString::fromUtf8(svalue) + '#';
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
      if(fieldName == QLatin1String("author") || fieldName == QLatin1String("editor")) {
        str.replace(QRegExp(QLatin1String("\\sand\\s")), QLatin1String("; "));
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
  foreach(AST* node, m_nodes) {
    bt_free_ast(node);
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
  QRegExp rx(QLatin1String("[{}]"));
  QRegExp macroName(QLatin1String("@string\\s*\\{\\s*(.*)="), Qt::CaseInsensitive);
  macroName.setMinimal(true);

  bool needsCleanup = false;
  int brace = 0;
  int startpos = 0;
  int pos = rx.indexIn(text, 0);
  while(pos > 0 && !m_cancelled) {
    if(text[pos] == '{') {
      ++brace;
    } else if(text[pos] == '}' && brace > 0) {
      --brace;
    }
    if(brace == 0) {
      entry = text.mid(startpos, pos-startpos+1).trimmed();
      // All the downstream text processing on the AST node will assume utf-8
      AST* node = bt_parse_entry_s(const_cast<char*>(entry.toUtf8().data()),
                                   const_cast<char*>(url().fileName().toLocal8Bit().data()),
                                   0, bt_options, &ok);
      if(ok && node) {
        if(bt_entry_metatype(node) == BTE_MACRODEF && macroName.indexIn(entry) > -1) {
          char* macro;
          (void) bt_next_field(node, 0, &macro);
          m_macros.insert(QString::fromUtf8(macro), macroName.cap(1).trimmed());
        }
        m_nodes.append(node);
        needsCleanup = true;
      }
      startpos = pos+1;
    }
    pos = rx.indexIn(text, pos+1);
  }
  if(needsCleanup) {
    // clean up some structures
    bt_parse_entry_s(0, 0, 1, 0, 0);
  }
}

void BibtexImporter::slotCancel() {
  m_cancelled = true;
}

QWidget* BibtexImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("Bibtex Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_readUTF8 = new QRadioButton(i18n("Use Unicode (UTF-8) encoding"), gbox);
  m_readUTF8->setWhatsThis(i18n("Read the imported file in Unicode (UTF-8)."));
  QString localStr = i18n("Use user locale (%1) encoding",
                          QLatin1String(QTextCodec::codecForLocale()->name()));
  m_readLocale = new QRadioButton(localStr, gbox);
  m_readLocale->setChecked(true);
  m_readLocale->setWhatsThis(i18n("Read the imported file in the local encoding."));

  vlay->addWidget(m_readUTF8);
  vlay->addWidget(m_readLocale);

  QButtonGroup* bg = new QButtonGroup(gbox);
  bg->addButton(m_readUTF8);
  bg->addButton(m_readLocale);

  KConfigGroup config(KGlobal::config(), "Import Options");
  bool useUTF8 = config.readEntry("Bibtex UTF8", false);
  if(useUTF8) {
    m_readUTF8->setChecked(true);
  } else {
    m_readLocale->setChecked(true);
  }

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

bool BibtexImporter::maybeBibtex(const KUrl& url_) {
  QString text = FileHandler::readTextFile(url_, true /*quiet*/);
  if(text.isEmpty()) {
    return false;
  }

  bt_initialize();
  QRegExp rx(QLatin1String("[{}]"));

  ushort bt_options = 0; // ushort is defined in btparse.h
  boolean ok; // boolean is defined in btparse.h as an int
  bool foundOne = false;
  int brace = 0;
  int startpos = 0;
  int pos = rx.indexIn(text, 0);
  while(pos > 0) {
    if(text[pos] == '{') {
      ++brace;
    } else if(text[pos] == '}' && brace > 0) {
      --brace;
    }
    if(brace == 0) {
      QString entry = text.mid(startpos, pos-startpos+1).trimmed();
      // All the downstream text processing on the AST node will assume utf-8
      AST* node = bt_parse_entry_s(const_cast<char*>(entry.toUtf8().data()),
                                   const_cast<char*>(url_.fileName().toLocal8Bit().data()),
                                   0, bt_options, &ok);
      if(ok && node) {
        foundOne = true;
        break;
      }
      startpos = pos+1;
    }
    pos = rx.indexIn(text, pos+1);
  }
  if(foundOne) {
    // clean up some structures
    bt_parse_entry_s(0, 0, 1, 0, 0);
  }
  bt_cleanup();
  return foundOne;
}

#include "bibteximporter.moc"
