/***************************************************************************
    Copyright (C) 2007-2009 Sebastian Held <sebastian.held@gmx.de>
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

// uses code from v4lgrab.c         (c) Linux Kernel 2.6.30

#include "barcode_v4l.h"
#include "../tellico_debug.h"

#include <fcntl.h>              /* low-level i/o */
#include <errno.h>
#include <sys/ioctl.h>

extern "C" {
#include <libv4l1.h>
}

using barcodeRecognition::barcode_v4l;

#define READ_VIDEO_PIXEL(buf, format, depth, r, g, b)                   \
{                                                                       \
  switch (format)                                                 \
  {                                                               \
    case VIDEO_PALETTE_GREY:                                \
      switch (depth)                                  \
      {                                               \
        case 4:                                 \
        case 6:                                 \
        case 8:                                 \
          (r) = (g) = (b) = (*buf++ << 8);\
          break;                          \
                  \
        case 16:                                \
          (r) = (g) = (b) =               \
            *((unsigned short *) buf);      \
          buf += 2;                       \
          break;                          \
      }                                               \
      break;                                          \
                  \
                  \
    case VIDEO_PALETTE_RGB565:                              \
    {                                                       \
      unsigned short tmp = *(unsigned short *)buf;    \
      (r) = tmp&0xF800;                               \
      (g) = (tmp<<5)&0xFC00;                          \
      (b) = (tmp<<11)&0xF800;                         \
      buf += 2;                                       \
    }                                                       \
    break;                                                  \
                  \
    case VIDEO_PALETTE_RGB555:                              \
      (r) = (buf[0]&0xF8)<<8;                         \
      (g) = ((buf[0] << 5 | buf[1] >> 3)&0xF8)<<8;    \
      (b) = ((buf[1] << 2 ) & 0xF8)<<8;               \
      buf += 2;                                       \
      break;                                          \
                  \
    case VIDEO_PALETTE_RGB24:                               \
      (r) = buf[0] << 8; (g) = buf[1] << 8;           \
      (b) = buf[2] << 8;                              \
      buf += 3;                                       \
      break;                                          \
                  \
    default:                                                \
      fprintf(stderr,                                 \
        "Format %d not yet supported\n",        \
        format);                                \
  }                                                               \
}

barcode_v4l::barcode_v4l()
{
  m_devname = QString::fromLatin1("/dev/video0");
  m_grab_width = 640;
  m_grab_height = 480;

  m_fd = -1;
  m_buffer = 0;
  m_image = 0;

  grab_init();
}

barcode_v4l::~barcode_v4l()
{
  if (m_fd >= 0)
    v4l1_close(m_fd);
  if (m_buffer)
    delete m_buffer;
  if (m_image)
    delete m_image;
}

bool barcode_v4l::isOpen()
{
  return (m_fd >= 0);
}

QImage barcode_v4l::grab_one2()
{
  unsigned int bpp = 24, x, y;
  unsigned int r = 0, g = 0, b = 0;
  unsigned int src_depth = 16;
  char *src = m_buffer->data();

  static int counter = 0; // adjustment disabled; set to e.g. 20 or 50 to enable the brightness adjustment

  if (!isOpen())
    return QImage();

  v4l1_read(m_fd, m_buffer->data(), m_win.width * m_win.height * bpp);

  if (counter) {
    long newbright;
    int f;
    counter--;
    f = get_brightness_adj((unsigned char *)m_buffer->data(), m_win.width * m_win.height, &newbright);
    if (f) {
      m_pict.brightness += (newbright << 8);
      myDebug() << "v4l: Adjusting brightness: new brightness " << m_pict.brightness << endl;
      if (v4l1_ioctl(m_fd, VIDIOCSPICT, &m_pict) == -1) {
        myDebug() << "v4l: Cannot set brightness." << endl;
        counter = 0; // do not try again
      }
    } else
      counter = 0; // do not try again
  }

  if (m_pict.palette == VIDEO_PALETTE_RGB24) {
    // optimized case
    QRgb *scanline;
    for (y = 0; y < m_win.height; ++y) {
      scanline = (QRgb*)m_image->scanLine(y);
      for (x = 0; x < m_win.width; ++x) {
        const char src1 = *(src++);
        const char src2 = *(src++);
        const char src3 = *(src++);
        scanline[x] = qRgb(src1,src2,src3);
      }
    }
  } else {
    // generic case
    for (y = 0; y < m_win.height; ++y) {
      for (x = 0; x < m_win.width; ++x) {
        READ_VIDEO_PIXEL(src, m_pict.palette, src_depth, r, g, b);
        m_image->setPixel( x, y, qRgb(r>>8,g>>8,b>>8) );
      }
    }
  }

  return *m_image;
}

bool barcode_v4l::grab_init()
{
  m_fd = v4l1_open(m_devname.toLatin1().constData(), O_RDONLY);
  if (m_fd < 0) {
    myDebug() << "v4l: open " << m_devname << ": " << strerror(errno) << endl;
    return false;
  }

  if (v4l1_ioctl(m_fd, VIDIOCGCAP, &m_capability) < 0) {
    myDebug() << "v4l: ioctl VIDIOCGCAP failed; " << m_devname << " not a video4linux device?" << endl;
    v4l1_close(m_fd);
    m_fd = -1;
    return false;
  }

  if (v4l1_ioctl(m_fd, VIDIOCGWIN, &m_win) < 0) {
    myDebug() << "v4l: ioctl VIDIOCGWIN failed" << endl;
    v4l1_close(m_fd);
    m_fd = -1;
    return false;
  }

  if (v4l1_ioctl(m_fd, VIDIOCGPICT, &m_pict) < 0) {
    myDebug() << "v4l: ioctl VIDIOCGPICT failed" << endl;
    v4l1_close(m_fd);
    m_fd = -1;
    return false;
  }

  if (m_capability.type & VID_TYPE_MONOCHROME) {
    m_pict.depth=8;
    m_pict.palette=VIDEO_PALETTE_GREY;    /* 8bit grey */
    if (v4l1_ioctl(m_fd, VIDIOCSPICT, &m_pict) < 0) {
      myDebug() << "v4l: Unable to find a supported capture format." << endl;
      v4l1_close(m_fd);
      m_fd = -1;
      return false;
    }
  } else {
    m_pict.depth=24;
    m_pict.palette=VIDEO_PALETTE_RGB24;
    if (v4l1_ioctl(m_fd, VIDIOCSPICT, &m_pict) < 0) {
      myDebug() << "v4l: Unable to find a supported capture format." << endl;
      v4l1_close(m_fd);
      m_fd = -1;
      return false;
    }
  }

  // check the values
  video_picture temp;
  v4l1_ioctl(m_fd, VIDIOCGPICT, &temp);
  if ((temp.depth != m_pict.depth) || (temp.palette != m_pict.palette)) {
    myDebug() << "v4l: Unable to find a supported capture format." << endl;
    v4l1_close(m_fd);
    m_fd = -1;
    return false;
  }

  int bpp = 24;
  m_buffer = new QByteArray;
  m_buffer->reserve( m_win.width * m_win.height * bpp ); // FIXME! I think the example from the Linux kernel wastes memory here
  m_image = new QImage( m_win.width, m_win.height, QImage::Format_RGB32 );

  return true;
}

int barcode_v4l::get_brightness_adj(unsigned char *image, long size, long *brightness) {
  long i, tot = 0;
  for (i=0;i<size*3;i++) {
    tot += image[i];
  }
  *brightness = (128 - tot/(size*3))/3;
  return !((tot/(size*3)) >= 126 && (tot/(size*3)) <= 130);
}
