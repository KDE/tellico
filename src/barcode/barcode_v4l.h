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

#ifndef BARCODE_V4L_H
#define BARCODE_V4L_H

//#define Barcode_DEBUG

#include <QString>
#include <QImage>

#include <linux/types.h>
#include <linux/videodev.h>

namespace barcodeRecognition {

struct ng_video_fmt {
  unsigned int   fmtid;         /* VIDEO_* */
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytesperline;  /* zero for compressed formats */
};
enum { CAN_OVERLAY=1, CAN_CAPTURE=2, CAN_TUNE=4, NEEDS_CHROMAKEY=8 };
enum { VIDEO_NONE=0, VIDEO_RGB08, VIDEO_GRAY, VIDEO_RGB15_LE, VIDEO_RGB16_LE,
        VIDEO_RGB15_BE, VIDEO_RGB16_BE, VIDEO_BGR24, VIDEO_BGR32, VIDEO_RGB24,
        VIDEO_RGB32, VIDEO_LUT2, VIDEO_LUT4, VIDEO_YUYV, VIDEO_YUV422P,
        VIDEO_YUV420P, VIDEO_MJPEG, VIDEO_JPEG, VIDEO_UYVY, VIDEO_FMT_COUNT };
extern const char *device_cap[];
extern const unsigned int ng_vfmt_to_depth[];
extern const char* ng_vfmt_to_desc[];

/* lookup tables */
#define CLIP         320
extern unsigned int  ng_yuv_gray[256];
extern unsigned int  ng_yuv_red[256];
extern unsigned int  ng_yuv_blue[256];
extern unsigned int  ng_yuv_g1[256];
extern unsigned int  ng_yuv_g2[256];
extern unsigned int  ng_clip[256 + 2 * CLIP];
void ng_color_yuv2rgb_init();

class barcode_v4l
{
public:
  barcode_v4l();
  ~barcode_v4l();
  QImage grab_one2();
  bool isOpen();

protected:
  bool grab_init();
  int get_brightness_adj(unsigned char *image, long size, long *brightness);

  QString m_devname;
  int m_fd;
  int m_grab_width, m_grab_height;
  video_capability m_capability;
  video_picture m_pict;
  video_window m_win;
  QByteArray *m_buffer;
  QImage *m_image;
};

} // namespace
#endif
