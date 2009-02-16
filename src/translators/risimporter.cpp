/***************************************************************************
    copyright            : (C) 2004-2008 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "risimporter.h"
#include "../collections/bibtexcollection.h"
#include "../document.h"
#include "../entry.h"
#include "../field.h"
#include "../progressmanager.h"
#include "../filehandler.h"
#include "../isbnvalidator.h"
#include "../tellico_debug.h"

#include <kapplication.h>

#include <QRegExp>
#include <QTextStream>

using Tellico::Import::RISImporter;
QHash<QString, QString>* RISImporter::s_tagMap = 0;
QHash<QString, QString>* RISImporter::s_typeMap = 0;

// static
void RISImporter::initTagMap() {
  if(!s_tagMap) {
    s_tagMap = new QHash<QString, QString>();
    // BT is special and is handled separately
    s_tagMap->insert(QString::fromLatin1("TY"), QString::fromLatin1("entry-type"));
    s_tagMap->insert(QString::fromLatin1("ID"), QString::fromLatin1("bibtex-key"));
    s_tagMap->insert(QString::fromLatin1("T1"), QString::fromLatin1("title"));
    s_tagMap->insert(QString::fromLatin1("TI"), QString::fromLatin1("title"));
    s_tagMap->insert(QString::fromLatin1("T2"), QString::fromLatin1("booktitle"));
    s_tagMap->insert(QString::fromLatin1("A1"), QString::fromLatin1("author"));
    s_tagMap->insert(QString::fromLatin1("AU"), QString::fromLatin1("author"));
    s_tagMap->insert(QString::fromLatin1("ED"), QString::fromLatin1("editor"));
    s_tagMap->insert(QString::fromLatin1("YR"), QString::fromLatin1("year"));
    s_tagMap->insert(QString::fromLatin1("PY"), QString::fromLatin1("year"));
    s_tagMap->insert(QString::fromLatin1("N1"), QString::fromLatin1("note"));
    s_tagMap->insert(QString::fromLatin1("AB"), QString::fromLatin1("abstract")); // should be note?
    s_tagMap->insert(QString::fromLatin1("N2"), QString::fromLatin1("abstract"));
    s_tagMap->insert(QString::fromLatin1("KW"), QString::fromLatin1("keyword"));
    s_tagMap->insert(QString::fromLatin1("JF"), QString::fromLatin1("journal"));
    s_tagMap->insert(QString::fromLatin1("JO"), QString::fromLatin1("journal"));
    s_tagMap->insert(QString::fromLatin1("JA"), QString::fromLatin1("journal"));
    s_tagMap->insert(QString::fromLatin1("VL"), QString::fromLatin1("volume"));
    s_tagMap->insert(QString::fromLatin1("IS"), QString::fromLatin1("number"));
    s_tagMap->insert(QString::fromLatin1("PB"), QString::fromLatin1("publisher"));
    s_tagMap->insert(QString::fromLatin1("SN"), QString::fromLatin1("isbn"));
    s_tagMap->insert(QString::fromLatin1("AD"), QString::fromLatin1("address"));
    s_tagMap->insert(QString::fromLatin1("CY"), QString::fromLatin1("address"));
    s_tagMap->insert(QString::fromLatin1("UR"), QString::fromLatin1("url"));
    s_tagMap->insert(QString::fromLatin1("L1"), QString::fromLatin1("pdf"));
    s_tagMap->insert(QString::fromLatin1("T3"), QString::fromLatin1("series"));
    s_tagMap->insert(QString::fromLatin1("EP"), QString::fromLatin1("pages"));
  }
}

// static
void RISImporter::initTypeMap() {
  if(!s_typeMap) {
    s_typeMap = new QHash<QString, QString>();
    // leave capitalized, except for bibtex types
    s_typeMap->insert(QString::fromLatin1("ABST"),   QString::fromLatin1("Abstract"));
    s_typeMap->insert(QString::fromLatin1("ADVS"),   QString::fromLatin1("Audiovisual material"));
    s_typeMap->insert(QString::fromLatin1("ART"),    QString::fromLatin1("Art Work"));
    s_typeMap->insert(QString::fromLatin1("BILL"),   QString::fromLatin1("Bill/Resolution"));
    s_typeMap->insert(QString::fromLatin1("BOOK"),   QString::fromLatin1("book")); // bibtex
    s_typeMap->insert(QString::fromLatin1("CASE"),   QString::fromLatin1("Case"));
    s_typeMap->insert(QString::fromLatin1("CHAP"),   QString::fromLatin1("inbook")); // == "inbook" ?
    s_typeMap->insert(QString::fromLatin1("COMP"),   QString::fromLatin1("Computer program"));
    s_typeMap->insert(QString::fromLatin1("CONF"),   QString::fromLatin1("inproceedings")); // == "conference" ?
    s_typeMap->insert(QString::fromLatin1("CTLG"),   QString::fromLatin1("Catalog"));
    s_typeMap->insert(QString::fromLatin1("DATA"),   QString::fromLatin1("Data file"));
    s_typeMap->insert(QString::fromLatin1("ELEC"),   QString::fromLatin1("Electronic Citation"));
    s_typeMap->insert(QString::fromLatin1("GEN"),    QString::fromLatin1("Generic"));
    s_typeMap->insert(QString::fromLatin1("HEAR"),   QString::fromLatin1("Hearing"));
    s_typeMap->insert(QString::fromLatin1("ICOMM"),  QString::fromLatin1("Internet Communication"));
    s_typeMap->insert(QString::fromLatin1("INPR"),   QString::fromLatin1("In Press"));
    s_typeMap->insert(QString::fromLatin1("JFULL"),  QString::fromLatin1("Journal (full)")); // = "periodical" ?
    s_typeMap->insert(QString::fromLatin1("JOUR"),   QString::fromLatin1("article")); // "Journal"
    s_typeMap->insert(QString::fromLatin1("MAP"),    QString::fromLatin1("Map"));
    s_typeMap->insert(QString::fromLatin1("MGZN"),   QString::fromLatin1("article")); // bibtex
    s_typeMap->insert(QString::fromLatin1("MPCT"),   QString::fromLatin1("Motion picture"));
    s_typeMap->insert(QString::fromLatin1("MUSIC"),  QString::fromLatin1("Music score"));
    s_typeMap->insert(QString::fromLatin1("NEWS"),   QString::fromLatin1("Newspaper"));
    s_typeMap->insert(QString::fromLatin1("PAMP"),   QString::fromLatin1("Pamphlet")); // = "booklet" ?
    s_typeMap->insert(QString::fromLatin1("PAT"),    QString::fromLatin1("Patent"));
    s_typeMap->insert(QString::fromLatin1("PCOMM"),  QString::fromLatin1("Personal communication"));
    s_typeMap->insert(QString::fromLatin1("RPRT"),   QString::fromLatin1("Report")); // = "techreport" ?
    s_typeMap->insert(QString::fromLatin1("SER"),    QString::fromLatin1("Serial (BookMonograph)"));
    s_typeMap->insert(QString::fromLatin1("SLIDE"),  QString::fromLatin1("Slide"));
    s_typeMap->insert(QString::fromLatin1("SOUND"),  QString::fromLatin1("Sound recording"));
    s_typeMap->insert(QString::fromLatin1("STAT"),   QString::fromLatin1("Statute"));
    s_typeMap->insert(QString::fromLatin1("THES"),   QString::fromLatin1("phdthesis")); // "mastersthesis" ?
    s_typeMap->insert(QString::fromLatin1("UNBILL"), QString::fromLatin1("Unenacted bill/resolution"));
    s_typeMap->insert(QString::fromLatin1("UNPB"),   QString::fromLatin1("unpublished")); // bibtex
    s_typeMap->insert(QString::fromLatin1("VIDEO"),  QString::fromLatin1("Video recording"));
  }
}

RISImporter::RISImporter(const KUrl::List& urls_) : Tellico::Import::Importer(urls_), m_coll(0), m_cancelled(false) {
  initTagMap();
  initTypeMap();
}

bool RISImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr RISImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  m_coll = new Data::BibtexCollection(true);

  QHash<QString, Data::FieldPtr> risFields;

  // need to know if any extended properties in current collection point to RIS
  // if so, add to collection
  Data::CollPtr currColl = Data::Document::self()->collection();
  foreach(Data::FieldPtr field, currColl->fields()) {
    // continue if property is empty
    QString ris = field->property(QString::fromLatin1("ris"));
    if(ris.isEmpty()) {
      continue;
    }
    // if current collection has one with the same name, set the property
    Data::FieldPtr f = m_coll->fieldByName(field->name());
    if(!f) {
      f = new Data::Field(*field);
      m_coll->addField(f);
    }
    f->setProperty(QString::fromLatin1("ris"), ris);
    risFields.insert(ris, f);
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(urls().count() * 100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  int count = 0;
  KUrl::List urls = this->urls();
  for(KUrl::List::ConstIterator it = urls.constBegin(); it != urls.constEnd() && !m_cancelled; ++it, ++count) {
    readURL(*it, count, risFields);
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
  }
  return m_coll;
}

void RISImporter::readURL(const KUrl& url_, int n, const QHash<QString, Tellico::Data::FieldPtr>& risFields_) {
  QString str = FileHandler::readTextFile(url_);
  if(str.isEmpty()) {
    return;
  }

  ISBNValidator isbnval(this);

  QTextStream t(&str);

  const uint length = str.length();
  const uint stepSize = qMax(s_stepSize, length/100);
  const bool showProgress = options() & ImportProgress;

  bool needToAddFinal = false;

  QString sp, ep;

  uint j = 0;
  Data::EntryPtr entry(new Data::Entry(m_coll));
  // technically, the spec requires a space immediately after the hyphen
  // however, at least one website (Springer) outputs RIS with no space after the final "ER -"
  // so just strip the white space later
  // also be gracious and allow any amount of space before hyphen
  QRegExp rx(QString::fromLatin1("^(\\w\\w)\\s+-(.*)$"));
  QString currLine, nextLine;
  for(currLine = t.readLine(); !m_cancelled && !t.atEnd(); currLine = nextLine, j += currLine.length()) {
    nextLine = t.readLine();
    rx.indexIn(currLine);
    QString tag = rx.cap(1);
    QString value = rx.cap(2).trimmed();
    if(tag.isEmpty()) {
      continue;
    }
//    myDebug() << tag << ": " << value << endl;
    // if the next line is not empty and does not match start regexp, append to value
    while(!nextLine.isEmpty() && nextLine.indexOf(rx) == -1) {
      value += nextLine.trimmed();
      nextLine = t.readLine();
    }

    // every entry ends with "ER"
    if(tag == QLatin1String("ER")) {
      m_coll->addEntries(entry);
      entry = new Data::Entry(m_coll);
      needToAddFinal = false;
      continue;
    } else if(tag == QLatin1String("TY") && s_typeMap->contains(value)) {
      // for entry-type, switch it to normalized type name
      value = (*s_typeMap)[value];
    } else if(tag == QLatin1String("SN")) {
      // test for valid isbn, sometimes the issn gets stuck here
      int pos = 0;
      if(isbnval.validate(value, pos) != ISBNValidator::Acceptable) {
        continue;
      }
    } else if(tag == QLatin1String("SP")) {
      sp = value;
      if(!ep.isEmpty()) {
        value = sp + '-' + ep;
        tag = QString::fromLatin1("EP");
        sp = QString();
        ep = QString();
      } else {
        // nothing else to do
        continue;
      }
    } else if(tag == QLatin1String("EP")) {
      ep = value;
      if(!sp.isEmpty()) {
        value = sp + '-' + ep;
        sp = QString();
        ep = QString();
      } else {
        continue;
      }
    } else if(tag == QLatin1String("YR") || tag == QLatin1String("PY")) {  // for now, just grab the year
      value = value.section('/', 0, 0);
    }

    // the lookup scheme is:
    // 1. any field has an RIS property that matches the tag name
    // 2. default field mapping tag -> field name
    Data::FieldPtr f = risFields_[tag];
    if(!f) {
      // special case for BT
      // primary title for books, secondary for everything else
      if(tag == QLatin1String("BT")) {
        if(entry->field(QString::fromLatin1("entry-type")) == QLatin1String("book")) {
          f = m_coll->fieldByName(QString::fromLatin1("title"));
        } else {
          f = m_coll->fieldByName(QString::fromLatin1("booktitle"));
        }
      } else {
        f = fieldByTag(tag);
      }
    }
    if(!f) {
      continue;
    }
    needToAddFinal = true;

    // harmless for non-choice fields
    // for entry-type, want it in lower case
    f->addAllowed(value);
    // if the field can have multiple values, append current values to new value
    if((f->flags() & Data::Field::AllowMultiple) && !entry->field(f->name()).isEmpty()) {
      value.prepend(entry->field(f->name()) + QString::fromLatin1("; "));
    }
    entry->setField(f, value);

    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, n*100 + 100*j/length);
      kapp->processEvents();
    }
  }

  if(needToAddFinal) {
    m_coll->addEntries(entry);
  }
}

Tellico::Data::FieldPtr RISImporter::fieldByTag(const QString& tag_) {
  Data::FieldPtr f;
  const QString& fieldTag = (*s_tagMap)[tag_];
  if(!fieldTag.isEmpty()) {
    f = m_coll->fieldByName(fieldTag);
    if(f) {
      f->setProperty(QString::fromLatin1("ris"), tag_);
      return f;
    }
  }

  // add non-default fields if not already there
  if(tag_== QLatin1String("L1")) {
    f = new Data::Field(QString::fromLatin1("pdf"), i18n("PDF"), Data::Field::URL);
    f->setProperty(QString::fromLatin1("ris"), QString::fromLatin1("L1"));
    f->setCategory(i18n("Miscellaneous"));
  }
  m_coll->addField(f);
  return f;
}

void RISImporter::slotCancel() {
  m_cancelled = true;
}

bool RISImporter::maybeRIS(const KUrl& url_) {
  QString text = FileHandler::readTextFile(url_, true /*quiet*/);
  if(text.isEmpty()) {
    return false;
  }

  // bare bones check, strip white space at beginning
  // and then first text line must be valid RIS
  QTextStream t(&text);

  QRegExp rx(QString::fromLatin1("^(\\w\\w)\\s+-(.*)$"));
  QString currLine;
  for(currLine = t.readLine(); !t.atEnd(); currLine = t.readLine()) {
    if(currLine.trimmed().isEmpty()) {
      continue;
    }
    break;
  }
  return rx.exactMatch(currLine);
}

#include "risimporter.moc"
