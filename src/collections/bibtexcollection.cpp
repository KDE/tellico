/***************************************************************************
                             bibtexcollection.cpp
                             -------------------
    begin                : Tue Aug 26 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "bibtexcollection.h"

#include <klocale.h>
#include <kdebug.h>

static const char* bibtex_general = I18N_NOOP("General");
static const char* bibtex_publishing = I18N_NOOP("Publishing");
static const char* bibtex_misc = I18N_NOOP("Miscellaneous");

BibtexCollection::BibtexCollection(bool addAttributes_, const QString& title_ /*=null*/)
   : BCCollection(title_, QString::fromLatin1("bibtex"), i18n("Entries")) {
  setTitle(title_.isNull() ? i18n("Bibliography") : title_);
  if(addAttributes_) {
    addAttributes(BibtexCollection::defaultAttributes());
  }
  setDefaultGroupAttribute(QString::fromLatin1("author"));
  setDefaultViewAttributes(QStringList::split(',', QString::fromLatin1("title")));

  // Bibtex has some default macros for the months
  addMacro(QString::fromLatin1("jan"), QString::null);
  addMacro(QString::fromLatin1("feb"), QString::null);
  addMacro(QString::fromLatin1("mar"), QString::null);
  addMacro(QString::fromLatin1("apr"), QString::null);
  addMacro(QString::fromLatin1("may"), QString::null);
  addMacro(QString::fromLatin1("jun"), QString::null);
  addMacro(QString::fromLatin1("jul"), QString::null);
  addMacro(QString::fromLatin1("aug"), QString::null);
  addMacro(QString::fromLatin1("sep"), QString::null);
  addMacro(QString::fromLatin1("oct"), QString::null);
  addMacro(QString::fromLatin1("nov"), QString::null);
  addMacro(QString::fromLatin1("dec"), QString::null);
}

BCAttributeList BibtexCollection::defaultAttributes() {
  BCAttributeList list;
//  list.setAutoDelete(true);
  BibtexAttribute* att;

/******************* General ****************************/
  att = new BibtexAttribute(QString::fromLatin1("title"), i18n("Title"));
  att->setBibtexFieldName(QString::fromLatin1("title"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::NoDelete);
  att->setFormatFlag(BCAttribute::FormatTitle);
  list.append(att);

  QStringList types;
  types << QString::fromLatin1("article") << QString::fromLatin1("book")
        << QString::fromLatin1("booklet") << QString::fromLatin1("inbook")
        << QString::fromLatin1("incollection") << QString::fromLatin1("inproceedings")
        << QString::fromLatin1("manual") << QString::fromLatin1("mastersthesis")
        << QString::fromLatin1("misc") << QString::fromLatin1("phdthesis")
        << QString::fromLatin1("proceedings") << QString::fromLatin1("techreport")
        << QString::fromLatin1("unpublished") << QString::fromLatin1("periodical")
        << QString::fromLatin1("conference");
  att = new BibtexAttribute(QString::fromLatin1("entry-type"), i18n("Entry Type"), types);
  att->setBibtexFieldName(QString::fromLatin1("entry-type"));
  att->setCategory(i18n(bibtex_general));
  att->setFlags(BCAttribute::AllowGrouped | BCAttribute::NoDelete);
  //TODO: finish this
  att->setDescription(i18n("An article is from a journal or magazine. A book has an "
                           "explicit publisher, while a booklet does not."));
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("author"), i18n("Author"));
  att->setBibtexFieldName(QString::fromLatin1("author"));
  att->setCategory(i18n(bibtex_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatName);
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("bibtex-key"), i18n("Bibtex Key"));
  att->setBibtexFieldName(QString::fromLatin1("key"));
  att->setCategory(i18n("General"));
  att->setFlags(BCAttribute::NoDelete);
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("booktitle"), i18n("Book Title"));
  att->setBibtexFieldName(QString::fromLatin1("booktitle"));
  att->setCategory(i18n(bibtex_general));
  att->setFormatFlag(BCAttribute::FormatTitle);
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("editor"), i18n("Editor"));
  att->setBibtexFieldName(QString::fromLatin1("editor"));
  att->setCategory(i18n(bibtex_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatName);
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("organization"), i18n("Organization"));
  att->setBibtexFieldName(QString::fromLatin1("organization"));
  att->setCategory(i18n(bibtex_general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

//  att = new BibtexAttribute(QString::fromLatin1("institution"), i18n("Institution"));
//  att->setBibtexFieldName(QString::fromLatin1("institution"));
//  att->setCategory(i18n(bibtex_general));
//  att->setFlags(BCAttribute::AllowDelete);
//  att->setFormatFlag(BCAttribute::FormatTitle);
//  list.append(att);

/******************* Publishing ****************************/
  att = new BibtexAttribute(QString::fromLatin1("publisher"), i18n("Publisher"));
  att->setBibtexFieldName(QString::fromLatin1("publisher"));
  att->setCategory(i18n(bibtex_publishing));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

//  att = new BibtexAttribute(QString::fromLatin1("address"), i18n("Address"));
//  att->setBibtexFieldName(QString::fromLatin1("address"));
//  att->setCategory(i18n(bibtex_publishing));
//  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
//  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("edition"), i18n("Edition"));
  att->setBibtexFieldName(QString::fromLatin1("edition"));
  att->setCategory(i18n(bibtex_publishing));
  att->setFlags(BCAttribute::AllowCompletion);
  list.append(att);

  // don't make it a nuumber, it could have latex processing commands in it
  att = new BibtexAttribute(QString::fromLatin1("pages"), i18n("Pages"));
  att->setBibtexFieldName(QString::fromLatin1("pages"));
  att->setCategory(i18n(bibtex_publishing));
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("year"), i18n("Year"), BCAttribute::Number);
  att->setBibtexFieldName(QString::fromLatin1("year"));
  att->setCategory(i18n(bibtex_publishing));
  att->setFlags(BCAttribute::AllowGrouped);
  list.append(att);

//  att = new BibtexAttribute(QString::fromLatin1("isbn"), i18n("ISBN#"));
//  att->setBibtexFieldName(QString::fromLatin1("isbn"));
//  att->setCategory(i18n(bibtex_publishing));
//  att->setDescription(i18n("International Standard Book Number"));
//  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("journal"), i18n("Journal"));
  att->setBibtexFieldName(QString::fromLatin1("journal"));
  att->setCategory(i18n(bibtex_publishing));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatPlain);
  list.append(att);

  // could make this a string list, but since bibtex import could have funky values
  // keep it an editbox
  att = new BibtexAttribute(QString::fromLatin1("month"), i18n("Month"),  BCAttribute::Number);
  att->setBibtexFieldName(QString::fromLatin1("month"));
  att->setCategory(i18n(bibtex_publishing));
  att->setFlags(BCAttribute::AllowCompletion);
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("number"), i18n("Number"));
  att->setBibtexFieldName(QString::fromLatin1("number"));
  att->setCategory(i18n(bibtex_publishing));
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("howpublished"), i18n("How Published"));
  att->setBibtexFieldName(QString::fromLatin1("howpublished"));
  att->setCategory(i18n(bibtex_publishing));
  list.append(att);

//  att = new BibtexAttribute(QString::fromLatin1("school"), i18n("School"));
//  att->setBibtexFieldName(QString::fromLatin1("school"));
//  att->setCategory(i18n(bibtex_publishing));
//  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
//  list.append(att);

/******************* Classification ****************************/
  att = new BibtexAttribute(QString::fromLatin1("chapter"), i18n("Chapter"), BCAttribute::Number);
  att->setBibtexFieldName(QString::fromLatin1("chapter"));
  att->setCategory(i18n(bibtex_misc));
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("series"), i18n("Series"));
  att->setBibtexFieldName(QString::fromLatin1("series"));
  att->setCategory(i18n(bibtex_misc));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatTitle);
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("volume"), i18n("Volume"), BCAttribute::Number);
  att->setBibtexFieldName(QString::fromLatin1("volume"));
  att->setCategory(i18n(bibtex_misc));
  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("crossref"), i18n("Cross-Reference"));
  att->setBibtexFieldName(QString::fromLatin1("crossref"));
  att->setCategory(i18n(bibtex_misc));
  list.append(att);

//  att = new BibtexAttribute(QString::fromLatin1("annote"), i18n("Annotation"));
//  att->setBibtexFieldName(QString::fromLatin1("annote"));
//  att->setCategory(i18n(bibtex_misc));
//  list.append(att);

  att = new BibtexAttribute(QString::fromLatin1("note"), i18n("Notes"));
  att->setBibtexFieldName(QString::fromLatin1("note"));
  att->setCategory(i18n(bibtex_misc));
  list.append(att);

  return list;
}

bool BibtexCollection::addAttribute(BCAttribute* att_) {
//  kdDebug() << "BibtexCollection::addAttribute()" << endl;
  bool success = BCCollection::addAttribute(att_);
  if(success && att_->isBibtexAttribute()) {
    BibtexAttribute* batt = dynamic_cast<BibtexAttribute*>(att_);
    if(batt && !batt->bibtexFieldName().isEmpty()) {
      m_bibtexFieldDict.insert(batt->bibtexFieldName(), batt);
    }
  }
  return success;
}

bool BibtexCollection::modifyAttribute(BCAttribute* newAtt_) {
//  kdDebug() << "BibtexCollection::modifyAttribute()" << endl;
  bool success = BCCollection::modifyAttribute(newAtt_);
  BCAttribute* oldAtt = attributeByName(newAtt_->name());
  BibtexAttribute* oldBAtt = dynamic_cast<BibtexAttribute*>(oldAtt);
  BibtexAttribute* newBAtt = dynamic_cast<BibtexAttribute*>(newAtt_);
  if(oldBAtt && newBAtt) {
    success &= m_bibtexFieldDict.remove(oldBAtt->bibtexFieldName());
    oldBAtt->setBibtexFieldName(newBAtt->bibtexFieldName());
    m_bibtexFieldDict.insert(oldBAtt->bibtexFieldName(), oldBAtt);
  }
  return success;
}

bool BibtexCollection::deleteAttribute(BCAttribute* att_, bool force_) {
//  kdDebug() << "BibtexCollection::deleteAttribute()" << endl;
  bool success = true;
  BibtexAttribute* batt = dynamic_cast<BibtexAttribute*>(att_);
  if(batt) {
    success &= m_bibtexFieldDict.remove(batt->bibtexFieldName());
  }
  return success && BCCollection::deleteAttribute(att_, force_);
}

BibtexAttribute* const BibtexCollection::attributeByBibtexField(const QString& field_) const {
  return m_bibtexFieldDict.isEmpty() ? 0 : m_bibtexFieldDict.find(field_);
}

void BibtexCollection::addMacro(const QString& key_, const QString& value_) {
  m_macros.insert(key_, value_);
}

// static
BibtexCollection* BibtexCollection::convertBookCollection(const BCCollection* coll_) {
  if(coll_->collectionType() != BCCollection::Book) {
    kdDebug() << "BibtexCollection::convertBookCollection() - only converts book collections!" << endl;
    return 0;
  }

  BibtexCollection* coll = new BibtexCollection(false, coll_->title());
  BCAttributeListIterator attIt(coll_->attributeList());
  for( ; attIt.current(); ++attIt) {
    BibtexAttribute* att = new BibtexAttribute(*attIt.current());
    QString name = att->name();

    // this first group has bibtex field names the same as their own attribute name
    if(name == QString::fromLatin1("title")
       || name == QString::fromLatin1("author")
       || name == QString::fromLatin1("editor")
       || name == QString::fromLatin1("edition")
       || name == QString::fromLatin1("publisher")
       || name == QString::fromLatin1("isbn")
       || name == QString::fromLatin1("lccn")
       || name == QString::fromLatin1("url")
       || name == QString::fromLatin1("language")
       || name == QString::fromLatin1("pages")
       || name == QString::fromLatin1("series")) {
      att->setBibtexFieldName(name);
    } else if(name == QString::fromLatin1("series_num")) {
      att->setBibtexFieldName(QString::fromLatin1("number"));
    } else if(name == QString::fromLatin1("pur_price")) {
      att->setBibtexFieldName(QString::fromLatin1("price"));
    } else if(name == QString::fromLatin1("cr_year")) {
      att->setBibtexFieldName(QString::fromLatin1("year"));
    } else if(name == QString::fromLatin1("bibtex-id")) {
      att->setBibtexFieldName(QString::fromLatin1("key"));
    } else if(name == QString::fromLatin1("keyword")) {
      att->setBibtexFieldName(QString::fromLatin1("keywords"));
    } else if(name == QString::fromLatin1("comments")) {
      att->setBibtexFieldName(QString::fromLatin1("note"));
    }
    coll->addAttribute(att);
  }

  // also need to add required attributes
  BCAttributeList list = defaultAttributes();
  BCAttribute* cur = list.first();
  BCAttribute* next = 0;
  while(cur) {
    next = list.next();
    if(coll->attributeByName(cur->name()) == 0 && (cur->flags() & BCAttribute::NoDelete)) {
      // but don't add a Bibtex Key if there's already a bibtex-id
      if(static_cast<BibtexAttribute*>(cur)->bibtexFieldName() != QString::fromLatin1("key")
         || coll->attributeByName(QString::fromLatin1("bibtex-id")) == 0) {
        coll->addAttribute(cur);
      }
    } else {
      delete cur;
    }
    cur = next;
  }

  // set the entry-type to book
  BibtexAttribute* att = coll->attributeByBibtexField(QString::fromLatin1("entry-type"));
  QString entryTypeName;
  if(att) {
    entryTypeName = att->name();
  } 

  BCUnitListIterator unitIt(coll_->unitList());
  for( ; unitIt.current(); ++unitIt) {
    BCUnit* unit = new BCUnit(*unitIt.current(), coll);
    coll->addUnit(unit);
    if(!entryTypeName.isEmpty()) {
      unit->setAttribute(entryTypeName, QString::fromLatin1("book"));
    }
  }

  return coll;
}
