/***************************************************************************
    copyright            : (C) 2004-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "freedbimporter.h"
#include "../collections/musiccollection.h"
#include "../entry.h"
#include "../field.h"
#include "../latin1literal.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"
#include "../tellico_kernel.h"
#include "../progressmanager.h"

#include <config.h>

#if HAVE_KCDDB
#ifdef QT_NO_CAST_ASCII
#define HAD_QT_NO_CAST_ASCII
#undef QT_NO_CAST_ASCII
#endif
#include <libkcddb/client.h>
#ifdef HAD_QT_NO_CAST_ASCII
#define QT_NO_CAST_ASCII
#undef HAD_QT_NO_CAST_ASCII
#endif
#endif

#include <kcombobox.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qfile.h>
#include <qdir.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qhbox.h>
#include <qcheckbox.h>

using Tellico::Import::FreeDBImporter;

FreeDBImporter::FreeDBImporter() : Tellico::Import::Importer(), m_coll(0), m_widget(0), m_cancelled(false) {
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
    m_coll = 0;
  }
  return m_coll;
}

void FreeDBImporter::readCDROM() {
#if HAVE_KCDDB
  QString drivePath = m_driveCombo->currentText();
  if(drivePath.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>").arg(drivePath));
    myDebug() << "FreeDBImporter::readCDROM() - no drive!" << endl;
    return;
  }

  // now it's ok to add device to saved list
  m_driveCombo->insertItem(drivePath);
  QStringList drives;
  for(int i = 0; i < m_driveCombo->count(); ++i) {
    if(drives.findIndex(m_driveCombo->text(i)) == -1) {
      drives += m_driveCombo->text(i);
    }
  }

  {
    KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
    config.writeEntry("CD-ROM Devices", drives);
    config.writeEntry("Last Device", drivePath);
    config.writeEntry("Cache Files Only", false);
  }

  KCDDB::TrackOffsetList list;
#if 0
  // a1107d0a - Kruder & Dorfmeister - The K&D Sessions - Disc One.
  list
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
  QCString drive = QFile::encodeName(drivePath);
  QValueList<uint> lengths;
  list = offsetList(drive, lengths);
#endif

  if(list.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>").arg(drivePath));
    return;
  }
//  myDebug() << KCDDB::CDDB::trackOffsetListToId(list) << endl;
//  for(KCDDB::TrackOffsetList::iterator it = list.begin(); it != list.end(); ++it) {
//    myDebug() << *it << endl;
//  }

// crashes if I use the Config class
//  KCDDB::Config c;
//  config.setHostname(QString::fromLatin1("freedb.freedb.org"));
//  config.setPort(8880);

//  KCDDB::Client client(c);
  KCDDB::Client client;
  client.setBlockingMode(true);
  KCDDB::CDDB::Result r = client.lookup(list);
  if(r != KCDDB::CDDB::Success) {
//    myDebug() << "FreeDBImporter::readCDROM() - no success! Return value = " << r << endl;
    switch(r) {
      case KCDDB::CDDB::NoRecordFound:
        setStatusMessage(i18n("<qt>No records were found to match the CD.</qt>"));
        break;

      case KCDDB::CDDB::MultipleRecordFound:
        setStatusMessage(i18n("<qt>Multiple records were found to match the CD.</qt>"));
        break;

      case KCDDB::CDDB::ServerError:
        myDebug() << "Server Error" << endl;
      case KCDDB::CDDB::HostNotFound:
        myDebug() << "Host Not Found" << endl;
      case KCDDB::CDDB::NoResponse:
        myDebug() << "No Response" << endl;
      case KCDDB::CDDB::UnknownError:
        myDebug() << "Unknown Error" << endl;
      default:
        setStatusMessage(i18n("<qt>Tellico was unable to complete the CD lookup.</qt>"));
        break;

    }
    // go ahead and try to read cd-text, too
    readCDText(drive);
    return;
  }

/*
  KCDDB::CDInfoList response = client.lookupResponse();
  myDebug() << "Client::lookup returned : " << response.count() << " entries" << endl;

  for(KCDDB::CDInfoList::ConstIterator it = response.begin(); it != response.end(); ++it) {
    KCDDB::CDInfo info(*it);
    myDebug() << "Disc title: " << info.title << endl;
    myDebug() << "Total tracks: " << info.trackInfoList.count() << endl;
    myDebug() << "Disc revision: '" << info.revision << "'" << endl;
  }
*/
  KCDDB::CDInfo info = client.bestLookupResponse();
//  myDebug() << "Best CDInfo had title: " << info.title << endl;

  m_coll = new Data::MusicCollection(true);

  Data::EntryPtr entry = new Data::Entry(m_coll);
  // obviously a CD
  entry->setField(QString::fromLatin1("medium"), i18n("Compact Disc"));
  entry->setField(QString::fromLatin1("title"),  info.title);
  entry->setField(QString::fromLatin1("artist"), info.artist);
  entry->setField(QString::fromLatin1("genre"),  info.genre);
  if(info.year > 0) {
    entry->setField(QString::fromLatin1("year"), QString::number(info.year));
  }

  QStringList trackList;
  KCDDB::TrackInfoList t = info.trackInfoList;
  for(uint i = 0; i < t.count(); ++i) {
    QString s = t[i].title + "::" + info.artist;
    if(i < lengths.count()) {
      s += "::" + Tellico::minutes(lengths[i]);
    }
    trackList << s;
    // TODO: KDE4 will probably have track length too
  }
  entry->setField(QString::fromLatin1("track"), trackList.join(QString::fromLatin1("; ")));

  m_coll->addEntry(entry);
  readCDText(drive);
#endif
}

void FreeDBImporter::readCache() {
#if HAVE_KCDDB
  QDict<Data::Entry> dict;

  {
    KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
    config.writeEntry("Cache Files Only", true);
  }

  KCDDB::Config* cfg = new KCDDB::Config();
  cfg->readConfig();

  QStringList dirs = cfg->cacheLocations();
  for(QStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it) {
    dirs += Tellico::findAllSubDirs(*it);
  }

  // using a QMap is a lazy man's way of getting unique keys
  // the cddb info may be in multiple files, all with the same filename, the cddb id
  QMap<QString, QString> files;
  for(QStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it) {
    if((*it).isEmpty()) {
      continue;
    }

    QDir dir(*it);
    dir.setFilter(QDir::Files | QDir::Readable | QDir::Hidden); // hidden since I want directory files
    const QStringList list = dir.entryList();
    for(QStringList::ConstIterator it2 = list.begin(); it2 != list.end(); ++it2) {
      files.insert(*it2, dir.absFilePath(*it2), false);
    }
    kapp->processEvents();
  }

  const QString title    = QString::fromLatin1("title");
  const QString artist   = QString::fromLatin1("artist");
  const QString year     = QString::fromLatin1("year");
  const QString genre    = QString::fromLatin1("genre");
  const QString track    = QString::fromLatin1("track");
  const QString comments = QString::fromLatin1("comments");
  uint numFiles = files.count();

  if(numFiles == 0) {
    myDebug() << "FreeDBImporter::readCache() - no files found" << endl;
    return;
  }

  m_coll = new Data::MusicCollection(true);

  const uint stepSize = KMAX(static_cast<uint>(1), numFiles / 100);

  ProgressItem& item = ProgressManager::self()->newProgressItem(this, progressLabel(), true);
  item.setTotalSteps(numFiles);
  connect(&item, SIGNAL(signalCancelled(ProgressItem*)), SLOT(slotCancel()));

  uint step = 1;

  KCDDB::CDInfo info;
  for(QMap<QString, QString>::Iterator it = files.begin(); !m_cancelled && it != files.end(); ++it, ++step) {
    // open file and read content
    QFileInfo fileinfo(it.data()); // skip files larger than 10 kB
    if(!fileinfo.exists() || !fileinfo.isReadable() || fileinfo.size() > 10*1024) {
      myDebug() << "FreeDBImporter::readCache() - skipping " << it.data() << endl;
      continue;
    }
    QFile file(it.data());
    if(!file.open(IO_ReadOnly)) {
      continue;
    }
    QTextStream ts(&file);
    ts.setEncoding(QTextStream::Locale);
    QString cddbData = ts.read();
    file.close();

    if(cddbData.isEmpty() || !info.load(cddbData) || !info.isValid()) {
      myDebug() << "FreeDBImporter::readCache() - Error - CDDB record is not valid" << endl;
      myDebug() << "FreeDBImporter::readCache() - File = " << it.data() << endl;
      continue;
    }

    // create a new entry and set fields
    Data::EntryPtr entry = new Data::Entry(m_coll);
    // obviously a CD
    entry->setField(QString::fromLatin1("medium"), i18n("Compact Disc"));
    entry->setField(title,  info.title);
    entry->setField(artist, info.artist);
    entry->setField(genre,  info.genre);
    if(info.year > 0) {
      entry->setField(QString::fromLatin1("year"), QString::number(info.year));
    }

    // step through trackList
    QStringList trackList;
    KCDDB::TrackInfoList t = info.trackInfoList;
    for(uint i = 0; i < t.count(); ++i) {
      trackList << t[i].title;
    }
    entry->setField(track, trackList.join(QString::fromLatin1("; ")));

#if 0
    // add CDDB info
    const QString br = QString::fromLatin1("<br/>");
    QString comment;
    if(!info.extd.isEmpty()) {
      comment.append(info.extd + br);
    }
    if(!info.id.isEmpty()) {
      comment.append(QString::fromLatin1("CDDB-ID: ") + info.id + br);
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
    m_coll->addEntry(entry);

    if(step % stepSize == 0) {
      ProgressManager::self()->setProgress(this, step);
      kapp->processEvents();
    }
  }
#endif
}

#define SETFIELD(name,value) \
  if(entry->field(QString::fromLatin1(name)).isEmpty()) { \
    entry->setField(QString::fromLatin1(name), value); \
  }

void FreeDBImporter::readCDText(const QCString& drive_) {
#if USE_CDTEXT
  Data::EntryPtr entry;
  if(m_coll) {
    entry = m_coll->entries().front();
  } else {
    m_coll = new Data::MusicCollection(true);
  }
  if(!entry) {
    entry = new Data::Entry(m_coll);
    entry->setField(QString::fromLatin1("medium"), i18n("Compact Disc"));
    m_coll->addEntry(entry);
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
  for(uint i = 0; i < cdtext.trackTitles.size(); ++i) {
    tracks << cdtext.trackTitles[i] + "::" + cdtext.trackArtists[i];
    if(artist.isEmpty()) {
      artist = cdtext.trackArtists[i];
    }
    if(!artist.isEmpty() && artist.lower() != cdtext.trackArtists[i].lower()) {
      artist = i18n("Various");
    }
  }
  SETFIELD("track", tracks.join(QString::fromLatin1("; ")));

  // something special for compilations and such
  SETFIELD("title",  Data::Collection::s_emptyGroupTitle);
  SETFIELD("artist", artist);
#endif
}
#undef SETFIELD

QWidget* FreeDBImporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* bigbox = new QGroupBox(1, Qt::Horizontal, i18n("Audio CD Options"), m_widget);

  // cdrom stuff
  QHBox* box = new QHBox(bigbox);
  m_radioCDROM = new QRadioButton(i18n("Read data from CD-ROM device"), box);
  m_driveCombo = new KComboBox(true, box);
  m_driveCombo->setDuplicatesEnabled(false);
  QString w = i18n("Select or input the CD-ROM device location.");
  QWhatsThis::add(m_radioCDROM, w);
  QWhatsThis::add(m_driveCombo, w);

  /********************************************************************************/

  m_radioCache = new QRadioButton(i18n("Read all CDDB cache files only"), bigbox);
  QWhatsThis::add(m_radioCache, i18n("Read data recursively from all the CDDB cache files "
                                     "contained in the default cache folders."));

  // cddb cache stuff
  m_buttonGroup = new QButtonGroup(0);
  m_buttonGroup->setExclusive(true);
  m_buttonGroup->insert(m_radioCDROM);
  m_buttonGroup->insert(m_radioCache);
  connect(m_buttonGroup, SIGNAL(clicked(int)), SLOT(slotClicked(int)));

  l->addWidget(bigbox);
  l->addStretch(1);

  // now read config options
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
  QStringList devices = config.readListEntry("CD-ROM Devices");
  if(devices.isEmpty()) {
    devices += QString::fromLatin1("/dev/cdrom");
  }
  m_driveCombo->insertStringList(devices);
  QString device = config.readEntry("Last Device");
  if(!device.isEmpty()) {
    m_driveCombo->setCurrentText(device);
  }
  if(config.readBoolEntry("Cache Files Only", false)) {
    m_radioCache->setChecked(true);
  } else {
    m_radioCDROM->setChecked(true);
  }
  // set enabled widgets
  slotClicked(m_buttonGroup->selectedId());

  return m_widget;
}

void FreeDBImporter::slotClicked(int id_) {
  QButton* button = m_buttonGroup->find(id_);
  if(!button) {
    return;
  }

  m_driveCombo->setEnabled(button == m_radioCDROM);
}

void FreeDBImporter::slotCancel() {
  m_cancelled = true;
}

#include "freedbimporter.moc"
