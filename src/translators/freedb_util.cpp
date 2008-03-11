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
#include "../tellico_debug.h"

using Tellico::Import::FreeDBImporter;

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)

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

#include <config.h>

namespace {
  class CloseDrive {
  public:
    CloseDrive(int d) : drive(d) {}
    ~CloseDrive() { ::close(drive); }
  private:
    int drive;
  };
}

QValueList<uint> FreeDBImporter::offsetList(const QCString& drive_, QValueList<uint>& trackLengths_) {
  QValueList<uint> list;

  int drive = ::open(drive_.data(), O_RDONLY | O_NONBLOCK);
  CloseDrive closer(drive);
  if(drive < 0) {
    return list;
  }

  cdrom_tochdr hdr;
#if defined(__APPLE__)
  dk_cd_read_disc_info_t discInfoParams;
  ::memset(&discInfoParams, 0, sizeof(discInfoParams));
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

  cdrom_tocentry* TocEntry = new cdrom_tocentry[last+1];
#if defined(__OpenBSD__)
  ioc_read_toc_entry t;
  t.starting_track = 0;
#elif defined(__NetBSD__)
  ioc_read_toc_entry t;
  t.starting_track = 1;
#endif
#if defined(__OpenBSD__) || defined(__NetBSD__)
  t.address_format = CDROM_LBA;
  t.data_len = (last + 1) * sizeof(cdrom_tocentry);
  t.data = TocEntry;

  if (::ioctl(drive, CDIOREADTOCENTRYS, (char *) &t) < 0)
       return list;

#elif defined(__APPLE__)
  dk_cd_read_track_info_t trackInfoParams;
  ::memset(&trackInfoParams, 0, sizeof(trackInfoParams));
  trackInfoParams.addressType = kCDTrackInfoAddressTypeTrackNumber;
  trackInfoParams.bufferLength = sizeof(*TocEntry);

  for(int i = 0; i < last; ++i) {
    trackInfoParams.address = i + 1;
    trackInfoParams.buffer = &TocEntry[i];
    ::ioctl(drive, DKIOCCDREADTRACKINFO, &trackInfoParams);
  }

  /* MacOS X on G5-based systems does not report valid info for
   * TocEntry[last-1].lastRecordedAddress + 1, so we compute the start
   * of leadout from the start+length of the last track instead
   */
  TocEntry[last].cdte_track_address = TocEntry[last-1].trackSize + TocEntry[last-1].trackStartAddress;
#else /* FreeBSD, Linux, Solaris */
  for(uint i = 0; i < last; ++i) {
    /* tracks start with 1, but I must start with 0 on OpenBSD */
    TocEntry[i].cdte_track = i + 1;
    TocEntry[i].cdte_format = CDROM_LBA;
    ::ioctl(drive, CDROMREADTOCENTRY, &TocEntry[i]);
  }

  TocEntry[last].cdte_track = CDROM_LEADOUT;
  TocEntry[last].cdte_format = CDROM_LBA;
  ::ioctl(drive, CDROMREADTOCENTRY, &TocEntry[last]);
#endif

#if defined(__FreeBSD__)
  TocEntry[last].cdte_track_address = ntohl(TocEntry[last].cdte_track_address);
#endif

  for(uint i = 0; i < last; ++i) {
#if defined(__FreeBSD__)
    TocEntry[i].cdte_track_address = ntohl(TocEntry[i].cdte_track_address);
#endif
    list.append(TocEntry[i].cdte_track_address + CD_MSF_OFFSET);
  }

  list.append(TocEntry[0].cdte_track_address + CD_MSF_OFFSET);
  list.append(TocEntry[last].cdte_track_address + CD_MSF_OFFSET);

  // hey, these are track lengths! :P
  trackLengths_.clear();
  for(uint i = 0; i < last; ++i) {
    trackLengths_.append((TocEntry[i+1].cdte_track_address - TocEntry[i].cdte_track_address) / CD_FRAMES);
  }

  delete[] TocEntry;
  return list;
}

inline
ushort from2Byte(uchar* d) {
  return (d[0] << 8 & 0xFF00) | (d[1] & 0xFF);
}

#define SIZE 61
// mostly taken from kover and k3b
// licensed under GPL
FreeDBImporter::CDText FreeDBImporter::getCDText(const QCString& drive_) {
  CDText cdtext;
#ifdef USE_CDTEXT
// only works for linux ATM
#if defined(__linux__)
  int drive = ::open(drive_.data(), O_RDONLY | O_NONBLOCK);
  CloseDrive closer(drive);
  if(drive < 0) {
    return cdtext;
  }

  cdrom_generic_command m_cmd;
  ::memset(&m_cmd, 0, sizeof(cdrom_generic_command));

  int dataLen;

  int format = 5;
  uint track = 0;
  uchar buffer[2048];

  m_cmd.cmd[0] = 0x43;
  m_cmd.cmd[1] = 0x0;
  m_cmd.cmd[2] = format & 0x0F;
  m_cmd.cmd[6] = track;
  m_cmd.cmd[8] = 2; // we only read the length first

  m_cmd.buffer = buffer;
  m_cmd.buflen = 2;
  m_cmd.data_direction = CGC_DATA_READ;

  if(ioctl(drive, CDROM_SEND_PACKET, &m_cmd) != 0) {
    myDebug() << "FreeDBImporter::getCDText() - access error" << endl;
    return cdtext;
  }

  dataLen = from2Byte(buffer) + 2;
  m_cmd.cmd[7] = 2048 >> 8;
  m_cmd.cmd[8] = 2048 & 0xFF;
  m_cmd.buflen = 2048;
  ::ioctl(drive, CDROM_SEND_PACKET, &m_cmd);
  dataLen = from2Byte(buffer) + 2;

  ::memset(buffer, 0, dataLen);

  m_cmd.cmd[7] = dataLen >> 8;
  m_cmd.cmd[8] = dataLen;
  m_cmd.buffer = buffer;
  m_cmd.buflen = dataLen;
  ::ioctl(drive, CDROM_SEND_PACKET, &m_cmd);

  bool rc = false;
  int buffer_size = (buffer[0] << 8) | buffer[1];
  buffer_size -= 2;

  char data[SIZE];
  short pos_data = 0;
  char old_block_no = 0xff;
  for(uchar* bufptr = buffer + 4; buffer_size >= 18; bufptr += 18, buffer_size -= 18) {
    char code = *bufptr;

    if((code & 0x80) != 0x80) {
      continue;
    }

    char block_no = *(bufptr + 3);
    if(block_no & 0x80) {
      myDebug() << "FreeDBImporter::readCDText() - double byte code not supported" << endl;
      continue;
    }
    block_no &= 0x70;

    if(block_no != old_block_no) {
      if(rc) {
        break;
      }
      pos_data = 0;
      old_block_no = block_no;
    }

    track = *(bufptr + 1);
    if(track & 0x80) {
      continue;
    }

    uchar* txtstr = bufptr + 4;

    int length = 11;
    while(length >= 0 && *(txtstr + length) == '\0') {
      --length;
    }

    ++length;
    if(length < 12) {
      ++length;
    }

    for(int j = 0; j < length; ++j) {
      char c = *(txtstr + j);
      if(c == '\0') {
        data[pos_data] = c;
        if(track == 0) {
          if(code == (char)0xFFFFFF80) {
            cdtext.title = QString::fromUtf8(data);
          } else if(code == (char)0xFFFFFF81) {
            cdtext.artist = QString::fromUtf8(data);
          } else if (code == (char)0xFFFFFF85) {
            cdtext.message = QString::fromUtf8(data);
          }
        } else {
          if(code == (char)0xFFFFFF80) {
            if(cdtext.trackTitles.size() < track) {
              cdtext.trackTitles.resize(track);
            }
            cdtext.trackTitles[track-1] = QString::fromUtf8(data);
          } else if(code == (char)0xFFFFFF81) {
            if(cdtext.trackArtists.size() < track) {
              cdtext.trackArtists.resize(track);
            }
            cdtext.trackArtists[track-1] = QString::fromUtf8(data);
          }
        }
        rc = true;
        pos_data = 0;
        ++track;
      } else if(pos_data < (SIZE - 1)) {
        data[pos_data++] = c;
      }
    }
  }
  if(cdtext.trackTitles.size() != cdtext.trackArtists.size()) {
    int size = QMAX(cdtext.trackTitles.size(), cdtext.trackArtists.size());
    cdtext.trackTitles.resize(size);
    cdtext.trackArtists.resize(size);
  }
#endif
#endif
  return cdtext;
}
#undef SIZE
