/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "gcfilmsimporter.h"
#include "../collections/videocollection.h"
#include "../latin1literal.h"
#include "../tellico_utils.h"
#include "../imagefactory.h"
#include "../borrower.h"
#include "../progressmanager.h"

#include <kglobal.h> // for KMAX
#include <kapplication.h>

#include <qtextcodec.h>

#define CHECKLIMITS(n) if(values.count() <= n) continue

using Tellico::Import::GCfilmsImporter;

GCfilmsImporter::GCfilmsImporter(const KURL& url_) : TextImporter(url_), m_coll(0), m_cancelled(false) {
}

bool GCfilmsImporter::canImport(int type) const {
  return type == Data::Collection::Video;
}

Tellico::Data::CollPtr GCfilmsImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  m_coll = new Data::VideoCollection(true);
  bool hasURL = false;
  if(m_coll->hasField(QString::fromLatin1("url"))) {
    hasURL = m_coll->fieldByName(QString::fromLatin1("url"))->type() == Data::Field::URL;
  } else {
    Data::FieldPtr field = new Data::Field(QString::fromLatin1("url"), i18n("URL"), Data::Field::URL);
    field->setCategory(i18n("General"));
    m_coll->addField(field);
    hasURL = true;
  }

  bool convertUTF8 = false;
  QMap<QString, Data::BorrowerPtr> borrowers;
  const QRegExp rx(QString::fromLatin1("\\s*,\\s*"));
  QRegExp year(QString::fromLatin1("\\d{4}"));
  QRegExp runTimeHr(QString::fromLatin1("(\\d+)\\s?hr?"));
  QRegExp runTimeMin(QString::fromLatin1("(\\d+)\\s?mi?n?"));

  bool gotFirstLine = false;
  uint total = 0;

  QString str = text();
  QTextIStream t(&str);

  const uint length = str.length();
  const uint stepSize = KMAX(s_stepSize, length/100);
  item.setTotalSteps(length);
  uint j = 0;
  for(QString line = t.readLine(); !m_cancelled && !line.isNull(); line = t.readLine(), j += line.length()) {
    // string was wrongly converted
    QStringList values = QStringList::split('|', (convertUTF8 ? QString::fromUtf8(line.local8Bit()) : line), true);
    if(values.empty()) {
      continue;
    }

    if(!gotFirstLine) {
      if(values[0] != Latin1Literal("GCfilms")) {
        setStatusMessage(i18n("<qt>The file is not a valid GCfilms data file.</qt>"));
        m_coll = 0;
        return 0;
      }
      total = Tellico::toUInt(values[1], 0)+1; // number of lines really
      if(values.size() > 2 && values[2] == Latin1Literal("UTF8")) {
        // if locale encoding isn't utf8, need to do a reconversion
        QTextCodec* codec = QTextCodec::codecForLocale();
        if(QCString(codec->name()).find("utf-8", 0, false) == -1) {
          convertUTF8 = true;
        }
      }
      gotFirstLine = true;
      continue;
    }

    bool ok;

    Data::EntryPtr entry = new Data::Entry(m_coll);
    entry->setId(Tellico::toUInt(values[0], &ok));
    entry->setField(QString::fromLatin1("title"), values[1]);
    if(year.search(values[2]) > -1) {
      entry->setField(QString::fromLatin1("year"), year.cap());
    }

    uint time = 0;
    if(runTimeHr.search(values[3]) > -1) {
      time = Tellico::toUInt(runTimeHr.cap(1), &ok) * 60;
    }
    if(runTimeMin.search(values[3]) > -1) {
      time += Tellico::toUInt(runTimeMin.cap(1), &ok);
    }
    if(time > 0) {
      entry->setField(QString::fromLatin1("running-time"),  QString::number(time));
    }

    entry->setField(QString::fromLatin1("director"),      splitJoin(rx, values[4]));
    entry->setField(QString::fromLatin1("nationality"),   splitJoin(rx, values[5]));
    entry->setField(QString::fromLatin1("genre"),         splitJoin(rx, values[6]));
    KURL u = KURL::fromPathOrURL(values[7]);
    if(!u.isEmpty()) {
      QString id = ImageFactory::addImage(u, true /* quiet */);
      if(!id.isEmpty()) {
        entry->setField(QString::fromLatin1("cover"), id);
      }
    }
    entry->setField(QString::fromLatin1("cast"),  splitJoin(rx, values[8]));
    // values[9] is the original title
    entry->setField(QString::fromLatin1("plot"),  values[10]);
    if(hasURL) {
      entry->setField(QString::fromLatin1("url"), values[11]);
    }

    CHECKLIMITS(12);

    // values[12] is whether the film has been viewed or not
    entry->setField(QString::fromLatin1("medium"), values[13]);
    // values[14] is number of DVDS?
    // values[15] is place?
    // gcfilms's ratings go 0-10, just divide by two
    entry->setField(QString::fromLatin1("rating"), QString::number(int(Tellico::toUInt(values[16], &ok)/2)));
    entry->setField(QString::fromLatin1("comments"), values[17]);

    CHECKLIMITS(18);

    QStringList s = QStringList::split(',', values[18]);
    QStringList tracks, langs;
    for(QStringList::ConstIterator it = s.begin(); it != s.end(); ++it) {
      langs << (*it).section(';', 0, 0);
      tracks << (*it).section(';', 1, 1);
    }
    entry->setField(QString::fromLatin1("language"),    langs.join(QString::fromLatin1("; ")));
    entry->setField(QString::fromLatin1("audio-track"), tracks.join(QString::fromLatin1("; ")));

    entry->setField(QString::fromLatin1("subtitle"), splitJoin(rx, values[19]));

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
        int d = Tellico::toUInt(tmp.section('/', 0, 0), &ok);
        int m = Tellico::toUInt(tmp.section('/', 1, 1), &ok);
        int y = Tellico::toUInt(tmp.section('/', 2, 2), &ok);
        b->addLoan(new Data::Loan(entry, QDate(y, m, d), QDate(), QString()));
        entry->setField(QString::fromLatin1("loaned"), QString::fromLatin1("true"));
      }
    }
    // values[22] is history ?
    // for certification, only thing we can do is assume default american ratings
    // they're not translated one for one
    CHECKLIMITS(23);

    int age = Tellico::toUInt(values[23], &ok);
    if(age < 2) {
      entry->setField(QString::fromLatin1("certification"), QString::fromLatin1("U (USA)"));
    } else if(age < 3) {
      entry->setField(QString::fromLatin1("certification"), QString::fromLatin1("G (USA)"));
    } else if(age < 6) {
      entry->setField(QString::fromLatin1("certification"), QString::fromLatin1("PG (USA)"));
    } else if(age < 14) {
      entry->setField(QString::fromLatin1("certification"), QString::fromLatin1("PG-13 (USA)"));
    } else {
      entry->setField(QString::fromLatin1("certification"), QString::fromLatin1("R (USA)"));
    }

    m_coll->addEntries(entry);

    if(j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
  }

  if(m_cancelled) {
    m_coll = 0;
    return 0;
  }

  for(QMap<QString, Data::BorrowerPtr>::Iterator it = borrowers.begin(); it != borrowers.end(); ++it) {
    if(!it.data()->isEmpty()) {
      m_coll->addBorrower(it.data());
    }
  }

  return m_coll;
}

inline
QString GCfilmsImporter::splitJoin(const QRegExp& rx, const QString& s) {
  return QStringList::split(rx, s, false).join(QString::fromLatin1("; "));
}

void GCfilmsImporter::slotCancel() {
  m_cancelled = true;
}

#undef CHECKLIMITS
#include "gcfilmsimporter.moc"
