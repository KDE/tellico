/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>

#include "freedbimporter.h"
#include "../tellico_debug.h"

#include <QList>

extern "C" {
#ifdef HAVE_CDIO
#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#endif
}

namespace {
#ifdef HAVE_CDIO
  class CloseCdio {
  public:
    CloseCdio(CdIo_t* d) : cdio(d) {}
    ~CloseCdio() { cdio_destroy(cdio); }
  private:
    CdIo_t* cdio;
  };
#endif
}

using Tellico::Import::FreeDBImporter;

QList<uint> FreeDBImporter::offsetList(const QByteArray& drive_, QList<uint>& trackLengths_) {
  QList<uint> list;
#ifdef HAVE_CDIO
  // CDDB needs the Logical Block Addresses (LBA) for the track sector offsets
  CdIo_t* cdio_p = cdio_open(drive_.constData(), DRIVER_UNKNOWN);
  if(cdio_p == nullptr) {
    myDebug() << "no cdio pointer";
    return list;
  }
  CloseCdio closer(cdio_p);

  track_t i_track = cdio_get_first_track_num(cdio_p);
  track_t i_tracks = i_track + cdio_get_num_tracks(cdio_p);

  trackLengths_.clear();
  list.reserve(i_tracks+1);
  for( ; i_track < i_tracks; ++i_track) {
    list.append(cdio_get_track_lba(cdio_p, i_track));
    // add 1 so the conversion to int rounds up
    trackLengths_.append(1+cdio_get_track_sec_count(cdio_p, i_track) / CDIO_CD_FRAMES_PER_SEC);
  }
  list.append(cdio_lsn_to_lba(cdio_get_disc_last_lsn(cdio_p)));

#else
  Q_UNUSED(drive_);
  Q_UNUSED(trackLengths_);
#endif
  return list;
}

FreeDBImporter::CDText FreeDBImporter::getCDText(const QByteArray& drive_) {
  CDText cdtext;
#if defined(ENABLE_CDTEXT) && defined(HAVE_CDIO)
  CdIo_t* cdio_p = cdio_open(drive_.constData(), DRIVER_UNKNOWN);
  if(cdio_p == nullptr) {
    myDebug() << "no cdio pointer";
    return cdtext;
  }
  CloseCdio closer(cdio_p);

#if LIBCDIO_VERSION_NUM >= 90
  const cdtext_t* cdtext_p = cdio_get_cdtext(cdio_p);
#else
  const cdtext_t* cdtext_p = cdio_get_cdtext(cdio_p, 0);
#endif
  if(cdtext_p == nullptr) {
    myDebug() << "no cdtext pointer";
    return cdtext;
  }

#if LIBCDIO_VERSION_NUM >= 90
  const char* title = cdtext_get_const(cdtext_p, CDTEXT_FIELD_TITLE, 0);
#else
  const char* title = cdtext_get_const(CDTEXT_TITLE, cdtext_p);
#endif
  cdtext.title = QString::fromUtf8(title);
#if LIBCDIO_VERSION_NUM >= 90
  const char* performer = cdtext_get_const(cdtext_p, CDTEXT_FIELD_PERFORMER, 0);
#else
  const char* performer = cdtext_get_const(CDTEXT_PERFORMER, cdtext_p);
#endif
  cdtext.artist = QString::fromUtf8(performer);
#if LIBCDIO_VERSION_NUM >= 90
  const char* message = cdtext_get_const(cdtext_p, CDTEXT_FIELD_MESSAGE, 0);
#else
  const char* message = cdtext_get_const(CDTEXT_MESSAGE, cdtext_p);
#endif
  cdtext.message = QString::fromUtf8(message);

  track_t i_track = cdio_get_first_track_num(cdio_p);
  track_t i_tracks = i_track + cdio_get_num_tracks(cdio_p);
  for( ; i_track < i_tracks; ++i_track) {
#if LIBCDIO_VERSION_NUM >= 90
    const char* title = cdtext_get_const(cdtext_p, CDTEXT_FIELD_TITLE, i_track);
    cdtext.trackTitles.append(QString::fromUtf8(title));
    const char* performer = cdtext_get_const(cdtext_p, CDTEXT_FIELD_PERFORMER, i_track);
    cdtext.trackArtists.append(QString::fromUtf8(performer));
#else
    const cdtext_t *track_cdtext_p = cdio_get_cdtext(cdio_p, i_track);
    const char* title = cdtext_get_const(CDTEXT_TITLE, track_cdtext_p);
    cdtext.trackTitles.append(QString::fromUtf8(title));
    const char* performer = cdtext_get_const(CDTEXT_PERFORMER, track_cdtext_p);
    cdtext.trackArtists.append(QString::fromUtf8(performer));
#endif
  }
#else
  Q_UNUSED(drive_);
#endif
  return cdtext;
}
