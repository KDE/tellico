/***************************************************************************
    Copyright (C) 2005-2022 Robby Stephenson <robby@periapsis.org>
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

#include "gcstarimporter.h"
#include "../collections/videocollection.h"
#include "../utils/string_utils.h"
#include "../utils/datafileregistry.h"
#include "../images/imagefactory.h"
#include "../borrower.h"
#include "../fieldformat.h"
#include "xslthandler.h"
#include "tellicoimporter.h"

#include <KLocalizedString>

#include <QTextStream>

#define CHECKLIMITS(n) if(values.count() <= n) continue

using Tellico::Import::GCstarImporter;

GCstarImporter::GCstarImporter(const QUrl& url_) : TextImporter(url_, true), m_cancelled(false), m_relativeImageLinks(false) {
}

GCstarImporter::GCstarImporter(const QString& text_) : TextImporter(text_), m_cancelled(false), m_relativeImageLinks(false) {
}

bool GCstarImporter::canImport(int type) const {
  return type == Data::Collection::Video
      || type == Data::Collection::Book
      || type == Data::Collection::ComicBook
      || type == Data::Collection::Album
      || type == Data::Collection::Game
      || type == Data::Collection::Wine
      || type == Data::Collection::Coin
      || type == Data::Collection::BoardGame;
}

Tellico::Data::CollPtr GCstarImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  emit signalTotalSteps(this, 100);

  QString str = text();
  QTextStream t(&str);
  QString line = t.readLine();
  if(line.startsWith(QLatin1String("GCfilms"))) {
    readGCfilms(str);
  } else {
    // need to reparse the string if it's in utf-8
    if(line.toLower().indexOf(QLatin1String("utf-8")) > 0) {
      str = QString::fromUtf8(str.toLocal8Bit());
    }
    readGCstar(str);
  }
  return m_coll;
}

void GCstarImporter::readGCfilms(const QString& text_) {
  m_coll = new Data::VideoCollection(true);
  bool hasURL = false;
  if(m_coll->hasField(QStringLiteral("url"))) {
    hasURL = m_coll->fieldByName(QStringLiteral("url"))->type() == Data::Field::URL;
  } else {
    Data::FieldPtr field(new Data::Field(QStringLiteral("url"), i18n("URL"), Data::Field::URL));
    field->setCategory(i18n("General"));
    m_coll->addField(field);
    hasURL = true;
  }

  bool convertUTF8 = false;
  QHash<QString, Data::BorrowerPtr> borrowers;
  static const QRegularExpression rx(QLatin1String("\\s*,\\s*"));
  static const QRegularExpression year(QLatin1String("\\d{4}"));
  static const QRegularExpression runTimeHr(QLatin1String("(\\d+)\\s?hr?"));
  static const QRegularExpression runTimeMin(QLatin1String("(\\d+)\\s?mi?n?"));

  bool gotFirstLine = false;

  QString tmp = text_;
  QTextStream t(&tmp);

  const uint length = text_.length();
  const uint stepSize = qMax(s_stepSize, length/100);
  const bool showProgress = options() & ImportProgress;

  emit signalTotalSteps(this, length);

  uint j = 0;
  for(QString line = t.readLine(); !m_cancelled && !line.isNull(); line = t.readLine(), j += line.length()) {
    // string was wrongly converted
    if(convertUTF8) {
      line = QString::fromUtf8(line.toLocal8Bit());
    }
    QStringList values = line.split(QLatin1Char('|'));
    if(values.empty()) {
      continue;
    }

    if(!gotFirstLine) {
      if(values[0] != QLatin1String("GCfilms")) {
        setStatusMessage(i18n("<qt>The file is not a valid GCstar data file.</qt>"));
        m_coll = nullptr;
        return;
      }
      gotFirstLine = true;
      continue;
    }

    bool ok;

    Data::EntryPtr entry(new Data::Entry(m_coll));
    entry->setId(Tellico::toUInt(values[0], &ok));
    entry->setField(QStringLiteral("title"), values[1]);
    QRegularExpressionMatch yearMatch = year.match(values[2]);
    if(yearMatch.hasMatch()) {
      entry->setField(QStringLiteral("year"), yearMatch.captured());
    }

    uint time = 0;
    QRegularExpressionMatch runTimeMatch = runTimeHr.match(values[3]);
    if(runTimeMatch.hasMatch()) {
      time = Tellico::toUInt(runTimeMatch.captured(1), &ok) * 60;
    }
    runTimeMatch = runTimeMin.match(values[3]);
    if(runTimeMatch.hasMatch()) {
      time += Tellico::toUInt(runTimeMatch.captured(1), &ok);
    }
    if(time > 0) {
      entry->setField(QStringLiteral("running-time"),  QString::number(time));
    }

    entry->setField(QStringLiteral("director"),    splitJoin(rx, values[4]));
    entry->setField(QStringLiteral("nationality"), splitJoin(rx, values[5]));
    entry->setField(QStringLiteral("genre"),       splitJoin(rx, values[6]));
    QUrl u = QUrl(values[7]);
    if(!u.isEmpty()) {
      QString id = ImageFactory::addImage(u, true /* quiet */);
      if(!id.isEmpty()) {
        entry->setField(QStringLiteral("cover"), id);
      }
    }
    entry->setField(QStringLiteral("cast"),  splitJoin(rx, values[8]));
    // values[9] is the original title
    entry->setField(QStringLiteral("plot"),  values[10]);
    if(hasURL) {
      entry->setField(QStringLiteral("url"), values[11]);
    }

    CHECKLIMITS(12);

    // values[12] is whether the film has been viewed or not
    entry->setField(QStringLiteral("medium"), values[13]);
    // values[14] is number of DVDS?
    // values[15] is place?
    // gcfilms's ratings go 0-10, just divide by two
    entry->setField(QStringLiteral("rating"), QString::number(int(Tellico::toUInt(values[16], &ok)/2)));
    entry->setField(QStringLiteral("comments"), values[17]);

    CHECKLIMITS(18);

    QStringList s = values[18].split(QLatin1Char(','));
    QStringList tracks, langs;
    foreach(const QString& value, s) {
      langs << value.section(QLatin1Char(';'), 0, 0);
      tracks << value.section(QLatin1Char(';'), 1, 1);
    }
    entry->setField(QStringLiteral("language"), langs.join(FieldFormat::delimiterString()));
    entry->setField(QStringLiteral("audio-track"), tracks.join(FieldFormat::delimiterString()));

    entry->setField(QStringLiteral("subtitle"), splitJoin(rx, values[19]));

    CHECKLIMITS(20);

    // values[20] is borrower name
    if(!values[20].isEmpty()) {
      QString tmp = values[20];
      Data::BorrowerPtr b = borrowers[tmp];
      if(!b) {
        b = new Data::Borrower(tmp, QString());
        borrowers.insert(tmp, b);
      }
      // values[21] is loan date
      if(!values[21].isEmpty()) {
        tmp = values[21]; // assume date is dd/mm/yyyy
        int d = Tellico::toUInt(tmp.section(QLatin1Char('/'), 0, 0), &ok);
        int m = Tellico::toUInt(tmp.section(QLatin1Char('/'), 1, 1), &ok);
        int y = Tellico::toUInt(tmp.section(QLatin1Char('/'), 2, 2), &ok);
        b->addLoan(Data::LoanPtr(new Data::Loan(entry, QDate(y, m, d), QDate(), QString())));
        entry->setField(QStringLiteral("loaned"), QStringLiteral("true"));
      }
    }
    // values[22] is history ?
    // for certification, only thing we can do is assume default american ratings
    // they're not translated one for one
    CHECKLIMITS(23);

    int age = Tellico::toUInt(values[23], &ok);
    if(age < 2) {
      entry->setField(QStringLiteral("certification"), QStringLiteral("U (USA)"));
    } else if(age < 3) {
      entry->setField(QStringLiteral("certification"), QStringLiteral("G (USA)"));
    } else if(age < 6) {
      entry->setField(QStringLiteral("certification"), QStringLiteral("PG (USA)"));
    } else if(age < 14) {
      entry->setField(QStringLiteral("certification"), QStringLiteral("PG-13 (USA)"));
    } else {
      entry->setField(QStringLiteral("certification"), QStringLiteral("R (USA)"));
    }

    m_coll->addEntries(entry);

    if(showProgress && j%stepSize == 0) {
      emit signalProgress(this, j);
    }
  }

  if(m_cancelled) {
    m_coll = nullptr;
    return;
  }

  for(QHash<QString, Data::BorrowerPtr>::Iterator it = borrowers.begin(); it != borrowers.end(); ++it) {
    if(!it.value()->isEmpty()) {
      m_coll->addBorrower(it.value());
    }
  }
}

void GCstarImporter::readGCstar(const QString& text_) {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("gcstar2tellico.xsl"));
  XSLTHandler handler(QUrl::fromLocalFile(xsltFile));
  if(!handler.isValid()) {
    setStatusMessage(i18n("Tellico encountered an error in XSLT processing."));
    return;
  }

  const QString str = handler.applyStylesheet(text_);

  if(str.isEmpty()) {
    setStatusMessage(i18n("<qt>The file is not a valid GCstar data file.</qt>"));
    return;
  }

  Import::TellicoImporter imp(str);
  if(m_relativeImageLinks) {
    imp.setBaseUrl(url()); /// empty base url is ok
  }
  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());
}

inline
QString GCstarImporter::splitJoin(const QRegularExpression& rx, const QString& s) {
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
  return s.split(rx, QString::SkipEmptyParts).join(FieldFormat::delimiterString());
#else
  return s.split(rx, Qt::SkipEmptyParts).join(FieldFormat::delimiterString());
#endif
}

void GCstarImporter::slotCancel() {
  m_cancelled = true;
}

#undef CHECKLIMITS
