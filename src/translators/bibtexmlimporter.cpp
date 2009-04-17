/***************************************************************************
    copyright            : (C) 2003-2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bibtexmlimporter.h"
#include "tellico_xml.h"
#include "../collections/bibtexcollection.h"
#include "../field.h"
#include "../entry.h"
#include "../tellico_strings.h"
#include "../tellico_debug.h"

#include <kapplication.h>

using Tellico::Import::BibtexmlImporter;

BibtexmlImporter::BibtexmlImporter(const KUrl& url) : Import::XMLImporter(url), m_cancelled(false) {
}

bool BibtexmlImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr BibtexmlImporter::collection() {
  if(!m_coll) {
    loadDomDocument();
  }
  return m_coll;
}

void BibtexmlImporter::loadDomDocument() {
  QDomElement root = domDocument().documentElement();
  if(root.isNull() || root.localName() != QLatin1String("file")) {
    setStatusMessage(i18n(errorLoad, url().fileName()));
    return;
  }

  const QString& ns = XML::nsBibtexml;
  m_coll = new Data::BibtexCollection(true);

  QDomNodeList entryelems = root.elementsByTagNameNS(ns, QLatin1String("entry"));
//  myDebug() << "found " << entryelems.count() << " entries";

  const uint count = entryelems.count();
  const uint stepSize = qMax(s_stepSize, count/100);
  const bool showProgress = options() & ImportProgress;

  for(int j = 0; !m_cancelled && j < entryelems.count(); ++j) {
    readEntry(entryelems.item(j));

    if(showProgress && j%stepSize == 0) {
      emit signalProgress(this, 100*j/count);
      kapp->processEvents();
    }
  } // end entry loop
}

void BibtexmlImporter::readEntry(const QDomNode& entryNode_) {
  QDomNode node = const_cast<QDomNode&>(entryNode_);

  Data::EntryPtr entry(new Data::Entry(m_coll));

/* The Bibtexml format looks like
  <entry id="...">
   <book>
    <authorlist>
     <author>...</author>
    </authorlist>
    <publisher>...</publisher> */

  QString type = node.firstChild().toElement().tagName();
  entry->setField(QLatin1String("entry-type"), type);
  QString id = node.toElement().attribute(QLatin1String("id"));
  entry->setField(QLatin1String("bibtex-key"), id);

  QString name, value;
  // field values are first child of first child of entry node
  for(QDomNode n = node.firstChild().firstChild(); !n.isNull(); n = n.nextSibling()) {
    // n could be something like authorlist, with multiple authors, or just
    // a plain element with a single text child...
    // second case first
    if(n.firstChild().isText()) {
      name = n.toElement().tagName();
      value = n.toElement().text();
    } else {
      // is either titlelist, authorlist, editorlist, or keywords
      QString parName = n.toElement().tagName();
      if(parName == QLatin1String("titlelist")) {
        for(QDomNode n2 = node.firstChild(); !n2.isNull(); n2 = n2.nextSibling()) {
          name = n2.toElement().tagName();
          value = n2.toElement().text();
          if(!name.isEmpty() && !value.isEmpty()) {
            Data::BibtexCollection::setFieldValue(entry, name, value.simplified());
          }
        }
        name.clear();
        value.clear();
      } else {
        name = n.firstChild().toElement().tagName();
        if(name == QLatin1String("keyword")) {
          name = QLatin1String("keywords");
        }
        value.clear();
        for(QDomNode n2 = n.firstChild(); !n2.isNull(); n2 = n2.nextSibling()) {
          // n2 could have first, middle, lastname elements...
          if(name == QLatin1String("person")) {
            QStringList names;
            names << QLatin1String("initials") << QLatin1String("first")
                  << QLatin1String("middle") << QLatin1String("prelast")
                  << QLatin1String("last") << QLatin1String("lineage");
            foreach(const QString& name, names) {
              QDomNodeList list = n2.toElement().elementsByTagName(name);
              if(list.count() > 1) {
                value += list.item(0).toElement().text();
              }
              if(name != names.last()) {
                value += QLatin1String(" ");
              }
            }
          }
          for(QDomNode n3 = n2.firstChild(); !n3.isNull(); n3 = n3.nextSibling()) {
            if(n3.isElement()) {
              value += n3.toElement().text();
            } else if(n3.isText()) {
              value += n3.toText().data();
            }
            if(n3 != n2.lastChild()) {
              value += QLatin1String(" ");
            }
          }
          if(n2 != n.lastChild()) {
            value += QLatin1String("; ");
          }
        }
      }
    }
    if(!name.isEmpty() && !value.isEmpty()) {
      Data::BibtexCollection::setFieldValue(entry, name, value.simplified());
    }
  }

  m_coll->addEntries(entry);
}

void BibtexmlImporter::slotCancel() {
  m_cancelled = true;
}

#include "bibtexmlimporter.moc"
