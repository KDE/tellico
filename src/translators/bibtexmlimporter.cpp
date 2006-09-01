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

#include "bibtexmlimporter.h"
#include "tellico_xml.h"
#include "bibtexhandler.h"
#include "../collections/bibtexcollection.h"
#include "../field.h"
#include "../entry.h"
#include "../latin1literal.h"
#include "../tellico_strings.h"
#include "../progressmanager.h"
#include "../tellico_debug.h"

#include <kglobal.h> // for KMAX
#include <kapplication.h>

using Tellico::Import::BibtexmlImporter;

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
  if(root.isNull() || root.localName() != Latin1Literal("file")) {
    setStatusMessage(i18n(errorLoad).arg(url().fileName()));
    return;
  }

  const QString& ns = XML::nsBibtexml;
  m_coll = new Data::BibtexCollection(true);

  QDomNodeList entryelems = root.elementsByTagNameNS(ns, QString::fromLatin1("entry"));
//  kdDebug() << "BibtexmlImporter::loadDomDocument - found " << entryelems.count() << " entries" << endl;

  const uint count = entryelems.count();
  const uint stepSize = KMAX(s_stepSize, count/100);
  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(count);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  for(uint j = 0; !m_cancelled && j < entryelems.count(); ++j) {
    readEntry(entryelems.item(j));

    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
  } // end entry loop
}

void BibtexmlImporter::readEntry(const QDomNode& entryNode_) {
  QDomNode node = const_cast<QDomNode&>(entryNode_);

  Data::EntryPtr entry = new Data::Entry(m_coll);

/* The Bibtexml format looks like
  <entry id="...">
   <book>
    <authorlist>
     <author>...</author>
    </authorlist>
    <publisher>...</publisher> */

  QString type = node.firstChild().toElement().tagName();
  entry->setField(QString::fromLatin1("entry-type"), type);
  QString id = node.toElement().attribute(QString::fromLatin1("id"));
  entry->setField(QString::fromLatin1("bibtex-key"), id);

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
      if(parName == Latin1Literal("titlelist")) {
        for(QDomNode n2 = node.firstChild(); !n2.isNull(); n2 = n2.nextSibling()) {
          name = n2.toElement().tagName();
          value = n2.toElement().text();
          if(!name.isEmpty() && !value.isEmpty()) {
            BibtexHandler::setFieldValue(entry, name, value.simplifyWhiteSpace());
          }
        }
        name.truncate(0);
        value.truncate(0);
      } else {
        name = n.firstChild().toElement().tagName();
        if(name == Latin1Literal("keyword")) {
          name = QString::fromLatin1("keywords");
        }
        value.truncate(0);
        for(QDomNode n2 = n.firstChild(); !n2.isNull(); n2 = n2.nextSibling()) {
          // n2 could have first, middle, lastname elements...
          if(name == Latin1Literal("person")) {
            QStringList names;
            names << QString::fromLatin1("initials") << QString::fromLatin1("first")
                  << QString::fromLatin1("middle") << QString::fromLatin1("prelast")
                  << QString::fromLatin1("last") << QString::fromLatin1("lineage");
            for(QStringList::ConstIterator it = names.begin(); it != names.end(); ++it) {
              QDomNodeList list = n2.toElement().elementsByTagName(*it);
              if(list.count() > 1) {
                value += list.item(0).toElement().text();
              }
              if(*it != names.last()) {
                value += QString::fromLatin1(" ");
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
              value += QString::fromLatin1(" ");
            }
          }
          if(n2 != n.lastChild()) {
            value += QString::fromLatin1("; ");
          }
        }
      }
    }
    if(!name.isEmpty() && !value.isEmpty()) {
      BibtexHandler::setFieldValue(entry, name, value.simplifyWhiteSpace());
    }
  }

  m_coll->addEntry(entry);
}

void BibtexmlImporter::slotCancel() {
  m_cancelled = true;
}

#include "bibtexmlimporter.moc"
