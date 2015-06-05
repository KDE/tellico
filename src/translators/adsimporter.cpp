/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "adsimporter.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <QRegExp>
#include <QTextStream>

using Tellico::Import::ADSImporter;
QHash<QString, QString>* ADSImporter::s_tagMap = 0;

// static
void ADSImporter::initTagMap() {
  if(!s_tagMap) {
    s_tagMap = new QHash<QString, QString>();
    s_tagMap->insert(QLatin1String("A"), QLatin1String("author"));
    s_tagMap->insert(QLatin1String("J"), QLatin1String("journal"));
    s_tagMap->insert(QLatin1String("V"), QLatin1String("volume"));
    s_tagMap->insert(QLatin1String("D"), QLatin1String("year"));
    s_tagMap->insert(QLatin1String("T"), QLatin1String("title"));
    s_tagMap->insert(QLatin1String("K"), QLatin1String("keyword"));
    s_tagMap->insert(QLatin1String("Y"), QLatin1String("doi"));
    s_tagMap->insert(QLatin1String("L"), QLatin1String("pages"));
    s_tagMap->insert(QLatin1String("B"), QLatin1String("abstract"));
    s_tagMap->insert(QLatin1String("U"), QLatin1String("url"));
  }
}

ADSImporter::ADSImporter(const QList<QUrl>& urls_) : Tellico::Import::Importer(urls_), m_coll(0), m_cancelled(false) {
  initTagMap();
}

ADSImporter::ADSImporter(const QString& text_) : Tellico::Import::Importer(text_), m_coll(0), m_cancelled(false) {
  initTagMap();
}

bool ADSImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr ADSImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  m_coll = new Data::BibtexCollection(true);
  emit signalTotalSteps(this, urls().count() * 100);

  if(text().isEmpty()) {
    int count = 0;
    foreach(const QUrl& url, urls()) {
      if(m_cancelled)  {
        break;
      }
      readURL(url, count);
      ++count;
    }
  } else {
    readText(text(), 0);
  }

  if(m_cancelled) {
    m_coll = Data::CollPtr();
  }
  return m_coll;
}

void ADSImporter::readURL(const QUrl& url_, int n) {
  QString str = FileHandler::readTextFile(url_);
  if(str.isEmpty()) {
    return;
  }
  readText(str, n);
}

void ADSImporter::readText(const QString& text_, int n) {
  QString text = text_;
  QTextStream t(&text);

  const uint length = text.length();
  const uint stepSize = qMax(s_stepSize, length/100);
  const bool showProgress = options() & ImportProgress;

  bool needToAdd = false;

  QString sp, ep;

  uint j = 0;
  Data::EntryPtr entry(new Data::Entry(m_coll));
  // all ADS entries are journal articles
  entry->setField(QLatin1String("entry-type"), QLatin1String("article"));

  // technically, the spec requires a space immediately after the hyphen
  // however, at least one website (Springer) outputs RIS with no space after the final "ER -"
  // so just strip the white space later
  // also be gracious and allow any amount of space before hyphen
  QRegExp rx(QLatin1String("^\\s*%(\\w)\\s+(.*)$"));
  QString currLine, nextLine;
  for(currLine = t.readLine(); !m_cancelled && !currLine.isNull(); currLine = nextLine, j += currLine.length()) {
    nextLine = t.readLine();
    rx.indexIn(currLine);
    QString tag = rx.cap(1);
    QString value = rx.cap(2).trimmed();
    if(tag.isEmpty()) {
      continue;
    }
//    myDebug() << tag << ":" << value;
    // if the next line is not empty and does not match start regexp, append to value
    while(!nextLine.isEmpty() && rx.indexIn(nextLine) == -1) {
      value += nextLine.trimmed();
      nextLine = t.readLine();
    }

    // every entry begins with "R"
    if(tag == QLatin1String("R")) {
      if(needToAdd) {
        m_coll->addEntries(entry);
      }
      entry = new Data::Entry(m_coll);
      entry->setField(QLatin1String("entry-type"), QLatin1String("article"));
      continue;
    } else if(tag == QLatin1String("P")) {
      sp = value;
      if(!ep.isEmpty()) {
        int startPage = sp.toInt();
        int endPage = ep.toInt();
        if(endPage > 0 && endPage < startPage) {
          myWarning() << "Assuming end page is really page count";
          ep = QString::number(startPage + endPage);
        }
        value = sp + QLatin1Char('-') + ep;
        tag = QLatin1String("L");
        sp.clear();
        ep.clear();
      } else {
        // nothing else to do
        continue;
      }
    } else if(tag == QLatin1String("L")) {
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
    } else if(tag == QLatin1String("D")) {  // for now, just grab the year
      value = value.section(QLatin1Char('/'), 1, 1);
    } else if(tag == QLatin1String("K")) {  // split the keywords
      value = value.split(QLatin1Char(',')).join(FieldFormat::delimiterString());
    } else if(tag == QLatin1String("Y")) {  // clean-up DOI
      value.remove(QRegExp(QLatin1String("^\\s*DOI[\\s:]*"), Qt::CaseInsensitive));
      value = value.section(QLatin1Char(';'), 0, 0);
    } else if(tag == QLatin1String("J")) {  // clean-up journal
      QStringList tokens = value.split(QRegExp(QLatin1String("\\s*,\\s*")));
      if(!tokens.isEmpty()) {
        value = tokens.first();
      }
    }

    // the lookup scheme is:
    // 1. any field has an RIS property that matches the tag name
    // 2. default field mapping tag -> field name
    Data::FieldPtr f = fieldByTag(tag);
    if(!f) {
      continue;
    }
    needToAdd = true;

    // if the field can have multiple values, append current values to new value
    if(f->hasFlag(Data::Field::AllowMultiple) && !entry->field(f->name()).isEmpty()) {
      value.prepend(entry->field(f->name()) + FieldFormat::delimiterString());
    }
    entry->setField(f, value);

    if(showProgress && j%stepSize == 0) {
      emit signalProgress(this, n*100 + 100*j/length);
    }
  }

  if(needToAdd) {
    m_coll->addEntries(entry);
  }
}

Tellico::Data::FieldPtr ADSImporter::fieldByTag(const QString& tag_) {
  Data::FieldPtr f;
  const QString& fieldTag = (*s_tagMap)[tag_];
  if(!fieldTag.isEmpty()) {
    f = m_coll->fieldByName(fieldTag);
    if(!f) {
      myDebug() << "no field found for" << fieldTag;
    }
  }

  // add non-default fields if not already there
  if(tag_== QLatin1String("L1")) {
//    f = new Data::Field(QLatin1String("pdf"), i18n("PDF"), Data::Field::URL);
//    f->setProperty(QLatin1String("ris"), QLatin1String("L1"));
//    f->setCategory(i18n("Miscellaneous"));
  }
//  m_coll->addField(f);
  return f;
}

void ADSImporter::slotCancel() {
  m_cancelled = true;
}

