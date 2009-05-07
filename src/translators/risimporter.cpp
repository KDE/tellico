/***************************************************************************
    Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>
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

#include "risimporter.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../core/filehandler.h"
#include "../utils/isbnvalidator.h"
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
    s_tagMap->insert(QLatin1String("TY"), QLatin1String("entry-type"));
    s_tagMap->insert(QLatin1String("ID"), QLatin1String("bibtex-key"));
    s_tagMap->insert(QLatin1String("T1"), QLatin1String("title"));
    s_tagMap->insert(QLatin1String("TI"), QLatin1String("title"));
    s_tagMap->insert(QLatin1String("T2"), QLatin1String("booktitle"));
    s_tagMap->insert(QLatin1String("A1"), QLatin1String("author"));
    s_tagMap->insert(QLatin1String("AU"), QLatin1String("author"));
    s_tagMap->insert(QLatin1String("ED"), QLatin1String("editor"));
    s_tagMap->insert(QLatin1String("YR"), QLatin1String("year"));
    s_tagMap->insert(QLatin1String("PY"), QLatin1String("year"));
    s_tagMap->insert(QLatin1String("N1"), QLatin1String("note"));
    s_tagMap->insert(QLatin1String("AB"), QLatin1String("abstract")); // should be note?
    s_tagMap->insert(QLatin1String("N2"), QLatin1String("abstract"));
    s_tagMap->insert(QLatin1String("KW"), QLatin1String("keyword"));
    s_tagMap->insert(QLatin1String("JF"), QLatin1String("journal"));
    s_tagMap->insert(QLatin1String("JO"), QLatin1String("journal"));
    s_tagMap->insert(QLatin1String("JA"), QLatin1String("journal"));
    s_tagMap->insert(QLatin1String("VL"), QLatin1String("volume"));
    s_tagMap->insert(QLatin1String("IS"), QLatin1String("number"));
    s_tagMap->insert(QLatin1String("PB"), QLatin1String("publisher"));
    s_tagMap->insert(QLatin1String("SN"), QLatin1String("isbn"));
    s_tagMap->insert(QLatin1String("AD"), QLatin1String("address"));
    s_tagMap->insert(QLatin1String("CY"), QLatin1String("address"));
    s_tagMap->insert(QLatin1String("UR"), QLatin1String("url"));
    s_tagMap->insert(QLatin1String("L1"), QLatin1String("pdf"));
    s_tagMap->insert(QLatin1String("T3"), QLatin1String("series"));
    s_tagMap->insert(QLatin1String("EP"), QLatin1String("pages"));
  }
}

// static
void RISImporter::initTypeMap() {
  if(!s_typeMap) {
    s_typeMap = new QHash<QString, QString>();
    // leave capitalized, except for bibtex types
    s_typeMap->insert(QLatin1String("ABST"),   QLatin1String("Abstract"));
    s_typeMap->insert(QLatin1String("ADVS"),   QLatin1String("Audiovisual material"));
    s_typeMap->insert(QLatin1String("ART"),    QLatin1String("Art Work"));
    s_typeMap->insert(QLatin1String("BILL"),   QLatin1String("Bill/Resolution"));
    s_typeMap->insert(QLatin1String("BOOK"),   QLatin1String("book")); // bibtex
    s_typeMap->insert(QLatin1String("CASE"),   QLatin1String("Case"));
    s_typeMap->insert(QLatin1String("CHAP"),   QLatin1String("inbook")); // == "inbook" ?
    s_typeMap->insert(QLatin1String("COMP"),   QLatin1String("Computer program"));
    s_typeMap->insert(QLatin1String("CONF"),   QLatin1String("inproceedings")); // == "conference" ?
    s_typeMap->insert(QLatin1String("CTLG"),   QLatin1String("Catalog"));
    s_typeMap->insert(QLatin1String("DATA"),   QLatin1String("Data file"));
    s_typeMap->insert(QLatin1String("ELEC"),   QLatin1String("Electronic Citation"));
    s_typeMap->insert(QLatin1String("GEN"),    QLatin1String("Generic"));
    s_typeMap->insert(QLatin1String("HEAR"),   QLatin1String("Hearing"));
    s_typeMap->insert(QLatin1String("ICOMM"),  QLatin1String("Internet Communication"));
    s_typeMap->insert(QLatin1String("INPR"),   QLatin1String("In Press"));
    s_typeMap->insert(QLatin1String("JFULL"),  QLatin1String("Journal (full)")); // = "periodical" ?
    s_typeMap->insert(QLatin1String("JOUR"),   QLatin1String("article")); // "Journal"
    s_typeMap->insert(QLatin1String("MAP"),    QLatin1String("Map"));
    s_typeMap->insert(QLatin1String("MGZN"),   QLatin1String("article")); // bibtex
    s_typeMap->insert(QLatin1String("MPCT"),   QLatin1String("Motion picture"));
    s_typeMap->insert(QLatin1String("MUSIC"),  QLatin1String("Music score"));
    s_typeMap->insert(QLatin1String("NEWS"),   QLatin1String("Newspaper"));
    s_typeMap->insert(QLatin1String("PAMP"),   QLatin1String("Pamphlet")); // = "booklet" ?
    s_typeMap->insert(QLatin1String("PAT"),    QLatin1String("Patent"));
    s_typeMap->insert(QLatin1String("PCOMM"),  QLatin1String("Personal communication"));
    s_typeMap->insert(QLatin1String("RPRT"),   QLatin1String("Report")); // = "techreport" ?
    s_typeMap->insert(QLatin1String("SER"),    QLatin1String("Serial (BookMonograph)"));
    s_typeMap->insert(QLatin1String("SLIDE"),  QLatin1String("Slide"));
    s_typeMap->insert(QLatin1String("SOUND"),  QLatin1String("Sound recording"));
    s_typeMap->insert(QLatin1String("STAT"),   QLatin1String("Statute"));
    s_typeMap->insert(QLatin1String("THES"),   QLatin1String("phdthesis")); // "mastersthesis" ?
    s_typeMap->insert(QLatin1String("UNBILL"), QLatin1String("Unenacted bill/resolution"));
    s_typeMap->insert(QLatin1String("UNPB"),   QLatin1String("unpublished")); // bibtex
    s_typeMap->insert(QLatin1String("VIDEO"),  QLatin1String("Video recording"));
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
  Data::CollPtr currColl = currentCollection();
  if(currColl) {
    foreach(Data::FieldPtr field, currColl->fields()) {
      // continue if property is empty
      QString ris = field->property(QLatin1String("ris"));
      if(ris.isEmpty()) {
        continue;
      }
      // if current collection has one with the same name, set the property
      Data::FieldPtr f = m_coll->fieldByName(field->name());
      if(!f) {
        f = new Data::Field(*field);
        m_coll->addField(f);
      }
      f->setProperty(QLatin1String("ris"), ris);
      risFields.insert(ris, f);
    }
  }
  emit signalTotalSteps(this, urls().count() * 100);

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
  QRegExp rx(QLatin1String("^(\\w\\w)\\s+-(.*)$"));
  QString currLine, nextLine;
  for(currLine = t.readLine(); !m_cancelled && !t.atEnd(); currLine = nextLine, j += currLine.length()) {
    nextLine = t.readLine();
    rx.indexIn(currLine);
    QString tag = rx.cap(1);
    QString value = rx.cap(2).trimmed();
    if(tag.isEmpty()) {
      continue;
    }
//    myDebug() << tag << ": " << value;
    // if the next line is not empty and does not match start regexp, append to value
    while(!nextLine.isEmpty() && rx.indexIn(nextLine) == -1) {
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
        int startPage = sp.toInt();
        int endPage = ep.toInt();
        if(endPage > 0 && endPage < startPage) {
          myWarning() << "Assuming end page is really page count";
          ep = QString::number(startPage + endPage);
        }
        value = sp + QLatin1Char('-') + ep;
        tag = QLatin1String("EP");
        sp.clear();
        ep.clear();
      } else {
        // nothing else to do
        continue;
      }
    } else if(tag == QLatin1String("EP")) {
      ep = value;
      if(!sp.isEmpty()) {
        int startPage = sp.toInt();
        int endPage = ep.toInt();
        if(endPage > 0 && endPage < startPage) {
          myWarning() << "Assuming end page is really page count";
          ep = QString::number(startPage + endPage);
        }
        value = sp + QLatin1Char('-') + ep;
        sp.clear();
        ep.clear();
      } else {
        continue;
      }
    } else if(tag == QLatin1String("YR") || tag == QLatin1String("PY")) {  // for now, just grab the year
      value = value.section(QLatin1Char('/'), 0, 0);
    }

    // the lookup scheme is:
    // 1. any field has an RIS property that matches the tag name
    // 2. default field mapping tag -> field name
    Data::FieldPtr f = risFields_[tag];
    if(!f) {
      // special case for BT
      // primary title for books, secondary for everything else
      if(tag == QLatin1String("BT")) {
        if(entry->field(QLatin1String("entry-type")) == QLatin1String("book")) {
          f = m_coll->fieldByName(QLatin1String("title"));
        } else {
          f = m_coll->fieldByName(QLatin1String("booktitle"));
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
      value.prepend(entry->field(f->name()) + QLatin1String("; "));
    }
    entry->setField(f, value);

    if(showProgress && j%stepSize == 0) {
      emit signalProgress(this, n*100 + 100*j/length);
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
      f->setProperty(QLatin1String("ris"), tag_);
      return f;
    }
  }

  // add non-default fields if not already there
  if(tag_== QLatin1String("L1")) {
    f = new Data::Field(QLatin1String("pdf"), i18n("PDF"), Data::Field::URL);
    f->setProperty(QLatin1String("ris"), QLatin1String("L1"));
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

  QRegExp rx(QLatin1String("^(\\w\\w)\\s+-(.*)$"));
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
