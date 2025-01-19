/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "ciwimporter.h"
#include "../collections/bibtexcollection.h"
#include "../entry.h"
#include "../field.h"
#include "../fieldformat.h"
#include "../core/filehandler.h"
#include "../utils/isbnvalidator.h"
#include "../tellico_debug.h"

#include <QRegularExpression>
#include <QTextStream>
#include <QApplication>

using Tellico::Import::CIWImporter;
QHash<QString, QString>* CIWImporter::s_tagMap = nullptr;

// static
void CIWImporter::initTagMap() {
  if(!s_tagMap) {
    s_tagMap = new QHash<QString, QString>();
    // BT is special and is handled separately
    s_tagMap->insert(QStringLiteral("PT"), QStringLiteral("entry-type"));
    s_tagMap->insert(QStringLiteral("TI"), QStringLiteral("title"));
    s_tagMap->insert(QStringLiteral("BT"), QStringLiteral("booktitle"));
    s_tagMap->insert(QStringLiteral("AU"), QStringLiteral("author"));
    s_tagMap->insert(QStringLiteral("AF"), QStringLiteral("author"));
    s_tagMap->insert(QStringLiteral("BE"), QStringLiteral("editor"));
    s_tagMap->insert(QStringLiteral("PY"), QStringLiteral("year"));
    s_tagMap->insert(QStringLiteral("AB"), QStringLiteral("abstract"));
    s_tagMap->insert(QStringLiteral("DE"), QStringLiteral("keyword"));
    s_tagMap->insert(QStringLiteral("SO"), QStringLiteral("journal"));
    s_tagMap->insert(QStringLiteral("SE"), QStringLiteral("journal"));
    s_tagMap->insert(QStringLiteral("VL"), QStringLiteral("volume"));
    s_tagMap->insert(QStringLiteral("IS"), QStringLiteral("number"));
    s_tagMap->insert(QStringLiteral("PU"), QStringLiteral("publisher"));
    s_tagMap->insert(QStringLiteral("BN"), QStringLiteral("isbn"));
    s_tagMap->insert(QStringLiteral("PA"), QStringLiteral("address"));
    s_tagMap->insert(QStringLiteral("DI"), QStringLiteral("doi"));
    s_tagMap->insert(QStringLiteral("EP"), QStringLiteral("pages"));
  }
}

CIWImporter::CIWImporter(const QList<QUrl>& urls_) : Tellico::Import::Importer(urls_), m_coll(nullptr), m_cancelled(false) {
  initTagMap();
}

CIWImporter::CIWImporter(const QString& text_) : Tellico::Import::Importer(text_), m_coll(nullptr), m_cancelled(false) {
  initTagMap();
}

bool CIWImporter::canImport(int type) const {
  return type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr CIWImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  m_coll = new Data::BibtexCollection(true);

  Q_EMIT signalTotalSteps(this, urls().count() * 100);

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

void CIWImporter::readURL(const QUrl& url_, int n) {
  QString str = FileHandler::readTextFile(url_);
  if(str.isEmpty()) {
    return;
  }
  readText(str, n);
}

void CIWImporter::readText(const QString& text_, int n) {
  ISBNValidator isbnval(this);

  QString text = text_;
  QTextStream t(&text);

  const uint length = text.length();
  const uint stepSize = qMax(s_stepSize, length/100);
  const bool showProgress = options() & ImportProgress;

  bool needToAddFinal = false;
  bool usebooktitle = false;

  QString sp, ep;

  uint j = 0;
  Data::EntryPtr entry(new Data::Entry(m_coll));
  // no idea what the "formal" format is, take it as two characters, followed by a space and then value
  // the entry ends with just ER
  static const QRegularExpression rx(QLatin1String("^(\\w\\w) ?(.*)$"));
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
      // authors and editors get the value separator
      if(tag == QLatin1String("AU") || tag == QLatin1String("AF") || tag == QLatin1String("BE")) {
        value += FieldFormat::delimiterString();
      } else {
        value += QLatin1String(" ");
      }
      value += nextLine.trimmed();
      nextLine = t.readLine();
    }

    // every entry ends with "ER"
    if(tag == QLatin1String("ER")) {
      m_coll->addEntries(entry);
      entry = new Data::Entry(m_coll);
      needToAddFinal = false;
      continue;
    } else if(tag == QLatin1String("PT")) {
      // but the S means that SO is the book title instead of journal name
      if(value == QLatin1String("S")) {
        usebooktitle = true;
      }
      // assume everything is article
      value = QStringLiteral("article");
    } else if(tag == QLatin1String("BN")) {
      // test for valid isbn
      int pos = 0;
      if(isbnval.validate(value, pos) != ISBNValidator::Acceptable) {
        continue;
      }
    } else if(tag == QLatin1String("SO")) {
      if(usebooktitle) {
        tag = QStringLiteral("BT");
      }
    } else if(tag == QLatin1String("BP")) {
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
    }

    Data::FieldPtr f = fieldByTag(tag);
    if(!f) {
      continue;
    }
    needToAddFinal = true;

    // harmless for non-choice fields
    // for entry-type, want it in lower case
    f->addAllowed(value);
    entry->setField(f, value);

    if(showProgress && j%stepSize == 0) {
      Q_EMIT signalProgress(this, n*100 + 100*j/length);
      qApp->processEvents();
    }
  }

  if(needToAddFinal) {
    m_coll->addEntries(entry);
  }
}

Tellico::Data::FieldPtr CIWImporter::fieldByTag(const QString& tag_) {
  const QString& fieldTag = (*s_tagMap)[tag_];
  if(fieldTag.isEmpty()) {
    return Data::FieldPtr();
  }
  return m_coll->fieldByName(fieldTag);
}

void CIWImporter::slotCancel() {
  m_cancelled = true;
}

bool CIWImporter::maybeCIW(const QUrl& url_) {
  QString text = FileHandler::readTextFile(url_, true /*quiet*/);
  if(text.isEmpty()) {
    return false;
  }

  // bare bones check, strip white space at beginning
  // and then first text line must be valid CIW, i.e. two letters followed by a space
  QTextStream t(&text);

  static const QRegularExpression rx(QLatin1String("^(\\w\\w) \\w(.*)$"));
  QString currLine;
  for(currLine = t.readLine(); !t.atEnd(); currLine = t.readLine()) {
    if(currLine.trimmed().isEmpty()) {
      continue;
    }
    break;
  }
  return rx.match(currLine).hasMatch();
}
