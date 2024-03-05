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

/*
 * We could use KCompactDisc now, but it's very buggy,
 * see https://bugs.kde.org/show_bug.cgi?id=183520
 *     https://bugs.kde.org/show_bug.cgi?id=183521
 *     https://lists.kde.org/?l=kde-multimedia&m=123397778013726&w=2
 */

#include <config.h>

#include "freedbimporter.h"
#include "../collections/musiccollection.h"
#include "../entry.h"
#include "../fieldformat.h"
#include "../utils/tellico_utils.h"
#include "../utils/string_utils.h"
#include "../utils/guiproxy.h"
#include "../progressmanager.h"
#include "../utils/cursorsaver.h"
#include "../tellico_debug.h"

#if defined HAVE_KCDDB
#include <KCddb/Client>
#elif defined HAVE_KCDDB
#include <libkcddb/client.h>
#endif

#include <KComboBox>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QInputDialog>
#include <QFile>
#include <QDir>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QTextStream>
#include <QVBoxLayout>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QTextCodec>
#else
#include <QStringConverter>
#endif
#include <QApplication>

using Tellico::Import::FreeDBImporter;

FreeDBImporter::FreeDBImporter() : Tellico::Import::Importer()
    , m_widget(nullptr)
    , m_buttonGroup(nullptr)
    , m_radioCDROM(nullptr)
    , m_radioCache(nullptr)
    , m_driveCombo(nullptr)
    , m_cancelled(false) {
}

bool FreeDBImporter::canImport(int type) const {
  return type == Data::Collection::Album;
}

Tellico::Data::CollPtr FreeDBImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  m_cancelled = false;
  if(m_radioCDROM->isChecked()) {
    readCDROM();
  } else {
    readCache();
  }
  if(m_cancelled) {
    m_coll = Data::CollPtr();
  }
  return m_coll;
}

void FreeDBImporter::readCDROM() {
#if defined (HAVE_OLD_KCDDB) || defined (HAVE_KCDDB)
  QString drivePath = m_driveCombo->currentText();
  if(drivePath.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>", drivePath));
    myDebug() << "no drive!";
    return;
  }

  // now it's ok to add device to saved list
  m_driveCombo->addItem(drivePath);
  QStringList drives;
  for(int i = 0; i < m_driveCombo->count(); ++i) {
    if(drives.indexOf(m_driveCombo->itemText(i)) == -1) {
      drives += m_driveCombo->itemText(i);
    }
  }

  {
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - FreeDB"));
    config.writeEntry("CD-ROM Devices", drives);
    config.writeEntry("Last Device", drivePath);
    config.writeEntry("Cache Files Only", false);
  }

  QByteArray drive = QFile::encodeName(drivePath);
  QList<uint> lengths;
  KCDDB::TrackOffsetList list;
#if 0
  // a1107d0a - Kruder & Dorfmeister - The K&D Sessions - Disc One.
/*  list
    << 150      // First track start.
    << 29462
    << 66983
    << 96785
    << 135628
    << 168676
    << 194147
    << 222158
    << 247076
    << 278203   // Last track start.
    << 316732;  // Disc end.
*/
/*
  list
    << 150      // First track start.
    << 3296
    << 14437
    << 41279
    << 51362
    << 56253
    << 59755
    << 61324
    << 66059
    << 69073
    << 77790
    << 83214
    << 89726
    << 92078
    << 106325
    << 113117
    << 116040
    << 119877
    << 124377
    << 145466
    << 157583
    << 167208
    << 173486
    << 180120
    << 185279
    << 193270
    << 206451
    << 217303   // Last track start.
    << 224925;  // Disc end.
  list
    << 150
    << 106965
    << 127220
    << 151925
    << 176085
    << 234500;
*/
#else
  list = offsetList(drive, lengths);
#endif

  if(list.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>", drivePath));
    return;
  }
//  myDebug() << list;

  // the result info, could be multiple ones
  KCDDB::CDInfo info;
  KCDDB::Client client;
  client.setBlockingMode(true);

  KCDDB::Result r = client.lookup(list);
  const KCDDB::CDInfoList responseList = client.lookupResponse();
  // KCDDB sometimes doesn't return MultipleRecordFound properly, so check length of response, too
  if(r == KCDDB::MultipleRecordFound || responseList.count() > 1) {
    QStringList list;
    foreach(const KCDDB::CDInfo& info, responseList) {
      list.append(QStringLiteral("%1, %2, %3").arg(info.get(KCDDB::Artist).toString(),
                                                        info.get(KCDDB::Title).toString(),
                                                        info.get(KCDDB::Genre).toString()));
    }

    // switch back to pointer cursor
    GUI::CursorSaver cs(Qt::ArrowCursor);
    bool ok;
    QString res = QInputDialog::getItem(GUI::Proxy::widget(),
                                        i18n("Select CDDB Entry"),
                                        i18n("Select a CDDB entry:"),
                                        list, 0, false, &ok);
    if(ok) {
      int i = 0;
      foreach(const QString& listValue, list) {
        if(listValue == res) {
          break;
        }
        ++i;
      }
      if(i < responseList.size()) {
        info = responseList.at(i);
      }
    } else { // cancelled dialog
      m_cancelled = true;
    }
  } else if(r == KCDDB::Success && !responseList.isEmpty()) {
    info = responseList.first();
  } else {
    myDebug() << "no success! Return value = " << r;
    QString s;
    switch(r) {
      case KCDDB::NoRecordFound:
        s = i18n("<qt>No records were found to match the CD.</qt>");
        break;
      case KCDDB::ServerError:
        myDebug() << "Server Error";
        break;
      case KCDDB::HostNotFound:
        myDebug() << "Host Not Found";
        break;
      case KCDDB::NoResponse:
        myDebug() << "No Response";
        break;
      case KCDDB::UnknownError:
        myDebug() << "Unknown Error";
        break;
      default:
        break;
    }
    if(s.isEmpty()) {
      s = i18n("<qt>Tellico was unable to complete the CD lookup.</qt>");
    }
    setStatusMessage(s);
    return;
  }

  if(!info.isValid()) {
    // go ahead and try to read cd-text if we weren't cancelled
    // could be the case we don't have net access
    if(!m_cancelled) {
      readCDText(drive);
    }
    return;
  }

  m_coll = new Data::MusicCollection(true);

  Data::EntryPtr entry(new Data::Entry(m_coll));
  // obviously a CD
  entry->setField(QStringLiteral("medium"), i18n("Compact Disc"));
  entry->setField(QStringLiteral("title"),  info.get(KCDDB::Title).toString());
  entry->setField(QStringLiteral("artist"), info.get(KCDDB::Artist).toString());
  entry->setField(QStringLiteral("genre"),  info.get(KCDDB::Genre).toString());
  if(!info.get(KCDDB::Year).isNull()) {
    entry->setField(QStringLiteral("year"), info.get(KCDDB::Year).toString());
  }
  entry->setField(QStringLiteral("keyword"), info.get(KCDDB::Category).toString());
  QString extd = info.get(QStringLiteral("EXTD")).toString();
  extd.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
  entry->setField(QStringLiteral("comments"), extd);

  QStringList trackList;
  trackList.reserve(info.numberOfTracks());
  for(int i = 0; i < info.numberOfTracks(); ++i) {
    QString s = info.track(i).get(KCDDB::Title).toString() + FieldFormat::columnDelimiterString();
    const QString trackArtist = info.track(i).get(KCDDB::Artist).toString().trimmed();
    s += trackArtist.isEmpty() ? info.get(KCDDB::Artist).toString() : trackArtist;
    if(i < lengths.count()) {
      s += FieldFormat::columnDelimiterString() + Tellico::minutes(lengths[i]);
    }
    trackList << s;
  }
  entry->setField(QStringLiteral("track"), trackList.join(FieldFormat::rowDelimiterString()));

  m_coll->addEntries(entry);
  readCDText(drive);
#endif
}

void FreeDBImporter::readCache() {
#if defined (HAVE_OLD_KCDDB) || defined (HAVE_KCDDB)
  {
    // remember the import options
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - FreeDB"));
    config.writeEntry("Cache Files Only", true);
  }

  KCDDB::Config cfg;
  cfg.load();

  const QStringList cacheDirs = cfg.cacheLocations();
  QStringList dirs = cacheDirs;
  foreach(const QString& dirName, cacheDirs) {
    dirs += Tellico::findAllSubDirs(dirName);
  }

  // using a QMap is a lazy man's way of getting unique keys
  // the cddb info may be in multiple files, all with the same filename, the cddb id
  QMap<QString, QString> files;
  foreach(const QString& dirName, dirs) {
    if(dirName.isEmpty()) {
      continue;
    }

    QDir dir(dirName);
    dir.setFilter(QDir::Files | QDir::Readable | QDir::Hidden); // hidden since I want directory files
    const QStringList list = dir.entryList();
    foreach(const QString& listEntry, list) {
      files.insert(listEntry, dir.absoluteFilePath(listEntry));
    }
//    qApp->processEvents(); // really needed ?
  }

  const QString title    = QStringLiteral("title");
  const QString artist   = QStringLiteral("artist");
  const QString year     = QStringLiteral("year");
  const QString genre    = QStringLiteral("genre");
  const QString medium   = QStringLiteral("medium");
  const QString keyword  = QStringLiteral("keyword");
  const QString track    = QStringLiteral("track");
  const QString comments = QStringLiteral("comments");
  int numFiles = files.count();

  if(numFiles == 0) {
    myDebug() << "no files found";
    return;
  }

  m_coll = new Data::MusicCollection(true);

  const uint stepSize = qMax(1, numFiles / 100);
  const bool showProgress = options() & ImportProgress;

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(numFiles);
  connect(&item, &Tellico::ProgressItem::signalCancelled, this, &FreeDBImporter::slotCancel);
  ProgressItem::Done done(this);

  uint step = 1;

  KCDDB::CDInfo info;
  for(QMap<QString, QString>::ConstIterator it = files.constBegin(); !m_cancelled && it != files.constEnd(); ++it, ++step) {
    // open file and read content
    QFileInfo fileinfo(it.value()); // skip files larger than 10 kB
    if(!fileinfo.exists() || !fileinfo.isReadable() || fileinfo.size() > 10*1024) {
      myDebug() << "skipping " << it.value();
      continue;
    }
    QFile file(it.value());
    if(!file.open(QIODevice::ReadOnly)) {
      continue;
    }
    QTextStream ts(&file);
    // libkcddb always writes the cache files in utf-8
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ts.setCodec("UTF-8");
#else
    ts.setEncoding(QStringConverter::Utf8);
#endif
    QString cddbData = ts.readAll();
    file.close();

    if(cddbData.isEmpty() || !info.load(cddbData) || !info.isValid()) {
      myDebug() << "Error - CDDB record is not valid";
      myDebug() << "File = " << it.value();
      continue;
    }

    // create a new entry and set fields
    Data::EntryPtr entry(new Data::Entry(m_coll));
    // obviously a CD
    entry->setField(medium, i18n("Compact Disc"));
    entry->setField(title, info.get(KCDDB::Title).toString());
    entry->setField(artist, info.get(KCDDB::Artist).toString());
    entry->setField(genre, info.get(KCDDB::Genre).toString());
    if(!info.get(KCDDB::Year).isNull()) {
      entry->setField(year, info.get(KCDDB::Year).toString());
    }
    entry->setField(keyword, info.get(KCDDB::Category).toString());
    QString extd = info.get(QStringLiteral("EXTD")).toString();
    extd.replace(QLatin1Char('\n'), QLatin1String("<br/>"));
    entry->setField(comments, extd);

    // step through trackList
    QStringList trackList;
    trackList.reserve(info.numberOfTracks());
    for(int i = 0; i < info.numberOfTracks(); ++i) {
      trackList << info.track(i).get(KCDDB::Title).toString();
    }
    entry->setField(track, trackList.join(FieldFormat::rowDelimiterString()));

#if 0
    // add CDDB info
    const QString br = QLatin1String("<br/>");
    QString comment;
    if(!info.extd.isEmpty()) {
      comment.append(info.extd + br);
    }
    if(!info.id.isEmpty()) {
      comment.append(QLatin1String("CDDB-ID: ") + info.id + br);
    }
    if(info.length > 0) {
      comment.append("Length: " + QString::number(info.length) + br);
    }
    if(info.revision > 0) {
      comment.append("Revision: " + QString::number(info.revision) + br);
    }
    entry->setField(comments, comment);
#endif

    // add this entry to the music collection
    m_coll->addEntries(entry);

    if(showProgress && step%stepSize == 0) {
      ProgressManager::self()->setProgress(this, step);
      qApp->processEvents();
    }
  }
#endif
}

#define SETFIELD(name,value) \
  if(entry->field(QStringLiteral(name)).isEmpty()) { \
    entry->setField(QStringLiteral(name), value); \
  }

void FreeDBImporter::readCDText(const QByteArray& drive_) {
#ifdef ENABLE_CDTEXT
  Data::EntryPtr entry;
  if(m_coll) {
    if(m_coll->entryCount() > 0) {
      entry = m_coll->entries().front();
    }
  } else {
    m_coll = new Data::MusicCollection(true);
  }
  if(!entry) {
    entry = new Data::Entry(m_coll);
    entry->setField(QStringLiteral("medium"), i18n("Compact Disc"));
    m_coll->addEntries(entry);
  }

  CDText cdtext = getCDText(drive_);
/*
  myDebug() << "CDText - title: " << cdtext.title;
  myDebug() << "CDText - title: " << cdtext.artist;
  for(int i = 0; i < cdtext.trackTitles.size(); ++i) {
    myDebug() << i << "--" << cdtext.trackTitles[i] << " - " << cdtext.trackArtists[i];
  }
*/

  QString artist = cdtext.artist;
  SETFIELD("title",    cdtext.title);
  SETFIELD("artist",   artist);
  SETFIELD("comments", cdtext.message);
  QStringList tracks;
  tracks.reserve(cdtext.trackTitles.size());
  for(int i = 0; i < cdtext.trackTitles.size(); ++i) {
    tracks << cdtext.trackTitles[i] + FieldFormat::columnDelimiterString() + cdtext.trackArtists[i];
    if(artist.isEmpty()) {
      artist = cdtext.trackArtists[i];
    }
    if(!artist.isEmpty() && artist.toLower() != cdtext.trackArtists[i].toLower()) {
      artist = i18n("Various");
    }
  }
  SETFIELD("track", tracks.join(FieldFormat::rowDelimiterString()));

  // something special for compilations and such
  SETFIELD("artist", artist);
#else
  Q_UNUSED(drive_);
#endif
}
#undef SETFIELD

QWidget* FreeDBImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("Audio CD Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  // cdrom stuff
  QHBoxLayout* hlay = new QHBoxLayout();
  vlay->addLayout(hlay);

  m_radioCDROM = new QRadioButton(i18n("Read data from CD-ROM device"), gbox);
  m_driveCombo = new KComboBox(true, gbox);
  m_driveCombo->setDuplicatesEnabled(false);
  QString w = i18n("Select or input the CD-ROM device location.");
  m_radioCDROM->setWhatsThis(w);
  m_driveCombo->setWhatsThis(w);

  hlay->addWidget(m_radioCDROM);
  hlay->addWidget(m_driveCombo);

  /********************************************************************************/

  m_radioCache = new QRadioButton(i18n("Read all CDDB cache files only"), gbox);
  m_radioCache->setWhatsThis(i18n("Read data recursively from all the CDDB cache files "
                                  "contained in the default cache folders."));
  vlay->addWidget(m_radioCache);

  // cddb cache stuff
  m_buttonGroup = new QButtonGroup(gbox);
  m_buttonGroup->addButton(m_radioCDROM);
  m_buttonGroup->addButton(m_radioCache);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  void (QButtonGroup::* buttonClickedInt)(int) = &QButtonGroup::buttonClicked;
  connect(m_buttonGroup, buttonClickedInt, this, &FreeDBImporter::slotClicked);
#else
  connect(m_buttonGroup, &QButtonGroup::idClicked, this, &FreeDBImporter::slotClicked);
#endif

  l->addWidget(gbox);
  l->addStretch(1);

  // now read config options
  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - FreeDB"));
  QStringList devices = config.readEntry("CD-ROM Devices", QStringList());
  if(devices.isEmpty()) {
#if defined(__OpenBSD__)
    devices += QLatin1String("/dev/rcd0c");
#endif
    devices += QStringLiteral("/dev/cdrom");
    devices += QStringLiteral("/dev/dvd");
  }
  m_driveCombo->addItems(devices);
  QString device = config.readEntry("Last Device");
  if(!device.isEmpty()) {
    m_driveCombo->setEditText(device);
  }
  if(config.readEntry("Cache Files Only", false)) {
    m_radioCache->setChecked(true);
  } else {
    m_radioCDROM->setChecked(true);
  }
  // set enabled widgets
  slotClicked(m_buttonGroup->checkedId());

  return m_widget;
}

void FreeDBImporter::slotClicked(int id_) {
  QAbstractButton* button = m_buttonGroup->button(id_);
  if(!button) {
    return;
  }

  m_driveCombo->setEnabled(button == m_radioCDROM);
}

void FreeDBImporter::slotCancel() {
  m_cancelled = true;
}
