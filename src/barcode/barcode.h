/***************************************************************************
    Copyright (C) 2007-2009 Sebastian Held <sebastian.held@gmx.de>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  based on BaToo: http://www.vs.inf.ethz.ch/res/show.html?what=barcode   *
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

#ifndef BARCODE_H
#define BARCODE_H

#include "barcode_v4l.h"

#include <QThread>
#include <QImage>
#include <QVector>
#include <QMutex>

#include <math.h>

//#define BarcodeDecoder_DEBUG
//#define Decoder_EAN13_DEBUG

namespace barcodeRecognition {
  static int code_odd[][4] = { { 30, 20, 10, 10 },
                            { 20, 20, 20, 10 },
                            { 20, 10, 20, 20 },
                            { 10, 40, 10, 10 },
                            { 10, 10, 30, 20 },
                            { 10, 20, 30, 10 },
                            { 10, 10, 10, 40 },
                            { 10, 30, 10, 20 },
                            { 10, 20, 10, 30 },
                            { 30, 10, 10, 20 } };

  static int code_even[][4] = { { 10, 10, 20, 30 },
                             { 10, 20, 20, 20 },
                             { 20, 20, 10, 20 },
                             { 10, 10, 40, 10 },
                             { 20, 30, 10, 10 },
                             { 10, 30, 20, 10 },
                             { 40, 10, 10, 10 },
                             { 20, 10, 30, 10 },
                             { 30, 10, 20, 10 },
                             { 20, 10, 10, 30 } };

  static bool parity_pattern_list[][6] = { { false, false, false, false, false, false },
                                           { false, false, true, false, true, true },
                                           { false, false, true, true, false, true },
                                           { false, false, true, true, true, false },
                                           { false, true, false, false, true, true },
                                           { false, true, true, false, false, true },
                                           { false, true, true, true, false, false },
                                           { false, true, false, true, false, true },
                                           { false, true, false, true, true, false },
                                           { false, true, true, false, true, false } };

  class Barcode_EAN13 {
  public:
    Barcode_EAN13();
    Barcode_EAN13( QVector<int> code );
    bool isNull() const { return m_null; }
    bool isValid() const;
    QVector<int> getNumbers() const;
    void setCode( QVector<int> code );
    QString toString() const;
    bool operator!= ( const Barcode_EAN13 &code );
  protected:
    QVector<int> m_numbers;
    bool m_null;
  };

  class MatchMakerResult {
  public:
    MatchMakerResult( bool even, int digit );
    bool isEven() const {return m_even;}
    int getDigit() const {return m_digit;}
  protected:
    int m_digit;
    bool m_even;
  };

  class Decoder_EAN13 {
  public:
    enum { BOTH_TABLES = 0, EVEN_TABLE = 1, ODD_TABLE = 2 };
    static Barcode_EAN13 recognize( QVector< QVector<int> > fields );
    static QVector<int> decode( QVector< QVector<int> > fields, int start_i, int end_i );
    static MatchMakerResult recognizeNumber( QVector< QVector<int> > fields, int code_table_to_use );
    static MatchMakerResult recognizeSystemCode( bool parity_pattern[6] );
  };

  /** \brief this thread handles barcode recognition using webcams
   *  @author Sebastian Held <sebastian.held@gmx.de>
   */
  class barcodeRecognitionThread : public QThread {
    Q_OBJECT
  public:
    barcodeRecognitionThread();
    ~barcodeRecognitionThread();
    virtual void run() override;
    void stop();
    void recognizeBarcode( QImage img );
    bool isWebcamAvailable();
  QSize getPreviewSize() const;
  Q_SIGNALS:
    void recognized( QString barcode );
    void gotImage( const QImage &img );
  protected:
    volatile bool m_stop;
    QImage m_barcode_img;
    QMutex m_stop_mutex, m_barcode_img_mutex;
    barcode_v4l *m_barcode_v4l;

    Barcode_EAN13 recognize( QImage img );
    Barcode_EAN13 recognizeCode( QImage img, int x1, int x2, int y );
    void addNumberToPossibleNumbers( QVector<int> number, int possible_numbers[10][13][2], bool correct_code );
    void sortDigits( int possible_numbers[10][13][2] );
    Barcode_EAN13 extractBarcode( int possible_numbers[10][13][2] );
    QVector<int> transformPathToBW( QVector<QRgb> line);
    QVector< QVector<int> > extractFieldInformation( QVector<int> string );
    Barcode_EAN13 detectValidBarcode ( int possible_numbers[10][13][2], int max_amount_of_considered_codes );
    bool isValid( int numbers[13] );
    bool isValid( QVector<int> numbers );
    void printArray( int array[10][13][2], int level );
  };
}

#endif
