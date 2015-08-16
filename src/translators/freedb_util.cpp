/***************************************************************************
 *                                                                         *
 * Modified from cd-discid.c, found at http://lly.org/~rcw/cd-discid/      *
 *                                                                         *
 * Copyright (c) 1999-2003 Robert Woodcock <rcw@debian.org>                *
 *                                                                         *
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

#include "freedbimporter.h"
#include "../tellico_debug.h"

#include <QList>
#include <qplatformdefs.h>

using Tellico::Import::FreeDBImporter;

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#if defined(__linux__)

// see http://bugs.kde.org/show_bug.cgi?id=86188
#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#define _ANSI_WAS_HERE_
#endif
#include <linux/types.h>
// swabb.h is not C++ friendly. Prevent including it.
#define _LINUX_BYTEORDER_SWABB_H
#include <linux/cdrom.h>
#ifdef _ANSI_WAS_HERE_
#define __STRICT_ANSI__
#undef _ANSI_WAS_HERE_
#endif
#define         cdte_track_address      cdte_addr.lba

#endif  /* os selection */

}

#include <config.h>

#ifdef HAVE_DISCID
#include <discid/discid.h>
#endif

namespace {
  class CloseDrive {
  public:
    CloseDrive(int d) : drive(d) {}
    ~CloseDrive() { ::close(drive); }
  private:
    int drive;
  };

#ifdef HAVE_DISCID
  class CloseDiscId {
  public:
    CloseDiscId(DiscId* d) : disc(d) {}
    ~CloseDiscId() { discid_free(disc); }
  private:
    DiscId* disc;
  };
#endif
}

QList<uint> FreeDBImporter::offsetList(const QByteArray& drive_, QList<uint>& trackLengths_) {
  QList<uint> list;

#ifdef HAVE_DISCID
  DiscId* disc = discid_new();
  if (!disc) {
    return list;
  }

  const CloseDiscId closer(disc);

#ifdef DISCID_HAVE_SPARSE_READ
  int res = discid_read_sparse(disc, drive_.constData(), 0);
#else
  int res = discid_read(disc, drive_.constData());
#endif
  if (!res) {
    return list;
  }

  const int first = discid_get_first_track_num(disc);
  const int last = discid_get_last_track_num(disc);

  trackLengths_.clear();
  for (int i = first; i <= last; ++i) {
    list.append(discid_get_track_offset(disc, i));
    trackLengths_.append(discid_get_track_length(disc, i));
  }
  list.append(discid_get_sectors(disc));
#endif
  return list;
}

inline
ushort from2Byte(uchar* d) {
  return (d[0] << 8 & 0xFF00) | (d[1] & 0xFF);
}

#define SIZE 61
// mostly taken from kover and k3b
// licensed under GPL
FreeDBImporter::CDText FreeDBImporter::getCDText(const QByteArray& drive_) {
  CDText cdtext;
#ifdef ENABLE_CDTEXT
// only works for linux ATM
#if defined(__linux__)
  int drive = QT_OPEN(drive_.data(), O_RDONLY | O_NONBLOCK);
  CloseDrive closer(drive);
  if(drive < 0) {
    return cdtext;
  }

  cdrom_generic_command m_cmd;
  ::memset(&m_cmd, 0, sizeof(cdrom_generic_command));

  int dataLen;

  int format = 5;
  int track = 0;
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
    myDebug() << "access error";
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
      myDebug() << "double byte code not supported";
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
          } else if(code == char(0xFFFFFF81)) {
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
    int size = qMax(cdtext.trackTitles.size(), cdtext.trackArtists.size());
    cdtext.trackTitles.resize(size);
    cdtext.trackArtists.resize(size);
  }
#endif
#else
  Q_UNUSED(drive_);
#endif
  return cdtext;
}
#undef SIZE
