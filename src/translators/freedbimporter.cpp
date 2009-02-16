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

/*
 * We could use KCompactDisc now, but it's very buggy,
 * see https://bugs.kde.org/show_bug.cgi?id=183520
 *     https://bugs.kde.org/show_bug.cgi?id=183521
 *     http://lists.kde.org/?l=kde-multimedia&m=123397778013726&w=2
 */

#include "freedbimporter.h"
#include "../collections/musiccollection.h"
#include "../entry.h"
#include "../field.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"
#include "../tellico_kernel.h"
#include "../progressmanager.h"

#include <config.h>

#ifdef HAVE_KCDDB
#include <libkcddb/client.h>
#endif

#include <kcombobox.h>
#include <KConfigGroup>
#include <kapplication.h>
#include <kinputdialog.h>

#include <QFile>
#include <QDir>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QTextStream>
#include <QVBoxLayout>
#include <QTextCodec>

using Tellico::Import::FreeDBImporter;

FreeDBImporter::FreeDBImporter() : Tellico::Import::Importer(), m_widget(0), m_cancelled(false) {
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
#ifdef HAVE_KCDDB
  QString drivePath = m_driveCombo->currentText();
  if(drivePath.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>", drivePath));
    myDebug() << "FreeDBImporter::readCDROM() - no drive!" << endl;
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
    KConfigGroup config(KGlobal::config(), QLatin1String("ImportOptions - FreeDB"));
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
    << 10       // Disc start.
    << 316732;  // Disc end.
*/
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
    << 10       // Disc start.
    << 224925;  // Disc end.
/*
  list
    << 150
    << 106965
    << 127220
    << 151925
    << 176085
    << 5
    << 234500;
*/
#else
  list = offsetList(drive, lengths);
#endif

  if(list.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>", drivePath));
    return;
  }
//  myDebug() << KCDDB::trackOffsetListToId(list) << endl;
//  for(KCDDB::TrackOffsetList::iterator it = list.begin(); it != list.end(); ++it) {
//    myDebug() << *it << endl;
//  }

  // the result info, could be multiple ones
  KCDDB::CDInfo info;
  KCDDB::Client client;
  client.setBlockingMode(true);
  KCDDB::Result r = client.lookup(list);
  // KCDDB doesn't return MultipleRecordFound properly, so check outselves
  if(r == KCDDB::MultipleRecordFound || client.lookupResponse().count() > 1) {
    QStringList list;
    KCDDB::CDInfoList infoList = client.lookupResponse();
    for(KCDDB::CDInfoList::iterator it = infoList.begin(); it != infoList.end(); ++it) {
      list.append(QString::fromLatin1("%1, %2, %3").arg((*it).get(KCDDB::Artist).toString())
                                                   .arg((*it).get(KCDDB::Title).toString())
                                                   .arg((*it).get(KCDDB::Genre).toString()));
    }

    // switch back to pointer cursor
    GUI::CursorSaver cs(Qt::ArrowCursor);
    bool ok;
    QString res = KInputDialog::getItem(i18n("Select CDDB Entry"),
                                        i18n("Select a CDDB entry:"),
                                        list, 0, false, &ok,
                                        Kernel::self()->widget());
    if(ok) {
      int i = 0;
      foreach(const QString& listValue, list) {
        if(listValue == res) {
          break;
        }
	++i;
      }
      if(i < infoList.size()) {
        info = infoList[i];
      }
    } else { // cancelled dialog
      m_cancelled = true;
    }
  } else if(r == KCDDB::Success) {
    info = client.lookupResponse().first();
  } else {
//    myDebug() << "FreeDBImporter::readCDROM() - no success! Return value = " << r << endl;
    QString s;
    switch(r) {
      case KCDDB::NoRecordFound:
        s = i18n("<qt>No records were found to match the CD.</qt>");
        break;
      case KCDDB::ServerError:
        myDebug() << "Server Error" << endl;
        break;
      case KCDDB::HostNotFound:
        myDebug() << "Host Not Found" << endl;
        break;
      case KCDDB::NoResponse:
        myDebug() << "No Response" << endl;
        break;
      case KCDDB::UnknownError:
        myDebug() << "Unknown Error" << endl;
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
  entry->setField(QLatin1String("medium"), i18n("Compact Disc"));
  entry->setField(QLatin1String("title"),  info.get(KCDDB::Title).toString());
  entry->setField(QLatin1String("artist"), info.get(KCDDB::Artist).toString());
  entry->setField(QLatin1String("genre"),  info.get(KCDDB::Genre).toString());
  if(!info.get(KCDDB::Year).isNull()) {
    entry->setField(QLatin1String("year"), info.get(KCDDB::Year).toString());
  }
  entry->setField(QLatin1String("keyword"), info.get(KCDDB::Category).toString());
  QString extd = info.get(QLatin1String("EXTD")).toString();
  extd.replace('\n', QLatin1String("<br/>"));
  entry->setField(QLatin1String("comments"), extd);

  QStringList trackList;
  for(int i = 0; i < info.numberOfTracks(); ++i) {
    QString s = info.track(i).get(KCDDB::Title).toString() + "::" + info.get(KCDDB::Artist).toString();
    if(i < lengths.count()) {
      s += "::" + Tellico::minutes(lengths[i]);
    }
    trackList << s;
    // TODO: KDE4 will probably have track length too
  }
  entry->setField(QLatin1String("track"), trackList.join(QLatin1String("; ")));

  m_coll->addEntries(entry);
  readCDText(drive);
#endif
}

void FreeDBImporter::readCache() {
#ifdef HAVE_KCDDB
  {
    // remember the import options
    KConfigGroup config(KGlobal::config(), QLatin1String("ImportOptions - FreeDB"));
    config.writeEntry("Cache Files Only", true);
  }

  KCDDB::Config cfg;
  cfg.readConfig();

  QStringList dirs = cfg.cacheLocations();
  foreach(const QString& dirName, dirs) {
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
//    kapp->processEvents(); // really needed ?
  }

  const QString title    = QLatin1String("title");
  const QString artist   = QLatin1String("artist");
  const QString year     = QLatin1String("year");
  const QString genre    = QLatin1String("genre");
  const QString track    = QLatin1String("track");
  const QString comments = QLatin1String("comments");
  int numFiles = files.count();

  if(numFiles == 0) {
    myDebug() << "FreeDBImporter::readCache() - no files found" << endl;
    return;
  }

  m_coll = new Data::MusicCollection(true);

  const uint stepSize = qMax(1, numFiles / 100);
  const bool showProgress = options() & ImportProgress;

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(numFiles);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));
  ProgressItem::Done done(this);

  uint step = 1;

  KCDDB::CDInfo info;
  for(QMap<QString, QString>::Iterator it = files.begin(); !m_cancelled && it != files.end(); ++it, ++step) {
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
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    QString cddbData = ts.readAll();
    file.close();

    if(cddbData.isEmpty() || !info.load(cddbData) || !info.isValid()) {
      myDebug() << "FreeDBImporter::readCache() - Error - CDDB record is not valid" << endl;
      myDebug() << "FreeDBImporter::readCache() - File = " << it.value() << endl;
      continue;
    }

    // create a new entry and set fields
    Data::EntryPtr entry(new Data::Entry(m_coll));
    // obviously a CD
    entry->setField(QLatin1String("medium"), i18n("Compact Disc"));
    entry->setField(title,  info.get(KCDDB::Title).toString());
    entry->setField(artist, info.get(KCDDB::Artist).toString());
    entry->setField(genre,  info.get(KCDDB::Genre).toString());
    if(!info.get(KCDDB::Year).isNull()) {
      entry->setField(QLatin1String("year"), info.get(KCDDB::Year).toString());
    }
    entry->setField(QLatin1String("keyword"), info.get(KCDDB::Category).toString());
    QString extd = info.get(QLatin1String("EXTD")).toString();
    extd.replace('\n', QLatin1String("<br/>"));
    entry->setField(QLatin1String("comments"), extd);

    // step through trackList
    QStringList trackList;
    for(int i = 0; i < info.numberOfTracks(); ++i) {
      trackList << info.track(i).get(KCDDB::Title).toString();
    }
    entry->setField(track, trackList.join(QLatin1String("; ")));

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
      kapp->processEvents();
    }
  }
#endif
}

#define SETFIELD(name,value) \
  if(entry->field(QLatin1String(name)).isEmpty()) { \
    entry->setField(QLatin1String(name), value); \
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
    entry->setField(QLatin1String("medium"), i18n("Compact Disc"));
    m_coll->addEntries(entry);
  }

  CDText cdtext = getCDText(drive_);
/*
  myDebug() << "CDText - title: " << cdtext.title << endl;
  myDebug() << "CDText - title: " << cdtext.artist << endl;
  for(int i = 0; i < cdtext.trackTitles.size(); ++i) {
    myDebug() << i << "::" << cdtext.trackTitles[i] << " - " << cdtext.trackArtists[i] << endl;
  }
*/

  QString artist = cdtext.artist;
  SETFIELD("title",    cdtext.title);
  SETFIELD("artist",   artist);
  SETFIELD("comments", cdtext.message);
  QStringList tracks;
  for(int i = 0; i < cdtext.trackTitles.size(); ++i) {
    tracks << cdtext.trackTitles[i] + "::" + cdtext.trackArtists[i];
    if(artist.isEmpty()) {
      artist = cdtext.trackArtists[i];
    }
    if(!artist.isEmpty() && artist.toLower() != cdtext.trackArtists[i].toLower()) {
      artist = i18n("Various");
    }
  }
  SETFIELD("track", tracks.join(QLatin1String("; ")));

  // something special for compilations and such
  SETFIELD("title",  i18n(Data::Collection::s_emptyGroupTitle));
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
  connect(m_buttonGroup, SIGNAL(buttonClicked(int)), SLOT(slotClicked(int)));

  l->addWidget(gbox);
  l->addStretch(1);

  // now read config options
  KConfigGroup config(KGlobal::config(), QLatin1String("ImportOptions - FreeDB"));
  QStringList devices = config.readEntry("CD-ROM Devices", QStringList());
  if(devices.isEmpty()) {
#if defined(__OpenBSD__)
    devices += QLatin1String("/dev/rcd0c");
#endif
    devices += QLatin1String("/dev/cdrom");
    devices += QLatin1String("/dev/dvd");
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

#include "freedbimporter.moc"
