/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "bibteximporter.h"
#include "../utils/bibtexhandler.h"
#include "../utils/string_utils.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QRegularExpression>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QFile>
#include <QApplication>

using namespace Tellico;
using Tellico::Import::BibtexImporter;

#ifndef ENABLE_BTPARSE
void bt_cleanup() {}
void bt_initialize() {}
#endif

int BibtexImporter::s_initCount = 0;

BibtexImporter::BibtexImporter(const QList<QUrl>& urls_) : Importer(urls_)
    , m_widget(nullptr), m_readUTF8(nullptr), m_readLocale(nullptr), m_cancelled(false) {
  init();
}

BibtexImporter::BibtexImporter(const QString& text_) : Importer(text_)
    , m_widget(nullptr), m_readUTF8(nullptr), m_readLocale(nullptr), m_cancelled(false) {
  init();
}

BibtexImporter::~BibtexImporter() {
  --s_initCount;
  if(s_initCount == 0) {
    bt_cleanup();
  }
  if(m_readUTF8) {
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Import Options"));
    config.writeEntry("Bibtex UTF8", m_readUTF8->isChecked());
  }
}

void BibtexImporter::init() {
  if(s_initCount == 0) {
    bt_initialize();
  }
  ++s_initCount;
}

bool BibtexImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr BibtexImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  Q_EMIT signalTotalSteps(this, urls().count() * 100);

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
      appendCollection(coll);
    }
  }

  foreach(const QUrl& url, urls()) {
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
    appendCollection(coll);
  }

  if(m_cancelled) {
    return Data::CollPtr();
  }

  return m_coll;
}

Tellico::Data::CollPtr BibtexImporter::readCollection(const QString& text, int urlCount) {
#ifdef ENABLE_BTPARSE
  if(text.isEmpty()) {
    myDebug() << "no text";
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

  Data::CollPtr currentColl = currentCollection();
  if(!currentColl || currentColl->type() != Data::Collection::Bibtex) {
    currentColl = ptr;
  }

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
      (void) bt_next_field(node, nullptr, &macro);
      // FIXME: replace macros within macro definitions!
      // lookup lowercase macro in map
      c->addMacro(m_macros[QString::fromUtf8(macro)], QString::fromUtf8(bt_macro_text(macro, nullptr, 0)));
      continue;
    }

    if(bt_entry_metatype(node) == BTE_COMMENT) {
      continue;
    }

    // now we're parsing a regular entry
    Data::EntryPtr entry(new Data::Entry(ptr));

    str = QString::fromUtf8(bt_entry_type(node));
//    myDebug() << "entry type: " << str;
    // text is automatically put into lower-case by btparse
    Data::BibtexCollection::setFieldValue(entry, QStringLiteral("entry-type"), str, currentColl);

    str = QString::fromUtf8(bt_entry_key(node));
//    myDebug() << "entry key: " << str;
    Data::BibtexCollection::setFieldValue(entry, QStringLiteral("key"), str, currentColl);

    static const QRegularExpression andRx(QStringLiteral("\\sand\\s"));
    char* name;
    AST* field = nullptr;
    while((field = bt_next_field(node, field, &name))) {
//      myDebug() << "\tfound: " << name;
//      str = QLatin1String(bt_get_text(field));
      str.clear();
      AST* value = nullptr;
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
            str += QString::fromUtf8(svalue) + QLatin1Char('#');
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
      if(fieldName == QLatin1StringView("author") ||
         fieldName == QLatin1StringView("editor")) {
        str.replace(andRx,FieldFormat::delimiterString());
      }
      // there's a 'key' field different from the citation key
      // https://nwalsh.com/tex/texhelp/bibtx-37.html
      // TODO account for this later
      if(fieldName == QLatin1StringView("key") && !str.isEmpty()) {
        myLog() << "skipping bibtex 'key' field for" << str;
      } else {
        Data::BibtexCollection::setFieldValue(entry, fieldName, str, currentColl);
      }
    }

    ptr->addEntries(entry);

    if(showProgress && j%stepSize == 0) {
      Q_EMIT signalProgress(this, urlCount*100 + 100*j/count);
      qApp->processEvents();
    }
  }

  if(m_cancelled) {
    ptr = nullptr;
  }

  // clean-up
  foreach(AST* node, m_nodes) {
    bt_free_ast(node);
  }

  return ptr;
#else
  return Data::CollPtr();
#endif // ENABLE_BTPARSE
}

void BibtexImporter::parseText(const QString& text) {
#ifdef ENABLE_BTPARSE
  m_nodes.clear();
  m_macros.clear();

  ushort bt_options = 0; // ushort is defined in btparse.h
  boolean ok; // boolean is defined in btparse.h as an int

  // for regular nodes (entries), do NOT convert numbers to strings, do NOT expand macros
  bt_set_stringopts(BTE_REGULAR, 0);
  bt_set_stringopts(BTE_MACRODEF, 0);
//  bt_set_stringopts(BTE_PREAMBLE, BTO_CONVERT | BTO_EXPAND);

  QString entry;
  static const QRegularExpression rx(QStringLiteral("[{}]"));
  static const QRegularExpression macroName(QStringLiteral("@string\\s*\\{\\s*(.*?)="),
                                            QRegularExpression::CaseInsensitiveOption);

  int line = 1;
  bool needsCleanup = false;
  int brace = 0;
  int startpos = 0;
  QRegularExpressionMatch m = rx.match(text);
  int pos = m.capturedStart();
  while(pos > 0 && !m_cancelled) {
    if(text[pos] == QLatin1Char('{')) {
      ++brace;
    } else if(text[pos] == QLatin1Char('}') && brace > 0) {
      --brace;
    }
    if(brace == 0) {
      entry = text.mid(startpos, pos-startpos+1);
      // All the downstream text processing on the AST node will assume utf-8
      QByteArray entryText = entry.toUtf8();
      QByteArray filename = QFile::encodeName(url().fileName());
      AST* node = bt_parse_entry_s(entryText.data(),
                                   filename.data(),
                                   line, bt_options, &ok);
      if(ok && node) {
        QRegularExpressionMatch macroMatch = macroName.match(entry);
        if(bt_entry_metatype(node) == BTE_MACRODEF && macroMatch.hasMatch()) {
          char* macro;
          (void) bt_next_field(node, nullptr, &macro);
          m_macros.insert(QString::fromUtf8(macro), macroMatch.captured(1).trimmed());
        }
        m_nodes.append(node);
        needsCleanup = true;
      }
      startpos = pos+1;
      line += entry.count(QLatin1Char('\n'));
    }
    m = rx.match(text, pos+1);
    pos = m.capturedStart();
  }
  if(needsCleanup) {
    // clean up some structures
    bt_parse_entry_s(nullptr, nullptr, 1, 0, nullptr);
  }
#endif // ENABLE_BTPARSE
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
  const QString localStr = i18n("Use user locale (%1) encoding",
                                QLatin1String(Tellico::localeEncodingName()));
  m_readLocale = new QRadioButton(localStr, gbox);
  m_readLocale->setChecked(true);
  m_readLocale->setWhatsThis(i18n("Read the imported file in the local encoding."));

  vlay->addWidget(m_readUTF8);
  vlay->addWidget(m_readLocale);

  QButtonGroup* bg = new QButtonGroup(gbox);
  bg->addButton(m_readUTF8);
  bg->addButton(m_readLocale);

  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("Import Options"));
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

bool BibtexImporter::maybeBibtex(const QUrl& url_) {
  QString text = FileHandler::readTextFile(url_, true /*quiet*/);
  if(text.isEmpty()) {
    return false;
  }
  return maybeBibtex(text, url_);
}

bool BibtexImporter::maybeBibtex(const QString& text, const QUrl& url_) {
  bool foundOne = false;
#ifdef ENABLE_BTPARSE
  bt_initialize();
  static const QRegularExpression rx(QStringLiteral("[{}]"));

  ushort bt_options = 0; // ushort is defined in btparse.h
  boolean ok; // boolean is defined in btparse.h as an int
  int brace = 0;
  int startpos = 0;
  QRegularExpressionMatch m = rx.match(text);
  int pos = m.capturedStart();
  while(pos > 0) {
    if(text[pos] == QLatin1Char('{')) {
      ++brace;
    } else if(text[pos] == QLatin1Char('}') && brace > 0) {
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
    m = rx.match(text, pos+1);
    pos = m.capturedStart();
  }
  if(foundOne) {
    // clean up some structures
    bt_parse_entry_s(nullptr, nullptr, 1, 0, nullptr);
  }
  bt_cleanup();
#endif // ENABLE_BTPARSE
  return foundOne;
}

void BibtexImporter::appendCollection(Data::CollPtr coll_) {
  Data::BibtexCollection* mainColl = static_cast<Data::BibtexCollection*>(m_coll.data());
  Data::BibtexCollection* newColl = static_cast<Data::BibtexCollection*>(coll_.data());

  foreach(Data::FieldPtr field, coll_->fields()) {
    m_coll->mergeField(field);
  }

  mainColl->addEntries(newColl->entries());
  // append the preamble and macro lists
  if(!newColl->preamble().isEmpty()) {
    QString pre = mainColl->preamble();
    if(!pre.isEmpty()) {
      pre += QLatin1Char('\n');
    }
    mainColl->setPreamble(pre + newColl->preamble());
  }
  StringMap macros = mainColl->macroList();
  macros.insert(newColl->macroList());
  mainColl->setMacroList(macros);
}
