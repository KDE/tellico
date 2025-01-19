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
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QRegularExpression>
#include <QTextStream>

using Tellico::Import::RISImporter;
QHash<QString, QString>* RISImporter::s_tagMap = nullptr;
QHash<QString, QString>* RISImporter::s_typeMap = nullptr;

// static
void RISImporter::initTagMap() {
  if(!s_tagMap) {
    s_tagMap = new QHash<QString, QString>();
    // BT is special and is handled separately
    s_tagMap->insert(QStringLiteral("TY"), QStringLiteral("entry-type"));
    s_tagMap->insert(QStringLiteral("ID"), QStringLiteral("bibtex-key"));
    s_tagMap->insert(QStringLiteral("T1"), QStringLiteral("title"));
    s_tagMap->insert(QStringLiteral("TI"), QStringLiteral("title"));
    s_tagMap->insert(QStringLiteral("T2"), QStringLiteral("booktitle"));
    s_tagMap->insert(QStringLiteral("A1"), QStringLiteral("author"));
    s_tagMap->insert(QStringLiteral("AU"), QStringLiteral("author"));
    s_tagMap->insert(QStringLiteral("ED"), QStringLiteral("editor"));
    s_tagMap->insert(QStringLiteral("YR"), QStringLiteral("year"));
    s_tagMap->insert(QStringLiteral("PY"), QStringLiteral("year"));
    s_tagMap->insert(QStringLiteral("Y1"), QStringLiteral("year"));
    s_tagMap->insert(QStringLiteral("N1"), QStringLiteral("note"));
    s_tagMap->insert(QStringLiteral("AB"), QStringLiteral("abstract")); // should be note?
    s_tagMap->insert(QStringLiteral("N2"), QStringLiteral("abstract"));
    s_tagMap->insert(QStringLiteral("KW"), QStringLiteral("keyword"));
    s_tagMap->insert(QStringLiteral("JF"), QStringLiteral("journal"));
    s_tagMap->insert(QStringLiteral("JO"), QStringLiteral("journal"));
    s_tagMap->insert(QStringLiteral("JA"), QStringLiteral("journal"));
    s_tagMap->insert(QStringLiteral("VL"), QStringLiteral("volume"));
    s_tagMap->insert(QStringLiteral("IS"), QStringLiteral("number"));
    s_tagMap->insert(QStringLiteral("PB"), QStringLiteral("publisher"));
    s_tagMap->insert(QStringLiteral("SN"), QStringLiteral("isbn"));
    s_tagMap->insert(QStringLiteral("AD"), QStringLiteral("address"));
    s_tagMap->insert(QStringLiteral("CY"), QStringLiteral("address"));
    s_tagMap->insert(QStringLiteral("UR"), QStringLiteral("url"));
    s_tagMap->insert(QStringLiteral("L1"), QStringLiteral("pdf"));
    s_tagMap->insert(QStringLiteral("T3"), QStringLiteral("series"));
    s_tagMap->insert(QStringLiteral("EP"), QStringLiteral("pages"));
    s_tagMap->insert(QStringLiteral("DO"), QStringLiteral("doi"));
  }
}

// static
void RISImporter::initTypeMap() {
  if(!s_typeMap) {
    s_typeMap = new QHash<QString, QString>();
    // leave capitalized, except for bibtex types
    s_typeMap->insert(QStringLiteral("ABST"),   QStringLiteral("Abstract"));
    s_typeMap->insert(QStringLiteral("ADVS"),   QStringLiteral("Audiovisual material"));
    s_typeMap->insert(QStringLiteral("ART"),    QStringLiteral("Art Work"));
    s_typeMap->insert(QStringLiteral("BILL"),   QStringLiteral("Bill/Resolution"));
    s_typeMap->insert(QStringLiteral("BOOK"),   QStringLiteral("book")); // bibtex
    s_typeMap->insert(QStringLiteral("CASE"),   QStringLiteral("Case"));
    s_typeMap->insert(QStringLiteral("CHAP"),   QStringLiteral("inbook")); // == "inbook" ?
    s_typeMap->insert(QStringLiteral("COMP"),   QStringLiteral("Computer program"));
    s_typeMap->insert(QStringLiteral("CONF"),   QStringLiteral("inproceedings")); // == "conference" ?
    s_typeMap->insert(QStringLiteral("CTLG"),   QStringLiteral("Catalog"));
    s_typeMap->insert(QStringLiteral("DATA"),   QStringLiteral("Data file"));
    s_typeMap->insert(QStringLiteral("ELEC"),   QStringLiteral("Electronic Citation"));
    s_typeMap->insert(QStringLiteral("GEN"),    QStringLiteral("Generic"));
    s_typeMap->insert(QStringLiteral("HEAR"),   QStringLiteral("Hearing"));
    s_typeMap->insert(QStringLiteral("ICOMM"),  QStringLiteral("Internet Communication"));
    s_typeMap->insert(QStringLiteral("INPR"),   QStringLiteral("In Press"));
    s_typeMap->insert(QStringLiteral("JFULL"),  QStringLiteral("Journal (full)")); // = "periodical" ?
    s_typeMap->insert(QStringLiteral("JOUR"),   QStringLiteral("article")); // "Journal"
    s_typeMap->insert(QStringLiteral("MAP"),    QStringLiteral("Map"));
    s_typeMap->insert(QStringLiteral("MGZN"),   QStringLiteral("article")); // bibtex
    s_typeMap->insert(QStringLiteral("MPCT"),   QStringLiteral("Motion picture"));
    s_typeMap->insert(QStringLiteral("MUSIC"),  QStringLiteral("Music score"));
    s_typeMap->insert(QStringLiteral("NEWS"),   QStringLiteral("Newspaper"));
    s_typeMap->insert(QStringLiteral("PAMP"),   QStringLiteral("Pamphlet")); // = "booklet" ?
    s_typeMap->insert(QStringLiteral("PAT"),    QStringLiteral("Patent"));
    s_typeMap->insert(QStringLiteral("PCOMM"),  QStringLiteral("Personal communication"));
    s_typeMap->insert(QStringLiteral("RPRT"),   QStringLiteral("Report")); // = "techreport" ?
    s_typeMap->insert(QStringLiteral("SER"),    QStringLiteral("Serial (BookMonograph)"));
    s_typeMap->insert(QStringLiteral("SLIDE"),  QStringLiteral("Slide"));
    s_typeMap->insert(QStringLiteral("SOUND"),  QStringLiteral("Sound recording"));
    s_typeMap->insert(QStringLiteral("STAT"),   QStringLiteral("Statute"));
    s_typeMap->insert(QStringLiteral("THES"),   QStringLiteral("phdthesis")); // "mastersthesis" ?
    s_typeMap->insert(QStringLiteral("UNBILL"), QStringLiteral("Unenacted bill/resolution"));
    s_typeMap->insert(QStringLiteral("UNPB"),   QStringLiteral("unpublished")); // bibtex
    s_typeMap->insert(QStringLiteral("VIDEO"),  QStringLiteral("Video recording"));
  }
}

RISImporter::RISImporter(const QList<QUrl>& urls_) : Tellico::Import::Importer(urls_), m_coll(nullptr), m_cancelled(false) {
  initTagMap();
  initTypeMap();
}

RISImporter::RISImporter(const QString& text_) : Tellico::Import::Importer(text_), m_coll(nullptr), m_cancelled(false) {
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
      QString ris = field->property(QStringLiteral("ris"));
      if(ris.isEmpty()) {
        continue;
      }
      // if current collection has one with the same name, set the property
      Data::FieldPtr f = m_coll->fieldByName(field->name());
      if(!f) {
        f = new Data::Field(*field);
        m_coll->addField(f);
      }
      f->setProperty(QStringLiteral("ris"), ris);
      risFields.insert(ris, f);
    }
  }
  Q_EMIT signalTotalSteps(this, urls().count() * 100);

  if(text().isEmpty()) {
    int count = 0;
    foreach(const QUrl& url, urls()) {
      if(m_cancelled)  {
        break;
      }
      readURL(url, count, risFields);
      ++count;
    }
  } else {
    readText(text(), 0, risFields);
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
  }
  return m_coll;
}

void RISImporter::readURL(const QUrl& url_, int n, const QHash<QString, Tellico::Data::FieldPtr>& risFields_) {
  QString str = FileHandler::readTextFile(url_);
  if(str.isEmpty()) {
    return;
  }
  readText(str, n, risFields_);
}

void RISImporter::readText(const QString& text_, int n, const QHash<QString, Tellico::Data::FieldPtr>& risFields_) {
  ISBNValidator isbnval(this);

  QString text = text_;
  QTextStream t(&text);

  const uint length = text.length();
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
  static const QRegularExpression rx(QLatin1String("^(\\w\\w)\\s+-(.*)$"));
  QString currLine, nextLine;
  for(currLine = t.readLine(); !m_cancelled && !t.atEnd(); currLine = nextLine, j += currLine.length()) {
    nextLine = t.readLine();
    QRegularExpressionMatch m = rx.match(currLine);
    QString tag = m.captured(1);
    QString value = m.captured(2).trimmed();
    if(tag.isEmpty()) {
      continue;
    }
//    myDebug() << tag << ": " << value;
    // if the next line is not empty and does not match start regexp, append to value
    while(!nextLine.isEmpty() && !rx.match(nextLine).hasMatch()) {
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
        tag = QStringLiteral("EP");
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
    } else if(s_tagMap->value(tag) == QLatin1String("year")) {  // for now, just grab the year
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
        if(entry->field(QStringLiteral("entry-type")) == QLatin1String("book")) {
          f = m_coll->fieldByName(QStringLiteral("title"));
        } else {
          f = m_coll->fieldByName(QStringLiteral("booktitle"));
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
    if(f->hasFlag(Data::Field::AllowMultiple) && !entry->field(f).isEmpty()) {
      value.prepend(entry->field(f) + FieldFormat::delimiterString());
    }
    entry->setField(f, value);

    if(showProgress && j%stepSize == 0) {
      Q_EMIT signalProgress(this, n*100 + 100*j/length);
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
      f->setProperty(QStringLiteral("ris"), tag_);
      return f;
    }
  }

  // add non-default fields if not already there
  if(tag_== QLatin1String("L1")) {
    f = new Data::Field(QStringLiteral("pdf"), i18n("PDF"), Data::Field::URL);
    f->setProperty(QStringLiteral("ris"), QStringLiteral("L1"));
    f->setCategory(i18n("Miscellaneous"));
  }
  m_coll->addField(f);
  return f;
}

void RISImporter::slotCancel() {
  m_cancelled = true;
}

bool RISImporter::maybeRIS(const QUrl& url_) {
  QString text = FileHandler::readTextFile(url_, true /*quiet*/);
  if(text.isEmpty()) {
    return false;
  }

  // bare bones check, strip white space at beginning
  // and then first text line must be valid RIS
  QTextStream t(&text);

  static const QRegularExpression rx(QLatin1String("^(\\w\\w)\\s+-(.*)$"));
  QString currLine;
  for(currLine = t.readLine(); !t.atEnd(); currLine = t.readLine()) {
    if(currLine.trimmed().isEmpty()) {
      continue;
    }
    break;
  }
  return rx.match(currLine).hasMatch();
}
