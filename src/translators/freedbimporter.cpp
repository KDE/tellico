/***************************************************************************
    copyright            : (C) 2005 by Robby Stephenson
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
#include "../latin1literal.h"

#if HAVE_KCDDB
#include <libkcddb/client.h>
extern "C" {
#include <cdda_interface.h>
}
#endif

#include <klocale.h>
#include <kdebug.h>
#include <kcombobox.h>
#include <kglobal.h>
#include <kconfig.h>

#include <qfile.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>

using Bookcase::Import::FreeDBImporter;

FreeDBImporter::FreeDBImporter() : Bookcase::Import::Importer(), m_coll(0), m_widget(0) {
}

Bookcase::Data::Collection* FreeDBImporter::collection() {
#if !HAVE_KCDDB
  return 0;
#else
  if(m_coll) {
    return m_coll;
  }

  struct cdrom_drive* drive = 0;
  QString device = m_deviceCombo->currentText();
  QCString drivePath = QFile::encodeName(device);
  if(!drivePath.isEmpty() && drivePath != "/") {
    drive = cdda_identify(drivePath.data(), CDDA_MESSAGE_FORGETIT, 0);
  }
  if(!drive) {
    kdDebug() << "FreeDBImporter::collection() - no drive!" << endl;
    setStatusMessage(i18n("<qt>Tellico was unable to access the CD-ROM device - <i>%1</i>.</qt>").arg(device));
    return 0;
  }

  // now it's ok to add device to saved list
  m_deviceCombo->insertItem(device);
  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
  QStringList devices;
  for(int i = 0; i < m_deviceCombo->count(); ++i) {
    if(devices.findIndex(m_deviceCombo->text(i)) == -1) {
      devices += m_deviceCombo->text(i);
    }
  }
  config.writeEntry("CD-ROM Devices", devices);
  config.writeEntry("Last Device", device);

  if(cdda_open(drive) != 0) {
    kdDebug() << "FreeDBImporter::collection() - open failed!" << endl;
    setStatusMessage(i18n("<qt>Tellico was unable to open the CD-ROM device - <i>%1</i>.</qt>").arg(device));
    cdda_close(drive);
    return 0;
  }

  KCDDB::TrackOffsetList list;
#if 0
  list
    << 150
    << 106965
    << 127220
    << 151925
    << 176085
    << 5
    << 234500;
#else
  const long tracks = cdda_tracks(drive);
//  kdDebug() << "found " << tracks << " tracks" << endl;
  for(int i = 0; i < tracks; ++i) {
// audiocd.cpp has some hack, but ignore for now
//    if((i+1) != hack_track) {
      list.append(cdda_track_firstsector(drive, i + 1) + 150);
//    } else {
//      list.append(start_of_first_data_as_in_toc + 150);
//    }
  }

  list.append(my_first_sector(drive)+150);
  list.append(my_last_sector(drive)+150);
#endif

// crashes if I use the Config class
//  KCDDB::Config c;
//  config.setHostname(QString::fromLatin1("freedb.freedb.org"));
//  config.setPort(8880);

//  KCDDB::Client client(c);
  KCDDB::Client client;
  client.setBlockingMode(true);
  KCDDB::CDDB::Result r = client.lookup(list);
  if(r != KCDDB::CDDB::Success) {
    kdDebug() << "FreeDBImporter::collection() - no success! Return value = " << r << endl;
    cdda_close(drive);
    switch(r) {
      case KCDDB::CDDB::NoRecordFound:
        setStatusMessage(i18n("<qt>No records were found to match the CD.</qt>"));
        break;

      case KCDDB::CDDB::MultipleRecordFound:
        setStatusMessage(i18n("<qt>Multiple records were found to match the CD.</qt>"));
        break;

      case KCDDB::CDDB::ServerError:
      case KCDDB::CDDB::HostNotFound:
      case KCDDB::CDDB::NoResponse:
      case KCDDB::CDDB::UnknownError:
      default:
        setStatusMessage(i18n("<qt>Tellico was unable to complete the CD lookup.</qt>"));
        break;

    }
    return 0;
  }

/*
  KCDDB::CDInfoList response = client.lookupResponse();
  kdDebug() << "Client::lookup returned : " << response.count() << " entries" << endl;

  for(KCDDB::CDInfoList::ConstIterator it = response.begin(); it != response.end(); ++it) {
    KCDDB::CDInfo info(*it);
    kdDebug() << "Disc title: " << info.title << endl;
    kdDebug() << "Total tracks: " << info.trackInfoList.count() << endl;
    kdDebug() << "Disc revision: '" << info.revision << "'" << endl;
  }
*/
  KCDDB::CDInfo info = client.bestLookupResponse();
//  kdDebug() << "Best CDInfo had title: " << info.title << endl;

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
//    if(cd->trk[i].data == 0) {
      trackList << t[i].title;
//    }
  }
  entry->setField(QString::fromLatin1("track"), trackList.join(QString::fromLatin1("; ")));

  m_coll->addEntry(entry);

  cdda_close(drive);

  return m_coll;
#endif
}

QWidget* FreeDBImporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(2, Qt::Horizontal, i18n("Audio CD Options"), m_widget);

  (void) new QLabel(i18n("CD-ROM Device:"), box);
  m_deviceCombo = new KComboBox(true, box);
  m_deviceCombo->setDuplicatesEnabled(false);
  QWhatsThis::add(m_deviceCombo, i18n("Select or input the CD-ROM device location."));

  KConfigGroup config(KGlobal::config(), QString::fromLatin1("ImportOptions - FreeDB"));
  QStringList devices = config.readListEntry("CD-ROM Devices");
  if(devices.isEmpty()) {
    devices += QString::fromLatin1("/dev/cdrom");
  }
  m_deviceCombo->insertStringList(devices);
  QString device = config.readEntry("Last Device");
  if(!device.isEmpty()) {
    m_deviceCombo->setCurrentText(device);
  }

  l->addWidget(box);
  l->addStretch(1);
  return m_widget;
}

// Everything from here on down is copied from audiocd.cpp in kdemultimedia
// Licensed under GPL v2

/* libcdda returns for cdda_disc_lastsector() the last sector of the last
   _audio_ track.  How broken. For CDDB Disc-ID we need the real last sector
   to calculate the disc length.  */
#if HAVE_KCDDB
long FreeDBImporter::my_last_sector(cdrom_drive *drive) {
  return cdda_track_lastsector(drive, drive->tracks);
}
#endif

/* Stupid CDparanoia returns the first sector of the first _audio_ track
   as first disc sector.  Equally broken to the last sector.  But what is
   even more shitty is, that if it happens that the first audio track is
   the first track at all, it returns a hardcoded _zero_, whatever else
   the TOC told it. This of course happens quite often, as usually the first
   track is audio, if there's audio at all. And usually it even works,
   because most of the time the real TOC offset is 150 frames, which we
   accounted for in our code. This is so unbelievable ugly. */
#if HAVE_KCDDB
long FreeDBImporter::my_first_sector(cdrom_drive *drive) {
  return cdda_track_firstsector(drive, 1);
}
#endif

#include "freedbimporter.moc"
