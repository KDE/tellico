/***************************************************************************
 *                                                                         *
 * Modified from cd-discid.c, found at http://lly.org/~rcw/cd-discid/      *
 *                                                                         *
 * Copyright (c) 1999-2003 Robert Woodcock <rcw@debian.org>                *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "freedbimporter.h"

using Tellico::Import::FreeDBImporter;

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>

/* Porting credits:
 * Solaris: David Champion <dgc@uchicago.edu>
 * FreeBSD: Niels Bakker <niels@bakker.net>
 * OpenBSD: Marcus Daniel <danielm@uni-muenster.de>
 * NetBSD: Chris Gilbert <chris@NetBSD.org>
 * MacOSX: Evan Jones <ejones@uwaterloo.ca> http://www.eng.uwaterloo.ca/~ejones/
 */

#if defined(__linux__)

// see http://bugs.kde.org/show_bug.cgi?id=86188
#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#define _ANSI_WAS_HERE_
#endif
#include <linux/types.h>
#include <linux/cdrom.h>
#ifdef _ANSI_WAS_HERE_
#define __STRICT_ANSI__
#undef _ANSI_WAS_HERE_
#endif
#define         cdte_track_address      cdte_addr.lba

#elif defined(sun) && defined(unix) && defined(__SVR4)

#include <sys/cdio.h>
#define CD_MSF_OFFSET   150
#define CD_FRAMES       75
/* According to David Schweikert <dws@ee.ethz.ch>, cd-discid needs this
 * to compile on Solaris */
#define cdte_track_address cdte_addr.lba

#elif defined(__FreeBSD__)

#include <netinet/in.h>
#include <sys/cdio.h>
#define        CDROM_LBA       CD_LBA_FORMAT   /* first frame is 0 */
#define        CD_MSF_OFFSET   150     /* MSF offset of first frame */
#define        CD_FRAMES       75      /* per second */
#define        CDROM_LEADOUT   0xAA    /* leadout track */
#define        CDROMREADTOCHDR         CDIOREADTOCHEADER
#define        CDROMREADTOCENTRY       CDIOREADTOCENTRY
#define        cdrom_tochdr    ioc_toc_header
#define        cdth_trk0       starting_track
#define        cdth_trk1       ending_track
#define        cdrom_tocentry  ioc_read_toc_single_entry
#define        cdte_track      track
#define        cdte_format     address_format
#define        cdte_track_address       entry.addr.lba

#elif defined(__OpenBSD__) || defined(__NetBSD__)

#include <netinet/in.h>
#include <sys/cdio.h>
#define        CDROM_LBA       CD_LBA_FORMAT   /* first frame is 0 */
#define        CD_MSF_OFFSET   150     /* MSF offset of first frame */
#define        CD_FRAMES       75      /* per second */
#define        CDROM_LEADOUT   0xAA    /* leadout track */
#define        CDROMREADTOCHDR         CDIOREADTOCHEADER
#define        cdrom_tochdr    ioc_toc_header
#define        cdth_trk0       starting_track
#define        cdth_trk1       ending_track
#define        cdrom_tocentry  cd_toc_entry
#define        cdte_track      track
#define        cdte_track_address       addr.lba

#elif defined(__APPLE__)

#include <sys/types.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#define        CD_FRAMES       75      /* per second */
#define        CD_MSF_OFFSET   150     /* MSF offset of first frame */
#define        cdrom_tochdr    CDDiscInfo
#define        cdth_trk0       numberOfFirstTrack
/* NOTE: Judging by the name here, we might have to do this:
 * hdr.lastTrackNumberInLastSessionMSB << 8 *
 * sizeof(hdr.lastTrackNumberInLastSessionLSB)
 * | hdr.lastTrackNumberInLastSessionLSB; */
#define        cdth_trk1       lastTrackNumberInLastSessionLSB
#define        cdrom_tocentry  CDTrackInfo
#define        cdte_track_address trackStartAddress

#else
# warning "Your OS isn't supported yet for CDDB lookup."
#endif  /* os selection */

}

QValueList<uint> FreeDBImporter::offsetList(QCString drive_) {
  QValueList<uint> list;

#if defined(__OpenBSD__) || defined(__NetBSD__)
  struct ioc_read_toc_entry t;
#elif defined(__APPLE__)
  dk_cd_read_disc_info_t discInfoParams;
#endif

  int drive = open(drive_.data(), O_RDONLY | O_NONBLOCK);
  if(drive < 0) {
    return list;
  }

  struct cdrom_tochdr hdr;
#if defined(__APPLE__)
  memset(&discInfoParams, 0, sizeof(discInfoParams));
  discInfoParams.buffer = &hdr;
  discInfoParams.bufferLength = sizeof(hdr);
  if(ioctl(drive, DKIOCCDREADDISCINFO, &discInfoParams) < 0
     || discInfoParams.bufferLength != sizeof(hdr)) {
    return list;
  }
#else
  if(ioctl(drive, CDROMREADTOCHDR, &hdr) < 0) {
    return list;
  }
#endif

//  uchar first = hdr.cdth_trk0;
  uchar last = hdr.cdth_trk1;

  int len = (last + 1) * sizeof(struct cdrom_tocentry);

  struct cdrom_tocentry *TocEntry = (cdrom_tocentry*)malloc(len);
  if(!TocEntry) {
    return list;
  }
#if defined(__OpenBSD__)
  t.starting_track = 0;
#elif defined(__NetBSD__)
  t.starting_track = 1;
#endif
#if defined(__OpenBSD__) || defined(__NetBSD__)
  t.address_format = CDROM_LBA;
  t.data_len = len;
  t.data = TocEntry;
  memset(TocEntry, 0, len);

  ioctl(drive, CDIOREADTOCENTRYS, (char *) &t);

#elif defined(__APPLE__)
  dk_cd_read_track_info_t trackInfoParams;
  memset( &trackInfoParams, 0, sizeof( trackInfoParams ) );
  trackInfoParams.addressType = kCDTrackInfoAddressTypeTrackNumber;
  trackInfoParams.bufferLength = sizeof( *TocEntry );

  for(int i = 0; i < last; ++i) {
    trackInfoParams.address = i + 1;
    trackInfoParams.buffer = &TocEntry[i];
    ioctl(drive, DKIOCCDREADTRACKINFO, &trackInfoParams);
  }

  /* MacOS X on G5-based systems does not report valid info for
   * TocEntry[last-1].lastRecordedAddress + 1, so we compute the start
   * of leadout from the start+length of the last track instead
   */
  TocEntry[last].cdte_track_address = TocEntry[last-1].trackSize + TocEntry[last-1].trackStartAddress;
#else /* FreeBSD, Linux, Solaris */
  for(int i=0; i < last; ++i) {
    /* tracks start with 1, but I must start with 0 on OpenBSD */
    TocEntry[i].cdte_track = i + 1;
    TocEntry[i].cdte_format = CDROM_LBA;
    ioctl(drive, CDROMREADTOCENTRY, &TocEntry[i]);
  }

  TocEntry[last].cdte_track = CDROM_LEADOUT;
  TocEntry[last].cdte_format = CDROM_LBA;
  ioctl(drive, CDROMREADTOCENTRY, &TocEntry[last]);
#endif

#if defined(__FreeBSD__)
  TocEntry[last].cdte_track_address = ntohl(TocEntry[last].cdte_track_address);
#endif

  for(int i=0; i < last; ++i) {
#if defined(__FreeBSD__)
    TocEntry[i].cdte_track_address = ntohl(TocEntry[i].cdte_track_address);
#endif
    list.append(TocEntry[i].cdte_track_address + CD_MSF_OFFSET);
  }

  list.append(TocEntry[0].cdte_track_address + CD_MSF_OFFSET);
  list.append(TocEntry[last].cdte_track_address + CD_MSF_OFFSET);

  free(TocEntry);

  return list;
}
