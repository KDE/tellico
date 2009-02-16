/***************************************************************************
    copyright            : (C) 2007 by Sebastian Held
    email                : sebastian.held@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BARCODE_V4L_H
#define BARCODE_V4L_H

#define GETELEM(array,index,default) \
	(index < sizeof(array)/sizeof(array[0]) ? array[index] : default)

/*
 *  Taken from xawtv-3.5.9 source 8/15/01 by George Staikos <staikos@kde.org>
 */
#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#define FOO__STRICT_ANSI__
#endif
#include <asm/types.h>
#ifdef FOO__STRICT_ANSI__
#define __STRICT_ANSI__ 1
#undef FOO__STRICT_ANSI__
#endif

//#include <linux/videodev2.h>
#include <linux/videodev.h>

#include <QString>
#include <QImage>
//Added by qt3to4:
#include <Q3PtrList>

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



class video_converter
{
public:
  video_converter() {;}
  virtual ~video_converter() {;}
  virtual void init( ng_video_fmt* ) {;}
  virtual void exit() {;}
  virtual void frame( QByteArray*, const QByteArray*,  ng_video_fmt ) {;}
  int fmtid_in() {return m_fmtid_in;}
  int fmtid_out() {return m_fmtid_out;}
protected:
  int m_fmtid_in, m_fmtid_out;
};

class yuv422p_to_rgb24 : public video_converter
{
public:
  yuv422p_to_rgb24() {m_fmtid_in=VIDEO_YUV422P; m_fmtid_out=VIDEO_RGB24;}
  virtual void frame( QByteArray *, const QByteArray *,  ng_video_fmt  ) {;}
};
class yuv422_to_rgb24 : public video_converter
{
public:
  yuv422_to_rgb24() {m_fmtid_in=VIDEO_YUYV; m_fmtid_out=VIDEO_RGB24;}
  virtual void frame( QByteArray *, const QByteArray *,  ng_video_fmt  ) {;}
};
class yuv420p_to_rgb24 : public video_converter
{
public:
  yuv420p_to_rgb24() {m_fmtid_in=VIDEO_YUV420P; m_fmtid_out=VIDEO_RGB24;}
  virtual void frame( QByteArray *, const QByteArray *,  ng_video_fmt  );
};

class ng_vid_driver
{
public:
  virtual ~ng_vid_driver() {}

  static ng_vid_driver *createDriver( QString device );
  static int xioctl( int fd, int cmd, void *arg );

  /* open/close */
  virtual bool open2( QString device ) = 0;
  virtual void close() = 0;

  /* attributes */
  virtual QString get_devname() {return m_name;}
  virtual int capabilities() = 0;

  /* capture */
  virtual bool setformat( ng_video_fmt *fmt ) = 0;
  virtual QByteArray* getimage2() = 0;  /* single image */

  // video converter
  static void register_video_converter( video_converter *conv );
  static video_converter *find_video_converter( int out, int in );

protected:
  QString m_name;
  static Q3PtrList<video_converter> m_converter;
};


class barcode_v4l
{
public:
  barcode_v4l();
  ~barcode_v4l();
  QImage grab_one2();
  bool isOpen();

protected:
  bool grab_init();

  QString m_devname;
  int m_grab_width, m_grab_height;
  ng_vid_driver *m_drv;
  ng_video_fmt m_fmt, m_fmt_drv;
  video_converter *m_conv;

};


class ng_vid_driver_v4l : public ng_vid_driver
{
public:
  ng_vid_driver_v4l();

  /* open/close */
  //virtual bool open( QString device );
  virtual bool open2( QString device );
  virtual void close();

  /* attributes */
  virtual int capabilities();

  /* capture */
  virtual bool setformat( ng_video_fmt *fmt );
  //virtual ng_video_buf* getimage();  /* single image */
  virtual QByteArray* getimage2();  /* single image */

protected:
  bool read_setformat( ng_video_fmt *fmt );
  QByteArray* read_getframe2();
  void *m_drv;
  int m_fd;
  video_capability m_capability;
  video_picture m_pict;
  video_window m_win;

  /* capture */
  bool m_use_read;
  ng_video_fmt m_fmt;

  unsigned short format2palette[VIDEO_FMT_COUNT];
};

} // namespace
#endif
