/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bibtexexporter.h"
#include "../collection.h"
#include "../collections/bibtexcollection.h"
#include "../latin1literal.h"

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kcombobox.h>

#include <qregexp.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qhbox.h>

using Tellico::Export::BibtexExporter;

BibtexExporter::BibtexExporter(const Data::Collection* coll_) : Tellico::Export::TextExporter(coll_),
   m_expandMacros(false),
   m_packageURL(true),
   m_skipEmptyKeys(false),
   m_widget(0) {
}

QString BibtexExporter::formatString() const {
  return i18n("Bibtex");
}

QString BibtexExporter::fileFilter() const {
  return i18n("*.bib|Bibtex files (*.bib)") + QChar('\n') + i18n("*|All files");
}

QWidget* BibtexExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Horizontal, i18n("Bibtex Options"), m_widget);
  l->addWidget(box);

  m_checkExpandMacros = new QCheckBox(i18n("Expand string macros"), box);
  m_checkExpandMacros->setChecked(m_expandMacros);
  QWhatsThis::add(m_checkExpandMacros, i18n("If checked, the string macros will be expanded and no "
                                            "@string{} entries will be written."));

  m_checkPackageURL = new QCheckBox(i18n("Use URL package"), box);
  m_checkPackageURL->setChecked(m_packageURL);
  QWhatsThis::add(m_checkPackageURL, i18n("If checked, any URL fields will be wrapped in a "
                                          "\\url declaration."));

  m_checkSkipEmpty = new QCheckBox(i18n("Skip entries with empty citation keys"), box);
  m_checkSkipEmpty->setChecked(m_skipEmptyKeys);
  QWhatsThis::add(m_checkSkipEmpty, i18n("If checked, any entries without a bibtex citation key "
                                         "will be skipped."));

  QHBox* hbox = new QHBox(box);
  QLabel* l1 = new QLabel(i18n("Bibtex quotation style:"), hbox);
  m_cbBibtexStyle = new KComboBox(hbox);
  m_cbBibtexStyle->insertItem(i18n("Braces"));
  m_cbBibtexStyle->insertItem(i18n("Quotes"));
  QString whats = i18n("<qt>The quotation style used when exporting bibtex. All field values will "
                       " be escaped with either braces or quotation marks.</qt>");
  QWhatsThis::add(l1, whats);
  QWhatsThis::add(m_cbBibtexStyle, whats);
  if(BibtexHandler::s_quoteStyle == BibtexHandler::BRACES) {
    m_cbBibtexStyle->setCurrentItem(i18n("Braces"));
  } else {
    m_cbBibtexStyle->setCurrentItem(i18n("Quotes"));
  }

  l->addStretch(1);
  return m_widget;
}

void BibtexExporter::readOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_expandMacros = config_->readBoolEntry("Expand Macros", m_expandMacros);
  m_packageURL = config_->readBoolEntry("URL Package", m_packageURL);
  m_skipEmptyKeys = config_->readBoolEntry("Skip Empty Keys", m_skipEmptyKeys);

  if(config_->readBoolEntry("Use Braces", true)) {
    BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
  } else {
    BibtexHandler::s_quoteStyle = BibtexHandler::QUOTES;
  }
}

void BibtexExporter::saveOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_expandMacros = m_checkExpandMacros->isChecked();
  config_->writeEntry("Expand Macros", m_expandMacros);
  m_packageURL = m_checkPackageURL->isChecked();
  config_->writeEntry("URL Package", m_packageURL);
  m_skipEmptyKeys = m_checkSkipEmpty->isChecked();
  config_->writeEntry("Skip Empty Keys", m_skipEmptyKeys);

  bool useBraces = m_cbBibtexStyle->currentText() == i18n("Braces");
  config_->writeEntry("Use Braces", useBraces);
  if(useBraces) {
    BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
  } else {
    BibtexHandler::s_quoteStyle = BibtexHandler::QUOTES;
  }
}

QString BibtexExporter::text(bool formatFields_, bool) {
// there are some special attributes
// the entry-type specifies the entry type - book, inproceedings, whatever
  QString typeField;
// the key specifies the cite-key
  QString keyField;
// keep a list of all the 'ordinary' fields to iterate through later
  Data::FieldList list;
  for(Data::FieldListIterator it(collection()->fieldList()); it.current(); ++it) {
    QString bibtex = it.current()->property(QString::fromLatin1("bibtex"));
    if(bibtex == Latin1Literal("entry-type")) {
      typeField = it.current()->name();
    } else if(bibtex == Latin1Literal("key")) {
      keyField = it.current()->name();
    } else if(!bibtex.isEmpty()) {
      list.append(it.current());
    }
  }

  if(typeField.isEmpty() || keyField.isEmpty()) {
    kdWarning() << "BibtexExporter::text() - the collection must have fields defining "
                   "the entry-type and the key of the entry" << endl;
    return QString::null;
  }
  if(list.isEmpty()) {
    kdWarning() << "BibtexExporter::text() - no bibtex field mapping exists in the collection." << endl;
    return QString::null;
  }

  QString text = QString::fromLatin1("@comment{Generated by Tellico ");
  text += QString::fromLatin1(VERSION);
  text += QString::fromLatin1("}\n");

  const Data::BibtexCollection* c = dynamic_cast<const Data::BibtexCollection*>(collection());
  QStringList macros;
  if(c) {
    QString pre = c->preamble();
    if(!pre.isEmpty()) {
      text += QString::fromLatin1("@preamble{") + pre + QString::fromLatin1("}\n");
    }

    if(!m_expandMacros) {
      macros = c->macroList().keys();
      QMap<QString, QString>::ConstIterator macroIt;
      for(macroIt = c->macroList().begin(); macroIt != c->macroList().end(); ++macroIt) {
        if(!macroIt.data().isEmpty()) {
          text += QString::fromLatin1("@string{")
                  + macroIt.key()
                  + QString::fromLatin1("=")
                  + BibtexHandler::exportText(macroIt.data(), macros)
                  + QString::fromLatin1("}\n");
        }
      }
    }
  }

  // use a dict for fast random access to keep track of the bibtex cite keys
  QDict<int> citeKeyDict;
  Data::FieldListIterator fIt(list);
  for(Data::EntryListIterator entryIt(entryList()); entryIt.current(); ++entryIt) {
    QString type = entryIt.current()->field(typeField);
    if(type.isEmpty()) {
      kdWarning() << "BibtexExporter::text() - the entry for '" << entryIt.current()->title()
                  << "' has no entry-type, skipping it!" << endl;
      continue;
    }

    QString key = entryIt.current()->field(keyField);
    if(key.isEmpty()) {
      if(m_skipEmptyKeys) {
        continue;
      }
      key = BibtexHandler::bibtexKey(entryIt.current());
    }
    QString newKey = key;
    uint c = 'a';
    while(citeKeyDict[newKey]) {
      // duplicate found!
      newKey = key + c;
      ++c;
    }
    key = newKey;
    citeKeyDict.insert(key, reinterpret_cast<const int *>(1));

    text += QString::fromLatin1("@") + type + QString::fromLatin1("{");
    text += key;

    QString value;
    for(fIt.toFirst(); fIt.current(); ++fIt) {
      if(formatFields_) {
        value = entryIt.current()->formattedField(fIt.current()->name());
      } else {
        value = entryIt.current()->field(fIt.current()->name());
      }
      // If the entry is formatted as a name and allow multiple values
      // insert "and" in between them (e.g. author and editor)
      if(fIt.current()->formatFlag() == Data::Field::FormatName
         && fIt.current()->flags() & Data::Field::AllowMultiple) {
        value.replace(Data::Field::delimiter(), QString::fromLatin1(" and "));
      }
      if(!value.isEmpty()) {
        if(m_packageURL && fIt.current()->type() == Data::Field::URL) {
          value = QString::fromLatin1("\\url{") + value + QString::fromLatin1("}");
        } else if(fIt.current()->type() != Data::Field::Number) {
        // numbers aren't escaped, nor will they have macros
          // if m_expandMacros is true, then macros is empty, so this is ok even then
          value = BibtexHandler::exportText(value, macros);
        }
        text += QString::fromLatin1(",\n  ");
        text += fIt.current()->property(QString::fromLatin1("bibtex"));
        text += QString::fromLatin1(" = ");
        text += value;
      }
    }
    text += QString::fromLatin1("\n}\n");
  }
  return text;
}
