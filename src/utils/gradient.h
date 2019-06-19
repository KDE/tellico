#ifndef __BLITZ_H
#define __BLITZ_H

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

#include <QImage>

namespace Tellico {

    enum GradientType {VerticalGradient=0, HorizontalGradient, DiagonalGradient,
        CrossDiagonalGradient, PyramidGradient, RectangleGradient,
        PipeCrossGradient, EllipticGradient};

    /**
     * Create a gradient from color a to color b of the specified type.
     *
     * @param size The desired size of the gradient.
     * @param ca Color a
     * @param cb Color b
     * @param type The type of gradient.
     * @return The gradient.
     */
    QImage gradient(const QSize &size, const QColor &ca,
                    const QColor &cb, GradientType type);
    /**
     * Creates an 8bit grayscale gradient suitable for use as an alpha channel
     * using QImage::setAlphaChannel().
     *
     * @param size The desired size of the gradient.
     * @param ca The grayscale start value.
     * @param cb The grayscale end value.
     * @param type The type of gradient.
     * @return The gradient.
     */
    QImage grayGradient(const QSize &size, unsigned char ca,
                        unsigned char cb, GradientType type);
    /**
     * Create an unbalanced gradient. An unbalanced gradient is a gradient
     * where the transition from color a to color b is not linear, but in this
     * case exponential.
     *
     * @param size The desired size of the gradient.
     * @param ca Color a
     * @param cb Color b
     * @param type The type of gradient.
     * @param xfactor The x decay length. Use a value between -200 and 200.
     * @param yfactor The y decay length.
     * @return The gradient.
     */
    QImage unbalancedGradient(const QSize &size, const QColor &ca,
                              const QColor &cb, GradientType type,
                              int xfactor=100, int yfactor=100);
    /**
     * Creates an 8bit grayscale gradient suitable for use as an alpha channel
     * using QImage::setAlphaChannel().
     *
     * @param size The desired size of the gradient.
     * @param type The type of gradient.
     * @param ca The grayscale start value.
     * @param cb The grayscale end value.
     * @param xfactor The x decay length. Use a value between -200 and 200.
     * @param yfactor The y decay length.
     * @return The gradient.
     */
    QImage grayUnbalancedGradient(const QSize &size, unsigned char ca,
                                  unsigned char cb, GradientType type,
                                  int xfactor=100, int yfactor=100);
}

#endif
