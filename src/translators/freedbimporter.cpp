/***************************************************************************
    copyright            : (C) 2004-2005 by Robby Stephenson
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

#include <klocale.h>
#include <kcombobox.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kprogress.h>

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

FreeDBImporter::FreeDBImporter() : Tellico::Import::Importer(), m_coll(0), m_widget(0) {
}

bool FreeDBImporter::canImport(int type) const {
  return type == Data::Collection::Album;
}

Tellico::Data::Collection* FreeDBImporter::collection() {
#if !HAVE_KCDDB
  return 0;
#else
  if(m_coll) {
    return m_coll;
  }

  if(m_radioCDROM->isChecked()) {
    return readCDROM();
  } else {
    return readCache();
  }
}

Tellico::Data::Collection* FreeDBImporter::readCDROM() {
  QString drivePath = m_driveCombo->currentText();
  QCString drive = QFile::encodeName(drivePath);
  if(drivePath.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>").arg(drivePath));
    myDebug() << "FreeDBImporter::readCDROM() - no drive!" << endl;
    return 0;
  }

  // now it's ok to add device to saved list
  m_driveCombo->insertItem(drivePath);
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
  QStringList drives;
  for(int i = 0; i < m_driveCombo->count(); ++i) {
    if(drives.findIndex(m_driveCombo->text(i)) == -1) {
      drives += m_driveCombo->text(i);
    }
  }
  config.writeEntry("CD-ROM Devices", drives);
  config.writeEntry("Last Device", drivePath);
  config.writeEntry("Cache Files Only", false);

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
  list = offsetList(drive);
#endif

  if(list.isEmpty()) {
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>").arg(drivePath));
    return 0;
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
    myDebug() << "FreeDBImporter::readCDROM() - no success! Return value = " << r << endl;
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
        myDebug() << "No Repsonse" << endl;
      case KCDDB::CDDB::UnknownError:
        myDebug() << "Unknown Error" << endl;
      default:
        setStatusMessage(i18n("<qt>Tellico was unable to complete the CD lookup.</qt>"));
        break;

    }
    return 0;
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

  Data::Entry* entry = new Data::Entry(m_coll);
  // obviously a CD
  entry->setField(QString::fromLatin1("medium"), i18n("Compact Disc"));
  entry->setField(QString::fromLatin1("title"), info.title);
  entry->setField(QString::fromLatin1("artist"), info.artist);
  entry->setField(QString::fromLatin1("genre"), info.genre);
  if(info.year > 0) {
    entry->setField(QString::fromLatin1("year"), QString::number(info.year));
  }

  QStringList trackList;
  KCDDB::TrackInfoList t = info.trackInfoList;
  for(uint i = 0; i < t.count(); ++i) {
    trackList << t[i].title;
  }
  entry->setField(QString::fromLatin1("track"), trackList.join(QString::fromLatin1("; ")));

  m_coll->addEntry(entry);

  return m_coll;
}

Tellico::Data::Collection* FreeDBImporter::readCache() {
  QDict<Data::Entry> dict;

  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
  config.writeEntry("Cache Files Only", true);

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
    return 0;
  }

  m_coll = new Data::MusicCollection(true);

  KProgressDialog progressDlg(m_widget);
  progressDlg.setLabel(i18n("Scanning CDDB cache files..."));
  progressDlg.progressBar()->setTotalSteps(numFiles);
  progressDlg.setMinimumDuration(numFiles > 20 ? 500 : 2000); // default is 2000

  const uint stepSize = KMAX(static_cast<uint>(1), numFiles / 100);
  uint step = 1;

  KCDDB::CDInfo info;
  for(QMap<QString, QString>::Iterator it = files.begin(); it != files.end(); ++it, ++step) {
    if(progressDlg.wasCancelled()) {
      break;
    }

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
    Data::Entry* entry = new Data::Entry(m_coll);
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
      progressDlg.progressBar()->setProgress(step);
      kapp->processEvents();
    }
  }
  if(progressDlg.wasCancelled()) {
    delete m_coll;
    return 0;
  }

  return m_coll;
#endif
}

QWidget* FreeDBImporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* bigbox = new QGroupBox(1, Qt::Horizontal, i18n("Audio CD Options"), m_widget);


  // cdrom stuff
  QHBox* box = new QHBox(bigbox);
  m_radioCDROM = new QRadioButton(i18n("Read data from CD-ROM device"), box);
//  QLabel* label1 = new QLabel(i18n("CD-ROM Device:"), box);
  m_driveCombo = new KComboBox(true, box);
  m_driveCombo->setDuplicatesEnabled(false);
  QWhatsThis::add(m_radioCDROM, i18n("Select or input the CD-ROM device location."));
  QWhatsThis::add(m_driveCombo, i18n("Select or input the CD-ROM device location."));

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

#include "freedbimporter.moc"
