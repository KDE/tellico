/***************************************************************************
                             bookcollection.cpp
                             -------------------
    begin                : Tue Mar 4 2003
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

#include "bookcollection.h"

static const char* general = I18N_NOOP("General");
static const char* publishing = I18N_NOOP("Publishing");
static const char* classification = I18N_NOOP("Classification");
static const char* personal = I18N_NOOP("Personal");

BookCollection::BookCollection(int id_, bool addAttributes_, const QString& title_ /* = i18n("My Books")*/)
   : BCCollection(id_, title_, QString::fromLatin1("book"), i18n("Books")) {
  if(addAttributes_) {
    addDefaultAttributes();
  }
}

void BookCollection::addDefaultAttributes() {
  BCAttribute* att;

  att = new BCAttribute(QString::fromLatin1("subtitle"), i18n("Subtitle"));
  att->setCategory(i18n(general));
  att->setFormatFlag(BCAttribute::FormatTitle);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("author"), i18n("Author"));
  att->setCategory(i18n(general));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  att->setFormatFlag(BCAttribute::FormatName);
  addAttribute(att);
  setDefaultGroupAttribute(att->name());

  QStringList binding;
  binding << i18n("Hardback") << i18n("Paperback") << i18n("Trade Paperback")
          << i18n("E-Book") << i18n("Magazine") << i18n("Journal");
  att = new BCAttribute(QString::fromLatin1("binding"), i18n("Binding"), binding);
  att->setCategory(i18n(general));
  att->setFlags(BCAttribute::AllowGrouped);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("pur_date"), i18n("Purchase Date"));
  att->setCategory(i18n(general));
  att->setFormatFlag(BCAttribute::FormatDate);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("pur_price"), i18n("Purchase Price"));
  att->setCategory(i18n(general));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("publisher"), i18n("Publisher"));
  att->setCategory(i18n(publishing));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("edition"), i18n("Edition"));
  att->setCategory(i18n(publishing));
  att->setFlags(BCAttribute::AllowCompletion);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("cr_year"), i18n("Copyright Year"), BCAttribute::Year);
  att->setCategory(i18n(publishing));
  att->setFlags(BCAttribute::AllowGrouped | BCAttribute::AllowMultiple);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("pub_year"), i18n("Publication Year"),  BCAttribute::Year);
  att->setCategory(i18n(publishing));
  att->setFlags(BCAttribute::AllowGrouped);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("isbn"), i18n("ISBN#"));
  att->setCategory(i18n(publishing));
  att->setDescription(i18n("International Standard Book Number"));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("lccn"), i18n("LCCN#"));
  att->setCategory(i18n(publishing));
  att->setDescription(i18n("Library of Congress Control Number"));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("pages"), i18n("Pages"));
  att->setCategory(i18n(publishing));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("language"), i18n("Language"));
  att->setCategory(i18n(publishing));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped | BCAttribute::AllowMultiple);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("genre"), i18n("Genre"));
  att->setCategory(i18n(classification));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  addAttribute(att);

  // in document versions < 3, this was "keywords" and not "keyword"
  // but the title didn't change, only the name
  att = new BCAttribute(QString::fromLatin1("keyword"), i18n("Keywords"));
  att->setCategory(i18n(classification));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowMultiple | BCAttribute::AllowGrouped);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("series"), i18n("Series"));
  att->setCategory(i18n(classification));
  att->setFlags(BCAttribute::AllowCompletion | BCAttribute::AllowGrouped);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("series_num"), i18n("Series Number"));
  att->setCategory(i18n(classification));
  addAttribute(att);

  QStringList cond;
  cond << i18n("New") << i18n("Used");
  att = new BCAttribute(QString::fromLatin1("condition"), i18n("Condition"), cond);
  att->setCategory(i18n(classification));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("signed"), i18n("Signed"), BCAttribute::Bool);
  att->setCategory(i18n(personal));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("read"), i18n("Read"), BCAttribute::Bool);
  att->setCategory(i18n(personal));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("gift"), i18n("Gift"), BCAttribute::Bool);
  att->setCategory(i18n(personal));
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("loaned"), i18n("Loaned"), BCAttribute::Bool);
  att->setCategory(i18n(personal));
  addAttribute(att);

  QStringList rating;
  rating << i18n("5 - Best") << i18n("4 - Good") << i18n("3 - Neutral") << i18n("2 - Bad") << i18n("1 - Worst");
  att = new BCAttribute(QString::fromLatin1("rating"), i18n("Rating"), rating);
  att->setCategory(i18n(personal));
  att->setFlags(BCAttribute::AllowGrouped);
  addAttribute(att);

  att = new BCAttribute(QString::fromLatin1("comments"), i18n("Comments"));
  att->setCategory(i18n(personal));
  addAttribute(att);

  // have to do this at end since addAttribute() sets it to true;
  m_isCustom = false;
}

QStringList BookCollection::defaultViewAttributes() {
  QStringList names;
  names += QString::fromLatin1("title");
  names += QString::fromLatin1("author");
  names += QString::fromLatin1("genre");
  names += QString::fromLatin1("series");
  names += QString::fromLatin1("comments");
  return names;
}

QStringList BookCollection::defaultPrintAttributes() {
  QStringList names;
  names += QString::fromLatin1("title");
  names += QString::fromLatin1("binding");
  names += QString::fromLatin1("publisher");
  names += QString::fromLatin1("isbn");
  return names;
}

QString BookCollection::defaultTitle() {
  return i18n("My Books");
}

QString BookCollection::defaultUnitTitle() {
  return i18n("Books");
}

