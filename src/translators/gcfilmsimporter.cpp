/***************************************************************************
    copyright            : (C) 2005-2008 by Robby Stephenson
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
#include "../tellico_utils.h"
#include "../imagefactory.h"
#include "../borrower.h"
#include "../progressmanager.h"
#include "xslthandler.h"
#include "tellicoimporter.h"

#include <kapplication.h>
#include <kstandarddirs.h>

#include <QTextCodec>
#include <QTextStream>

#define CHECKLIMITS(n) if(values.count() <= n) continue

using Tellico::Import::GCfilmsImporter;

GCfilmsImporter::GCfilmsImporter(const KUrl& url_) : TextImporter(url_), m_cancelled(false) {
}

bool GCfilmsImporter::canImport(int type) const {
  return type == Data::Collection::Video
      || type == Data::Collection::Book
      || type == Data::Collection::Album
      || type == Data::Collection::Game
      || type == Data::Collection::Wine
      || type == Data::Collection::Coin;
}

Tellico::Data::CollPtr GCfilmsImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(100);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

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

void GCfilmsImporter::readGCfilms(const QString& text_) {
  m_coll = new Data::VideoCollection(true);
  bool hasURL = false;
  if(m_coll->hasField(QLatin1String("url"))) {
    hasURL = m_coll->fieldByName(QLatin1String("url"))->type() == Data::Field::URL;
  } else {
    Data::FieldPtr field(new Data::Field(QLatin1String("url"), i18n("URL"), Data::Field::URL));
    field->setCategory(i18n("General"));
    m_coll->addField(field);
    hasURL = true;
  }

  bool convertUTF8 = false;
  QMap<QString, Data::BorrowerPtr> borrowers;
  const QRegExp rx(QLatin1String("\\s*,\\s*"));
  QRegExp year(QLatin1String("\\d{4}"));
  QRegExp runTimeHr(QLatin1String("(\\d+)\\s?hr?"));
  QRegExp runTimeMin(QLatin1String("(\\d+)\\s?mi?n?"));

  bool gotFirstLine = false;
  uint total = 0;

  QString tmp = text_;
  QTextStream t(&tmp);

  const uint length = text_.length();
  const uint stepSize = qMax(s_stepSize, length/100);
  const bool showProgress = options() & ImportProgress;

  ProgressManager::self()->setTotalSteps(this, length);
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
        m_coll = 0;
        return;
      }
      total = Tellico::toUInt(values[1], 0)+1; // number of lines really
      if(values.size() > 2 && values[2] == QLatin1String("UTF8")) {
        // if locale encoding isn't utf8, need to do a reconversion
        QTextCodec* codec = QTextCodec::codecForLocale();
        if(codec->name().toLower().indexOf("utf-8") == -1) {
          convertUTF8 = true;
        }
      }
      gotFirstLine = true;
      continue;
    }

    bool ok;

    Data::EntryPtr entry(new Data::Entry(m_coll));
    entry->setId(Tellico::toUInt(values[0], &ok));
    entry->setField(QLatin1String("title"), values[1]);
    if(year.indexIn(values[2]) > -1) {
      entry->setField(QLatin1String("year"), year.cap());
    }

    uint time = 0;
    if(runTimeHr.indexIn(values[3]) > -1) {
      time = Tellico::toUInt(runTimeHr.cap(1), &ok) * 60;
    }
    if(runTimeMin.indexIn(values[3]) > -1) {
      time += Tellico::toUInt(runTimeMin.cap(1), &ok);
    }
    if(time > 0) {
      entry->setField(QLatin1String("running-time"),  QString::number(time));
    }

    entry->setField(QLatin1String("director"),      splitJoin(rx, values[4]));
    entry->setField(QLatin1String("nationality"),   splitJoin(rx, values[5]));
    entry->setField(QLatin1String("genre"),         splitJoin(rx, values[6]));
    KUrl u = KUrl(values[7]);
    if(!u.isEmpty()) {
      QString id = ImageFactory::addImage(u, true /* quiet */);
      if(!id.isEmpty()) {
        entry->setField(QLatin1String("cover"), id);
      }
    }
    entry->setField(QLatin1String("cast"),  splitJoin(rx, values[8]));
    // values[9] is the original title
    entry->setField(QLatin1String("plot"),  values[10]);
    if(hasURL) {
      entry->setField(QLatin1String("url"), values[11]);
    }

    CHECKLIMITS(12);

    // values[12] is whether the film has been viewed or not
    entry->setField(QLatin1String("medium"), values[13]);
    // values[14] is number of DVDS?
    // values[15] is place?
    // gcfilms's ratings go 0-10, just divide by two
    entry->setField(QLatin1String("rating"), QString::number(int(Tellico::toUInt(values[16], &ok)/2)));
    entry->setField(QLatin1String("comments"), values[17]);

    CHECKLIMITS(18);

    QStringList s = values[18].split(QLatin1Char(','));
    QStringList tracks, langs;
    foreach(const QString& value, s) {
      langs << value.section(QLatin1Char(';'), 0, 0);
      tracks << value.section(QLatin1Char(';'), 1, 1);
    }
    entry->setField(QLatin1String("language"),    langs.join(QLatin1String("; ")));
    entry->setField(QLatin1String("audio-track"), tracks.join(QLatin1String("; ")));

    entry->setField(QLatin1String("subtitle"), splitJoin(rx, values[19]));

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
        entry->setField(QLatin1String("loaned"), QLatin1String("true"));
      }
    }
    // values[22] is history ?
    // for certification, only thing we can do is assume default american ratings
    // they're not translated one for one
    CHECKLIMITS(23);

    int age = Tellico::toUInt(values[23], &ok);
    if(age < 2) {
      entry->setField(QLatin1String("certification"), QLatin1String("U (USA)"));
    } else if(age < 3) {
      entry->setField(QLatin1String("certification"), QLatin1String("G (USA)"));
    } else if(age < 6) {
      entry->setField(QLatin1String("certification"), QLatin1String("PG (USA)"));
    } else if(age < 14) {
      entry->setField(QLatin1String("certification"), QLatin1String("PG-13 (USA)"));
    } else {
      entry->setField(QLatin1String("certification"), QLatin1String("R (USA)"));
    }

    m_coll->addEntries(entry);

    if(showProgress && j%stepSize == 0) {
      ProgressManager::self()->setProgress(this, j);
      kapp->processEvents();
    }
  }

  if(m_cancelled) {
    m_coll = 0;
    return;
  }

  for(QMap<QString, Data::BorrowerPtr>::Iterator it = borrowers.begin(); it != borrowers.end(); ++it) {
    if(!it.value()->isEmpty()) {
      m_coll->addBorrower(it.value());
    }
  }
}

void GCfilmsImporter::readGCstar(const QString& text_) {
  QString xsltFile = KStandardDirs::locate("appdata", QLatin1String("gcstar2tellico.xsl"));
  XSLTHandler handler(xsltFile);
  if(!handler.isValid()) {
    setStatusMessage(i18n("Tellico encountered an error in XSLT processing."));
    return;
  }

  QString str = handler.applyStylesheet(text_);

  if(str.isEmpty()) {
    setStatusMessage(i18n("<qt>The file is not a valid GCstar data file.</qt>"));
    return;
  }

  Import::TellicoImporter imp(str);
  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());
}

inline
QString GCfilmsImporter::splitJoin(const QRegExp& rx, const QString& s) {
  return s.split(rx, QString::SkipEmptyParts).join(QLatin1String("; "));
}

void GCfilmsImporter::slotCancel() {
  m_cancelled = true;
}

#undef CHECKLIMITS
#include "gcfilmsimporter.moc"
