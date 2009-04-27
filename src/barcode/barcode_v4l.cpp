/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
    email                : sebastian.held@gmx.de
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

// uses code from xawtv: webcam.c   (c) 1998-2002 Gerd Knorr

//#include <stdio.h>
//#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
//#include <unistd.h>
#include <errno.h>
//#include <malloc.h>
//#include <sys/stat.h>
//#include <sys/types.h>
//#include <sys/time.h>
//#include <sys/mman.h>
#include <sys/ioctl.h>
//#include <asm/types.h>          /* for videodev2.h */

#include "barcode_v4l.h"
#include "../tellico_debug.h"

#include <kde_file.h>

using barcodeRecognition::ng_vid_driver;
using barcodeRecognition::ng_vid_driver_v4l;
using barcodeRecognition::barcode_v4l;
using barcodeRecognition::video_converter;

Q3PtrList<barcodeRecognition::video_converter> barcodeRecognition::ng_vid_driver::m_converter;
const char *barcodeRecognition::device_cap[] = { "capture", "tuner", "teletext", "overlay", "chromakey", "clipping", "frameram", "scales", "monochrome", 0 };
const unsigned int barcodeRecognition::ng_vfmt_to_depth[] = {
    0,               /* unused   */
    8,               /* RGB8     */
    8,               /* GRAY8    */
    16,              /* RGB15 LE */
    16,              /* RGB16 LE */
    16,              /* RGB15 BE */
    16,              /* RGB16 BE */
    24,              /* BGR24    */
    32,              /* BGR32    */
    24,              /* RGB24    */
    32,              /* RGB32    */
    16,              /* LUT2     */
    32,              /* LUT4     */
    16,		     /* YUYV     */
    16,		     /* YUV422P  */
    12,		     /* YUV420P  */
    0,		     /* MJPEG    */
    0,		     /* JPEG     */
    16,		     /* UYVY     */
};

const char* barcodeRecognition::ng_vfmt_to_desc[] = {
    "none",
    "8 bit PseudoColor (dithering)",
    "8 bit StaticGray",
    "15 bit TrueColor (LE)",
    "16 bit TrueColor (LE)",
    "15 bit TrueColor (BE)",
    "16 bit TrueColor (BE)",
    "24 bit TrueColor (LE: bgr)",
    "32 bit TrueColor (LE: bgr-)",
    "24 bit TrueColor (BE: rgb)",
    "32 bit TrueColor (BE: -rgb)",
    "16 bit TrueColor (lut)",
    "32 bit TrueColor (lut)",
    "16 bit YUV 4:2:2 (packed, YUYV)",
    "16 bit YUV 4:2:2 (planar)",
    "12 bit YUV 4:2:0 (planar)",
    "MJPEG (AVI)",
    "JPEG (JFIF)",
    "16 bit YUV 4:2:2 (packed, UYVY)",
};

unsigned int  barcodeRecognition::ng_yuv_gray[256];
unsigned int  barcodeRecognition::ng_yuv_red[256];
unsigned int  barcodeRecognition::ng_yuv_blue[256];
unsigned int  barcodeRecognition::ng_yuv_g1[256];
unsigned int  barcodeRecognition::ng_yuv_g2[256];
unsigned int  barcodeRecognition::ng_clip[256 + 2 * CLIP];

void barcodeRecognition::ng_color_yuv2rgb_init()
{
    int i;
# define RED_NULL    128
# define BLUE_NULL   128
# define LUN_MUL     256
# define RED_MUL     512
# define BLUE_MUL    512
#define GREEN1_MUL  (-RED_MUL/2)
#define GREEN2_MUL  (-BLUE_MUL/6)
#define RED_ADD     (-RED_NULL  * RED_MUL)
#define BLUE_ADD    (-BLUE_NULL * BLUE_MUL)
#define GREEN1_ADD  (-RED_ADD/2)
#define GREEN2_ADD  (-BLUE_ADD/6)

    /* init Lookup tables */
    for (i = 0; i < 256; i++) {
        barcodeRecognition::ng_yuv_gray[i] = i * LUN_MUL >> 8;
        barcodeRecognition::ng_yuv_red[i]  = (RED_ADD    + i * RED_MUL)    >> 8;
        barcodeRecognition::ng_yuv_blue[i] = (BLUE_ADD   + i * BLUE_MUL)   >> 8;
        barcodeRecognition::ng_yuv_g1[i]   = (GREEN1_ADD + i * GREEN1_MUL) >> 8;
        barcodeRecognition::ng_yuv_g2[i]   = (GREEN2_ADD + i * GREEN2_MUL) >> 8;
    }
    for (i = 0; i < CLIP; i++)
        barcodeRecognition::ng_clip[i] = 0;
    for (; i < CLIP + 256; i++)
        barcodeRecognition::ng_clip[i] = i - CLIP;
    for (; i < 2 * CLIP + 256; i++)
        barcodeRecognition::ng_clip[i] = 255;
}











barcode_v4l::barcode_v4l()
{
  m_devname = QLatin1String("/dev/video0");
  m_grab_width = 640;
  m_grab_height = 480;
  m_drv = 0;
  m_conv = 0;

  barcodeRecognition::ng_color_yuv2rgb_init();
  //ng_vid_driver::register_video_converter( new barcodeRecognition::yuv422p_to_rgb24() );
  //ng_vid_driver::register_video_converter( new barcodeRecognition::yuv422_to_rgb24() );
  ng_vid_driver::register_video_converter( new barcodeRecognition::yuv420p_to_rgb24() );

  grab_init();
}

barcode_v4l::~barcode_v4l()
{
  if (m_drv) {
    m_drv->close();
    delete m_drv;
  }
}

bool barcode_v4l::isOpen()
{
  return m_drv;
}

QImage barcode_v4l::grab_one2()
{
  if (!m_drv) {
    myDebug() << "no driver/device available" << endl;
    return QImage();
  }

  QByteArray *cap;
  if (!(cap = m_drv->getimage2())) {
      myDebug() << "capturing image failed" << endl;
      return QImage();
  }

  QByteArray *buf;
  if (m_conv) {
    buf = new QByteArray();
    buf->resize( 3*m_fmt.width*m_fmt.height );
    m_conv->frame( buf, cap, m_fmt_drv );
  } else {
    buf = new QByteArray(*cap);
  }
  delete cap;

  int depth = 32;
  QByteArray *buf2 = new QByteArray();
  buf2->resize( depth/8 *m_fmt.width*m_fmt.height );
  for (uint i=0; i<m_fmt.width*m_fmt.height; i++) {
    (*buf2)[i*4+0] = buf->at(i*3+2);
    (*buf2)[i*4+1] = buf->at(i*3+1);
    (*buf2)[i*4+2] = buf->at(i*3+0);
    (*buf2)[i*4+3] = 0;
  }
  delete buf;
//  QImage *temp = new QImage( (uchar*)buf2->data(), m_fmt.width, m_fmt.height, depth, 0, 0, QImage::LittleEndian );
  QImage *temp = new QImage( (uchar*)buf2->data(), m_fmt.width, m_fmt.height, QImage::Format_RGB32 );
  QImage temp2 = temp->copy();
  delete temp;
  delete buf2;

  return temp2;
}







bool barcode_v4l::grab_init()
{
  m_drv = ng_vid_driver::createDriver( m_devname );
  if (!m_drv) {
      myDebug() << "no grabber device available" << endl;
      return false;
  }
  if (!(m_drv->capabilities() & CAN_CAPTURE)) {
      myDebug() << "device doesn't support capture" << endl;
      m_drv->close();
      delete m_drv;
      m_drv = 0;
      return false;
  }

  /* try native */
  m_fmt.fmtid  = VIDEO_RGB24;
  m_fmt.width  = m_grab_width;
  m_fmt.height = m_grab_height;
  if (m_drv->setformat( &m_fmt )) {
    m_fmt_drv = m_fmt;
    return true;
  }

  /* check all available conversion functions */
  m_fmt.bytesperline = m_fmt.width * ng_vfmt_to_depth[m_fmt.fmtid] / 8;
  for (int i = 0; i<VIDEO_FMT_COUNT; i++) {
    video_converter *conv = ng_vid_driver::find_video_converter( VIDEO_RGB24, i );
    if (!conv)
      continue;

    m_fmt_drv = m_fmt;
    m_fmt_drv.fmtid = conv->fmtid_in();
    m_fmt_drv.bytesperline = 0;
    if (m_drv->setformat( &m_fmt_drv )) {
      m_fmt.width  = m_fmt_drv.width;
      m_fmt.height = m_fmt_drv.height;
      m_conv = conv;
      //m_conv->init( &m_fmt );
      return true;
    }
  }

  myDebug() << "can't get rgb24 data" << endl;
  m_drv->close();
  delete m_drv;
  m_drv = 0;
  return false;
}


ng_vid_driver* ng_vid_driver::createDriver( QString device )
{
    /* check all grabber drivers */
    ng_vid_driver *drv = new ng_vid_driver_v4l();
    if (!drv->open2( device )) {
      myDebug() << "no v4l device found" << endl;
      delete drv;
      drv = 0;
    }

    return drv;
}

int ng_vid_driver::xioctl( int fd, int cmd, void *arg )
{
  int rc;

  rc = ioctl(fd,cmd,arg);
  if (rc == 0)
    return 0;
  //print_ioctl(stderr,ioctls_v4l1,PREFIX,cmd,arg);
  qDebug( ": %s\n",(rc == 0) ? "ok" : strerror(errno) );
  return rc;
}



void ng_vid_driver::register_video_converter( video_converter *conv )
{
  if (!conv)
    return;

  m_converter.append( conv );
}

video_converter *ng_vid_driver::find_video_converter( int out, int in )
{
  video_converter *conv;
  for (conv = m_converter.first(); conv; conv = m_converter.next() )
      if ((conv->fmtid_in() == in) && (conv->fmtid_out() == out))
        return conv;
  return 0;
}


















ng_vid_driver_v4l::ng_vid_driver_v4l()
{
  m_name = QLatin1String("v4l");
  m_drv = 0;
  m_fd = -1;

  m_use_read = false;

  for (int i=0; i<VIDEO_FMT_COUNT; i++)
    format2palette[i] = 0;
  format2palette[VIDEO_RGB08]    = VIDEO_PALETTE_HI240;
  format2palette[VIDEO_GRAY]     = VIDEO_PALETTE_GREY;
  format2palette[VIDEO_RGB15_LE] = VIDEO_PALETTE_RGB555;
  format2palette[VIDEO_RGB16_LE] = VIDEO_PALETTE_RGB565;
  format2palette[VIDEO_BGR24]    = VIDEO_PALETTE_RGB24;
  format2palette[VIDEO_BGR32]    = VIDEO_PALETTE_RGB32;
  format2palette[VIDEO_YUYV]     = VIDEO_PALETTE_YUV422;
  format2palette[VIDEO_UYVY]     = VIDEO_PALETTE_UYVY;
  format2palette[VIDEO_YUV422P]  = VIDEO_PALETTE_YUV422P;
  format2palette[VIDEO_YUV420P]  = VIDEO_PALETTE_YUV420P;
}

bool ng_vid_driver_v4l::open2( QString device )
{
  /* open device */
  if ((m_fd = KDE_open( device.toLatin1(), O_RDWR )) == -1) {
    qDebug() << "v4l: open" << device.toLatin1() << ":" << strerror(errno);
    return false;
  }
  if (ioctl( m_fd, VIDIOCGCAP, &m_capability ) == -1) {
    ::close( m_fd );
    return false;
  }

#ifdef Barcode_DEBUG
  qDebug( "v4l: open: %s (%s)\n", device.toLatin1(), m_capability.name );
#endif

  fcntl( m_fd, F_SETFD, FD_CLOEXEC ); // close on exit

#ifdef Barcode_DEBUG
  myDebug() << "  capabilities: " << endl;
  for (int i = 0; device_cap[i] != NULL; i++)
    if (m_capability.type & (1 << i))
      qDebug( " %s", device_cap[i] );
  qDebug( "  size    : %dx%d => %dx%d", m_capability.minwidth, m_capability.minheight, m_capability.maxwidth, m_capability.maxheight );
#endif

#ifdef Barcode_DEBUG
  fprintf(stderr,"  v4l: using read() for capture\n");
#endif
  m_use_read = true;

  return true;
}

void ng_vid_driver_v4l::close()
{
#ifdef Barcode_DEBUG
  fprintf(stderr, "v4l: close\n");
#endif

  if (m_fd != -1)
    ::close(m_fd);
  m_fd = -1;
  return;
}

int ng_vid_driver_v4l::capabilities()
{
    int ret = 0;

    if (m_capability.type & VID_TYPE_OVERLAY)
	ret |= CAN_OVERLAY;
    if (m_capability.type & VID_TYPE_CAPTURE)
	ret |= CAN_CAPTURE;
    if (m_capability.type & VID_TYPE_TUNER)
	ret |= CAN_TUNE;
    if (m_capability.type & VID_TYPE_CHROMAKEY)
	ret |= NEEDS_CHROMAKEY;
    return ret;
}

bool ng_vid_driver_v4l::setformat( ng_video_fmt *fmt )
{
  bool rc = false;

#ifdef Barcode_DEBUG
  fprintf(stderr,"v4l: setformat\n");
#endif
  if (m_use_read) {
    rc = read_setformat( fmt );
  } else {
    Q_ASSERT( false );
  }

  m_fmt = *fmt;

  return rc;
}





bool ng_vid_driver_v4l::read_setformat( ng_video_fmt *fmt )
{
    xioctl( m_fd, VIDIOCGCAP, &m_capability );
    if (fmt->width > static_cast<uint>(m_capability.maxwidth))
      fmt->width = m_capability.maxwidth;
    if (fmt->height > static_cast<uint>(m_capability.maxheight))
      fmt->height = m_capability.maxheight;
    fmt->bytesperline = fmt->width * ng_vfmt_to_depth[fmt->fmtid] / 8;

    xioctl( m_fd, VIDIOCGPICT, &m_pict );
    m_pict.depth   = ng_vfmt_to_depth[fmt->fmtid];
    m_pict.palette = GETELEM(format2palette,fmt->fmtid,0);
    m_pict.brightness = 20000;
    if (-1 == xioctl( m_fd, VIDIOCSPICT, &m_pict ))
      return false;

    xioctl( m_fd, VIDIOCGWIN, &m_win);
    m_win.width = m_fmt.width;
    m_win.height = m_fmt.height;
    m_win.clipcount = 0;
    if (-1 == xioctl( m_fd, VIDIOCSWIN, &m_win ))
      return false;

    return true;
}

QByteArray* ng_vid_driver_v4l::getimage2()
{
#ifdef Barcode_DEBUG
  myDebug() << "v4l: getimage2" << endl;
#endif

  return read_getframe2();
}


QByteArray* ng_vid_driver_v4l::read_getframe2()
{
    QByteArray* buf;
    int size;

    size = m_fmt.bytesperline * m_fmt.height;
    buf = new QByteArray();
    buf->resize( size );
    if (size != read( m_fd, buf->data(), size )) {
      delete( buf );
      return 0;
    }
    return buf;
}




























#define GRAY(val)		barcodeRecognition::ng_yuv_gray[val]
#define RED(gray,red)		ng_clip[ CLIP + gray + barcodeRecognition::ng_yuv_red[red] ]
#define GREEN(gray,red,blue)	ng_clip[ CLIP + gray + barcodeRecognition::ng_yuv_g1[red] +	\
						       barcodeRecognition::ng_yuv_g2[blue] ]
#define BLUE(gray,blue)		ng_clip[ CLIP + gray + barcodeRecognition::ng_yuv_blue[blue] ]

void barcodeRecognition::yuv420p_to_rgb24::frame( QByteArray *out, const QByteArray *in, const ng_video_fmt fmt )
{
    unsigned char *y, *u, *v, *d;
    unsigned char *us,*vs;
    unsigned char *dp;
    unsigned int i,j;
    int gray;

    dp = (unsigned char*)out->data();
    y  = (unsigned char*)in->data();
    u  = y + fmt.width * fmt.height;
    v  = u + fmt.width * fmt.height / 4;

    for (i = 0; i < fmt.height; i++) {
	d = dp;
	us = u; vs = v;
	for (j = 0; j < fmt.width; j+= 2) {
	    gray   = GRAY(*y);
	    *(d++) = RED(gray,*v);
	    *(d++) = GREEN(gray,*v,*u);
	    *(d++) = BLUE(gray,*u);
	    y++;
	    gray   = GRAY(*y);
	    *(d++) = RED(gray,*v);
	    *(d++) = GREEN(gray,*v,*u);
	    *(d++) = BLUE(gray,*u);
	    y++; u++; v++;
	}
	if (0 == (i % 2)) {
	    u = us; v = vs;
	}
	dp += 3*fmt.width;
    }
}
