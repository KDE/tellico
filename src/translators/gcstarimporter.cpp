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
#include "../tellico_debug.h"

#include <KLocalizedString>

#include <QTextStream>
#include <QFileInfo>
#include <QStandardPaths>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>

#define CHECKLIMITS(n) if(values.count() <= n) continue

using Tellico::Import::GCstarImporter;

GCstarImporter::GCstarImporter(const QUrl& url_) : TextImporter(url_, true)
    , m_cancelled(false)
    , m_imageLinksOnly(false)
    , m_widget(nullptr)
    , m_cbImageLink(nullptr) {
}

GCstarImporter::GCstarImporter(const QString& text_) : TextImporter(text_)
    , m_cancelled(false)
    , m_imageLinksOnly(false)
    , m_widget(nullptr)
    , m_cbImageLink(nullptr) {
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

void GCstarImporter::setImagePathsAsLinks(bool imagePathsAsLinks_) {
  m_imageLinksOnly = imagePathsAsLinks_;
}

Tellico::Data::CollPtr GCstarImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  Q_EMIT signalTotalSteps(this, 100);

  QString str = text();
  QTextStream t(&str);
  QString line = t.readLine();
  if(line.startsWith(QLatin1String("GCfilms"))) {
    readGCfilms(str);
  } else {
    // need to reparse the string if it's in utf-8
    if(line.contains(QLatin1String("utf-8"), Qt::CaseInsensitive)) {
      str = QString::fromUtf8(str.toLocal8Bit());
    }
    // also allow for custom collections by reading collection type
    while(!line.contains(QLatin1String("collection"))) {
      line = t.readLine();
    }
    QString collType;
    static const QRegularExpression typeRx(QStringLiteral("type=\"([^\"]+)\""));
    auto typeMatch = typeRx.match(line);
    if(typeMatch.hasMatch()) {
      collType = typeMatch.captured(1);
    }
    readGCstar(str, collType);
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

  QHash<QString, Data::BorrowerPtr> borrowers;
  static const QRegularExpression rx(QStringLiteral("\\s*,\\s*"));
  static const QRegularExpression year(QStringLiteral("\\d{4}"));
  static const QRegularExpression runTimeHr(QStringLiteral("(\\d+)\\s?hr?"));
  static const QRegularExpression runTimeMin(QStringLiteral("(\\d+)\\s?mi?n?"));

  bool gotFirstLine = false;

  QString tmp = text_;
  QTextStream t(&tmp);

  const uint length = text_.length();
  const uint stepSize = qMax(s_stepSize, length/100);
  const bool showProgress = options() & ImportProgress;

  Q_EMIT signalTotalSteps(this, length);

  uint j = 0;
  for(QString line = t.readLine(); !m_cancelled && !line.isNull(); line = t.readLine(), j += line.length()) {
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
      Q_EMIT signalProgress(this, j);
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

void GCstarImporter::readGCstar(const QString& text_, const QString& collType_) {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("gcstar2tellico.xsl"));
  XSLTHandler handler(QUrl::fromLocalFile(xsltFile));
  if(!handler.isValid()) {
    setStatusMessage(i18n("Tellico encountered an error in XSLT processing."));
    return;
  }

  // Assume no custom collection starts with "GC"
  if(!collType_.startsWith(QLatin1String("GC"))) {
    const QString modelFileName = collType_ + QLatin1String(".gcm");
    QString modelFile = DataFileRegistry::self()->locate(modelFileName);
    if(modelFile.isEmpty()) {
      modelFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                         QStringLiteral("gcstar/GCModels/") + modelFileName);
    }
    if(modelFile.isEmpty()) {
      myWarning() << "Failed to find a gcm model file";
      setStatusMessage(i18n("The GCstar model file could not be found."));
      return;
    } else {
      myLog() << "Reading custom GCstar collection type:" << collType_;
      myLog() << "Model file:" << modelFile;
      QFileInfo fi(modelFile);
      const QString modelDir = fi.path() + QLatin1Char('/');
      handler.addStringParam("datadir", modelDir.toUtf8());
    }
  }
  const QString str = handler.applyStylesheet(text_);

  if(str.isEmpty()) {
    setStatusMessage(i18n("<qt>The file is not a valid GCstar data file.</qt>"));
    return;
  }

  Import::TellicoImporter imp(str);
  imp.setBaseUrl(url()); // always allow for relative image links; empty base url is ok
  if(m_imageLinksOnly) {
    imp.setOptions(imp.options() | Import::ImportImagesAsLinks);
  }

  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());
}

inline
QString GCstarImporter::splitJoin(const QRegularExpression& rx, const QString& s) {
  return s.split(rx, Qt::SkipEmptyParts).join(FieldFormat::delimiterString());
}

QWidget* GCstarImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_);
  auto l = new QVBoxLayout(m_widget);

  auto gbox = new QGroupBox(i18n("GCstar Options"), m_widget);
  auto vlay = new QVBoxLayout(gbox);

  m_cbImageLink = new QCheckBox(i18n("Import images as links only"), gbox);
  m_cbImageLink->setWhatsThis(i18n("If checked, the image paths will be imported as links "
                                   "instead of being managed by Tellico directly."));
  m_cbImageLink->setChecked(m_imageLinksOnly);
  connect(m_cbImageLink, &QCheckBox::toggled, m_cbImageLink, [this](bool checked){
    m_imageLinksOnly = checked;
  });

  vlay->addWidget(m_cbImageLink);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void GCstarImporter::slotCancel() {
  m_cancelled = true;
}

#undef CHECKLIMITS
