/*
 Copyright (C) 1998, 1999, 2001, 2002, 2004, 2005, 2007
      Daniel M. Duley <daniel.duley@verizon.net>
 (C) 2004 Zack Rusin <zack@kde.org>
 (C) 2000 Josef Weidendorfer <weidendo@in.tum.de>
 (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 (C) 1998, 1999 Christian Tibirna <ctibirna@total.net>
 (C) 1998, 1999 Dirk Mueller <mueller@kde.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 Diagonal gradient code was inspired by BlackBox. BlackBox gradients are
 (C) Brad Hughes, <bhughes@tcac.net> and Mike Cole <mike@mydot.com>.
 */

#include "gradient.h"

#include <QColor>
#include <QVector>

#include <cmath>

QImage Tellico::gradient(QSize size, const QColor &ca,
                         const QColor &cb, Tellico::GradientType eff)
{
    QImage image(size, QImage::Format_RGB32);
    if(!size.isValid())
        return(image);

    int rca, gca, bca, rcb, gcb, bcb;
    int rDiff = (rcb = cb.red())   - (rca = ca.red());
    int gDiff = (gcb = cb.green()) - (gca = ca.green());
    int bDiff = (bcb = cb.blue())  - (bca = ca.blue());
    int x, y;
    QRgb rgb;

    if(eff == VerticalGradient || eff == HorizontalGradient){
        int rl = rca << 16;
        int gl = gca << 16;
        int bl = bca << 16;
        QRgb *p;
        if(eff == VerticalGradient){
            int rcdelta = ((1<<16) / size.height()) * rDiff;
            int gcdelta = ((1<<16) / size.height()) * gDiff;
            int bcdelta = ((1<<16) / size.height()) * bDiff;
            for(y=0; y < size.height(); ++y){
                rl += rcdelta;
                gl += gcdelta;
                bl += bcdelta;
                rgb = qRgb( (rl>>16), (gl>>16), (bl>>16) );

                p = (QRgb *)image.scanLine(y);
                for(x = 0; x < size.width(); ++x)
                    *p++ = rgb;
            }
        }
        else{                  // must be HorizontalGradient
            int rcdelta = ((1<<16) / size.width()) * rDiff;
            int gcdelta = ((1<<16) / size.width()) * gDiff;
            int bcdelta = ((1<<16) / size.width()) * bDiff;
            p = (QRgb *)image.scanLine(0);
            for(x = 0; x < size.width(); ++x){
                rl += rcdelta;
                gl += gcdelta;
                bl += bcdelta;
                *p++ = qRgb((rl>>16), (gl>>16), (bl>>16));
            }
            p = (QRgb *)image.scanLine(0);
            for(y = 1; y < size.height(); ++y)
                memcpy(image.scanLine(y), p, size.width()*sizeof(QRgb));
        }
    }
    else{
        float rfd, gfd, bfd;
        float rd = rca, gd = gca, bd = bca;

        int w = size.width(), h = size.height();
        int dw = w*2, dh = h*2;
        unsigned char *xtable = new unsigned char[w*3];
        unsigned char *ytable = new unsigned char[h*3];

        if(eff == DiagonalGradient || eff == CrossDiagonalGradient){
            rfd = (float)rDiff/dw;
            gfd = (float)gDiff/dw;
            bfd = (float)bDiff/dw;

            int dir;
            for(x=0; x < w; x++, rd+=rfd, gd+=gfd, bd+=bfd) {
                dir = eff == DiagonalGradient? x : w - x - 1;
                xtable[dir*3] = (unsigned char) rd;
                xtable[dir*3+1] = (unsigned char) gd;
                xtable[dir*3+2] = (unsigned char) bd;
            }
            rfd = (float)rDiff/dh;
            gfd = (float)gDiff/dh;
            bfd = (float)bDiff/dh;
            rd = gd = bd = 0;
            for(y = 0; y < h; y++, rd+=rfd, gd+=gfd, bd+=bfd){
                ytable[y*3] = (unsigned char) rd;
                ytable[y*3+1] = (unsigned char) gd;
                ytable[y*3+2] = (unsigned char) bd;
            }

            for(y = 0; y < h; y++){
                QRgb *p = (QRgb *)image.scanLine(y);
                for(x = 0; x < w; x++){
                    *p++ = qRgb(xtable[x*3] + ytable[y*3],
                                xtable[x*3+1] + ytable[y*3+1],
                                xtable[x*3+2] + ytable[y*3+2]);
                }
            }
        }
        else{
            int rSign = rDiff>0? 1: -1;
            int gSign = gDiff>0? 1: -1;
            int bSign = bDiff>0? 1: -1;

            rfd = (float)rDiff/w;
            gfd = (float)gDiff/w;
            bfd = (float)bDiff/w;

            rd = (float)rDiff/2;
            gd = (float)gDiff/2;
            bd = (float)bDiff/2;

            for(x=0; x < w; x++, rd-=rfd, gd-=gfd, bd-=bfd){
                xtable[x*3] = (unsigned char) qAbs((int)rd);
                xtable[x*3+1] = (unsigned char) qAbs((int)gd);
                xtable[x*3+2] = (unsigned char) qAbs((int)bd);
            }

            rfd = (float)rDiff/h;
            gfd = (float)gDiff/h;
            bfd = (float)bDiff/h;

            rd = (float)rDiff/2;
            gd = (float)gDiff/2;
            bd = (float)bDiff/2;

            for(y=0; y < h; y++, rd-=rfd, gd-=gfd, bd-=bfd){
                ytable[y*3] = (unsigned char) qAbs((int)rd);
                ytable[y*3+1] = (unsigned char) qAbs((int)gd);
                ytable[y*3+2] = (unsigned char) qAbs((int)bd);
            }

            dw = (w+1)>>1;
            dh = (h+1)>>1;
            int x2;
            QRgb *sl1, *sl2;
            for(y = 0; y < dh; y++){
                sl1 = (QRgb *)image.scanLine(y);
                sl2 = (QRgb *)image.scanLine(qMax(h-y-1, y));
                for(x = 0, x2 = w-1; x < dw; x++, x2--){
                    switch(eff){
                    case PyramidGradient:
                        rgb = qRgb(rcb-rSign*(xtable[x*3]+ytable[y*3]),
                                   gcb-gSign*(xtable[x*3+1]+ytable[y*3+1]),
                                   bcb-bSign*(xtable[x*3+2]+ytable[y*3+2]));
                        break;
                    case RectangleGradient:
                        rgb = qRgb(rcb - rSign *
                                   qMax(xtable[x*3], ytable[y*3]) * 2,
                                   gcb - gSign *
                                   qMax(xtable[x*3+1], ytable[y*3+1]) * 2,
                                   bcb - bSign *
                                   qMax(xtable[x*3+2], ytable[y*3+2]) * 2);
                        break;
                    case PipeCrossGradient:
                        rgb = qRgb(rcb - rSign *
                                   qMin(xtable[x*3], ytable[y*3]) * 2,
                                   gcb - gSign *
                                   qMin(xtable[x*3+1], ytable[y*3+1]) * 2,
                                   bcb - bSign *
                                   qMin(xtable[x*3+2], ytable[y*3+2]) * 2);
                        break;
                    case EllipticGradient:
                    default:
                        rgb = qRgb(rcb - rSign *
                                   (int)std::sqrt((xtable[x*3]*xtable[x*3] +
                                                   ytable[y*3]*ytable[y*3])*2.0f),
                                   gcb - gSign *
                                   (int)std::sqrt((xtable[x*3+1]*xtable[x*3+1] +
                                                   ytable[y*3+1]*ytable[y*3+1])*2.0f),
                                   bcb - bSign *
                                   (int)std::sqrt((xtable[x*3+2]*xtable[x*3+2] +
                                                   ytable[y*3+2]*ytable[y*3+2])*2.0f));
                        break;
                    }
                    sl1[x] = sl2[x] = rgb;
                    sl1[x2] = sl2[x2] = rgb;
                }
            }
        }
        delete [] xtable;
        delete [] ytable;
    }
    return(image);
}

QImage Tellico::grayGradient(QSize size, unsigned char ca,
                             unsigned char cb, Tellico::GradientType eff)
{
    QImage image(size, QImage::Format_Indexed8);
    if(!size.isValid())
        return(image);
    QVector<QRgb> colorTable(256);
    for(int i=0; i < 256; ++i)
        colorTable[i] = qRgba(i, i, i, 255);
    image.setColorTable(colorTable);

    int diff = cb - ca;
    int x, y;
    unsigned char idx;

    if(eff == VerticalGradient || eff == HorizontalGradient){
        int val = ca << 16;
        unsigned char *p;
        if(eff == VerticalGradient){
            int delta = ((1<<16) / size.height()) * diff;
            for(y=0; y < size.height(); ++y){
                val += delta;
                idx = val >> 16;
                p = image.scanLine(y);
                for(x = 0; x < size.width(); ++x)
                    *p++ = idx;
            }
        }
        else{                  // must be HorizontalGradient
            int delta = ((1<<16) / size.width()) * diff;
            p = image.scanLine(0);
            for(x = 0; x < size.width(); ++x){
                val += delta;
                *p++ = val >> 16;
            }
            p = image.scanLine(0);
            for(y = 1; y < size.height(); ++y)
                memcpy(image.scanLine(y), p, image.bytesPerLine());
        }
    }
    else{
        float delta, val=ca;

        unsigned int w = size.width(), h = size.height();
        unsigned char *xtable = new unsigned char[w];
        unsigned char *ytable = new unsigned char[h];
        w*=2, h*=2;

        if(eff == DiagonalGradient || eff == CrossDiagonalGradient){
            delta = (float)diff/w;
            int dir;
            for(x=0; x < size.width(); x++, val+=delta){
                dir = eff == DiagonalGradient? x : size.width() - x - 1;
                xtable[dir] = (unsigned char) val;
            }
            delta = (float)diff/h;
            val = 0;
            for(y = 0; y < size.height(); y++, val+=delta)
                ytable[y] = (unsigned char) val;

            for(y = 0; y < size.height(); y++){
                unsigned char *p = image.scanLine(y);
                for(x = 0; x < size.width(); x++)
                    *p++ = xtable[x] + ytable[y];
            }
        }
        else{
            int sign = diff>0? 1: -1;
            delta = (float)diff / size.width();
            val = (float)diff/2;
            for(x=0; x < size.width(); x++, val-=delta)
                xtable[x] = (unsigned char) qAbs((int)val);

            delta = (float)diff/size.height();
            val = (float)diff/2;
            for(y=0; y < size.height(); y++, val-=delta)
                ytable[y] = (unsigned char) qAbs((int)val);

            int w = (size.width()+1)>>1;
            int h = (size.height()+1)>>1;
            int x2;
            unsigned char *sl1, *sl2;
            for(y = 0; y < h; y++){
                sl1 = image.scanLine(y);
                sl2 = image.scanLine(qMax(size.height()-y-1, y));
                for(x = 0, x2 = size.width()-1; x < w; x++, x2--){
                    switch(eff){
                    case PyramidGradient:
                        idx = cb-sign*(xtable[x]+ytable[y]);
                        break;
                    case RectangleGradient:
                        idx = cb-sign*qMax(xtable[x], ytable[y])*2;
                        break;
                    case PipeCrossGradient:
                        idx = cb-sign*qMin(xtable[x], ytable[y])*2;
                        break;
                    case EllipticGradient:
                    default:
                        idx = cb - sign *
                            (int)std::sqrt((xtable[x]*xtable[x] +
                                            ytable[y]*ytable[y])*2.0f);
                        break;
                    }
                    sl1[x] = sl2[x] = idx;
                    sl1[x2] = sl2[x2] = idx;
                }
            }
        }
        delete [] xtable;
        delete [] ytable;
    }
    return(image);
}

QImage Tellico::unbalancedGradient(QSize size, const QColor &ca,
                                   const QColor &cb, Tellico::GradientType eff,
                                   int xfactor, int yfactor)
{
    QImage image(size, QImage::Format_RGB32);
    if(!size.isValid())
        return image;

    int dir; // general parameter used for direction switches
    bool _xanti = (xfactor < 0); // negative on X direction
    bool _yanti = (yfactor < 0); // negative on Y direction
    xfactor = qBound(1, qAbs(xfactor), 200);
    yfactor = qBound(1, qAbs(yfactor), 200);
    //    float xbal = xfactor/5000.;
    //    float ybal = yfactor/5000.;
    float xbal = xfactor/30.0f/size.width();
    float ybal = yfactor/30.0f/size.height();
    float rat;

    int x, y;
    int rca, gca, bca, rcb, gcb, bcb;
    int rDiff = (rcb = cb.red())   - (rca = ca.red());
    int gDiff = (gcb = cb.green()) - (gca = ca.green());
    int bDiff = (bcb = cb.blue())  - (bca = ca.blue());

    if(eff == VerticalGradient || eff == HorizontalGradient){
        QRgb *p;
        if(eff == VerticalGradient){
            QRgb rgbRow;
            for(y=0; y < size.height(); y++){
                dir = _yanti ? y : size.height() - 1 - y;
                rat =  1 - std::exp( - (float)y  * ybal );
                p = (QRgb *) image.scanLine(dir);
                rgbRow = qRgb(rcb - (int) ( rDiff * rat ),
                              gcb - (int) ( gDiff * rat ),
                              bcb - (int) ( bDiff * rat ));
                for(x = 0; x < size.width(); x++)
                    *p++ = rgbRow;
            }
        }
	else{
            p = (QRgb *)image.scanLine(0);
            for(x = 0; x < size.width(); x++){
                dir = _xanti ? x : size.width() - 1 - x;
                rat = 1 - std::exp( - (float)x  * xbal );
                p[dir] = qRgb(rcb - (int) ( rDiff * rat ),
                              gcb - (int) ( gDiff * rat ),
                              bcb - (int) ( bDiff * rat ));
            }

            p = (QRgb *)image.scanLine(0);
            for(y = 1; y < size.height(); ++y){
                memcpy(image.scanLine(y), p,
                       size.width()*sizeof(QRgb));
            }
        }
    }
    else{
        int w=size.width(), h=size.height();
        unsigned char *xtable = new unsigned char[w*3];
        unsigned char *ytable = new unsigned char[h*3];
        QRgb *p;

        if(eff == DiagonalGradient || eff == CrossDiagonalGradient){
            for(x = 0; x < w; x++){
                dir = _xanti ? x : w - 1 - x;
                rat = 1 - std::exp( - (float)x * xbal );

                xtable[dir*3] = (unsigned char) ( rDiff/2 * rat );
                xtable[dir*3+1] = (unsigned char) ( gDiff/2 * rat );
                xtable[dir*3+2] = (unsigned char) ( bDiff/2 * rat );
            }

            for(y = 0; y < h; y++){
                dir = _yanti ? y : h - 1 - y;
                rat =  1 - std::exp( - (float)y  * ybal );

                ytable[dir*3] = (unsigned char) ( rDiff/2 * rat );
                ytable[dir*3+1] = (unsigned char) ( gDiff/2 * rat );
                ytable[dir*3+2] = (unsigned char) ( bDiff/2 * rat );
            }

            for(y = 0; y < h; y++){
                p = (QRgb *)image.scanLine(y);
                for(x = 0; x < w; x++){
                    *p++ = qRgb(rcb - (xtable[x*3] + ytable[y*3]),
                                gcb - (xtable[x*3+1] + ytable[y*3+1]),
                                bcb - (xtable[x*3+2] + ytable[y*3+2]));
                }
            }
        }
        else{
            int rSign = rDiff>0? 1: -1;
            int gSign = gDiff>0? 1: -1;
            int bSign = bDiff>0? 1: -1;

            for(x = 0; x < w; x++){
                dir = _xanti ? x : w - 1 - x;
                rat =  1 - std::exp( - (float)x * xbal );

                xtable[dir*3] = (unsigned char) qAbs((int)(rDiff*(0.5-rat)));
                xtable[dir*3+1] = (unsigned char) qAbs((int)(gDiff*(0.5-rat)));
                xtable[dir*3+2] = (unsigned char) qAbs((int)(bDiff*(0.5-rat)));
            }

            for(y = 0; y < h; y++){
              dir = _yanti ? y : h - 1 - y;
              rat =  1 - std::exp( - (float)y * ybal );

              ytable[dir*3] = (unsigned char) qAbs((int)(rDiff*(0.5-rat)));
              ytable[dir*3+1] = (unsigned char) qAbs((int)(gDiff*(0.5-rat)));
              ytable[dir*3+2] = (unsigned char) qAbs((int)(bDiff*(0.5-rat)));
            }

            for(y = 0; y < h; y++){
                p = (QRgb *)image.scanLine(y);
                for(x = 0; x < w; x++) {
                    if (eff == PyramidGradient){
                        *p++ = qRgb(rcb-rSign*(xtable[x*3]+ytable[y*3]),
                                    gcb-gSign*(xtable[x*3+1]+ytable[y*3+1]),
                                    bcb-bSign*(xtable[x*3+2]+ytable[y*3+2]));
                    }
                    else if (eff == RectangleGradient){
                        *p++ = qRgb(rcb - rSign *
                                    qMax(xtable[x*3], ytable[y*3]) * 2,
                                    gcb - gSign *
                                    qMax(xtable[x*3+1], ytable[y*3+1]) * 2,
                                    bcb - bSign *
                                    qMax(xtable[x*3+2], ytable[y*3+2]) * 2);
                    }
                    else if (eff == PipeCrossGradient){
                        *p++ = qRgb(rcb - rSign *
                                    qMin(xtable[x*3], ytable[y*3]) * 2,
                                    gcb - gSign *
                                    qMin(xtable[x*3+1], ytable[y*3+1]) * 2,
                                    bcb - bSign *
                                    qMin(xtable[x*3+2], ytable[y*3+2]) * 2);
                    }
                    else if (eff == EllipticGradient){
                        *p++ = qRgb(rcb - rSign *
                                    (int)std::sqrt((xtable[x*3]*xtable[x*3] +
                                                    ytable[y*3]*ytable[y*3])*2.0),
                                    gcb - gSign *
                                    (int)std::sqrt((xtable[x*3+1]*xtable[x*3+1] +
                                                    ytable[y*3+1]*ytable[y*3+1])*2.0),
                                    bcb - bSign *
                                    (int)std::sqrt((xtable[x*3+2]*xtable[x*3+2] +
                                                    ytable[y*3+2]*ytable[y*3+2])*2.0));
                    }
                }
            }
        }
        delete [] xtable;
        delete [] ytable;
    }
    return(image);
}

QImage Tellico::grayUnbalancedGradient(QSize size, unsigned char ca,
                                       unsigned char cb, Tellico::GradientType eff,
                                       int xfactor, int yfactor)
{
    QImage image(size, QImage::Format_Indexed8);
    if(!size.isValid())
        return(image);
    QVector<QRgb> colorTable(256);
    for(int i=0; i < 256; ++i)
        colorTable[i] = qRgba(i, i, i, 255);
    image.setColorTable(colorTable);

    int dir; // general parameter used for direction switches
    bool _xanti = (xfactor < 0); // negative on X direction
    bool _yanti = (yfactor < 0); // negative on Y direction
    xfactor = qBound(1, qAbs(xfactor), 200);
    yfactor = qBound(1, qAbs(yfactor), 200);
    float xbal = xfactor/30.0f/size.width();
    float ybal = yfactor/30.0f/size.height();
    float rat;

    int x, y;
    int diff = cb-ca;

    if(eff == VerticalGradient || eff == HorizontalGradient){
        unsigned char *p;
        if(eff == VerticalGradient){
            unsigned char idx;
            for(y=0; y < size.height(); y++){
                dir = _yanti ? y : size.height() - 1 - y;
                rat =  1 - std::exp( - (float)y  * ybal );
                p = image.scanLine(dir);
                idx = cb - (int)( diff * rat );
                for(x = 0; x < size.width(); x++)
                    *p++ = idx;
            }
        }
	else{
            p = image.scanLine(0);
            for(x = 0; x < size.width(); x++){
                dir = _xanti ? x : size.width() - 1 - x;
                rat = 1 - std::exp( - (float)x  * xbal );
                p[dir] = cb - (int)( diff * rat );
            }

            p = image.scanLine(0);
            for(y = 1; y < size.height(); ++y)
                memcpy(image.scanLine(y), p, image.bytesPerLine());
        }
    }
    else{
        int w=size.width(), h=size.height();
        unsigned char *xtable = new unsigned char[w];
        unsigned char *ytable = new unsigned char[h];
        unsigned char *p;

        if(eff == DiagonalGradient || eff == CrossDiagonalGradient){
            for(x = 0; x < w; x++){
                dir = _xanti ? x : w - 1 - x;
                rat = 1 - std::exp( - (float)x * xbal );
                xtable[dir] = (unsigned char) ( diff/2 * rat );
            }

            for(y = 0; y < h; y++){
                dir = _yanti ? y : h - 1 - y;
                rat =  1 - std::exp( - (float)y  * ybal );
                ytable[dir] = (unsigned char) ( diff/2 * rat );
            }

            for(y = 0; y < h; y++){
                p = image.scanLine(y);
                for(x = 0; x < w; x++)
                    *p++ = cb - (xtable[x] + ytable[y]);
            }
        }
        else{
            int sign = diff>0? 1: -1;
            for(x = 0; x < w; x++){
                dir = _xanti ? x : w - 1 - x;
                rat =  1 - std::exp( - (float)x * xbal );
                xtable[dir] = (unsigned char) qAbs((int)(diff*(0.5-rat)));
            }

            for(y = 0; y < h; y++){
                dir = _yanti ? y : h - 1 - y;
                rat =  1 - std::exp( - (float)y * ybal );
                ytable[dir] = (unsigned char) qAbs((int)(diff*(0.5-rat)));
            }

            for(y = 0; y < h; y++){
                p = image.scanLine(y);
                for(x = 0; x < w; x++) {
                  if (eff == PyramidGradient)
                        *p++ = cb-sign*(xtable[x]+ytable[y]);
                    else if (eff == RectangleGradient)
                        *p++ = cb -sign*qMax(xtable[x], ytable[y])*2;
                    else if (eff == PipeCrossGradient)
                        *p++ = cb-sign*qMin(xtable[x], ytable[y])*2;
                    else if (eff == EllipticGradient)
                        *p++ = cb-sign * (int)std::sqrt((xtable[x]*xtable[x] +
                                                         ytable[y]*ytable[y])*2.0);
                }
            }
        }
        delete [] xtable;
        delete [] ytable;
    }
    return(image);
}
