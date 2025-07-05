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

// includes code from https://www.linuxtv.org/downloads/v4l-dvb-apis-old/capture-example.html

#include "barcode.h"
#include "../tellico_debug.h"

#include <QImage>
#include <QMutex>

#include <stdlib.h>

using barcodeRecognition::barcodeRecognitionThread;
using barcodeRecognition::Barcode_EAN13;
using barcodeRecognition::Decoder_EAN13;
using barcodeRecognition::MatchMakerResult;

namespace {
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
                                           { false, false, true,  false, true,  true  },
                                           { false, false, true,  true,  false, true  },
                                           { false, false, true,  true,  true,  false },
                                           { false, true,  false, false, true,  true  },
                                           { false, true,  true,  false, false, true  },
                                           { false, true,  true,  true,  false, false },
                                           { false, true,  false, true,  false, true  },
                                           { false, true,  false, true,  true,  false },
                                           { false, true,  true,  false, true,  false } };
}

barcodeRecognitionThread::barcodeRecognitionThread()
{
  m_stop = false;
  m_barcode_v4l = new barcode_v4l();
}
barcodeRecognitionThread::~barcodeRecognitionThread()
{
  delete m_barcode_v4l;
}

bool barcodeRecognitionThread::isWebcamAvailable()
{
  return m_barcode_v4l->isOpen();
}

QSize barcodeRecognitionThread::getPreviewSize() const
{
  return QSize( 320,240 );
}

void barcodeRecognitionThread::run()
{
  bool stop;
  m_stop_mutex.lock();
  stop = m_stop;
  m_stop_mutex.unlock();

  if (!isWebcamAvailable())
    stop = true;

  Barcode_EAN13 old;
  while (!stop) {
    QImage img;
    m_barcode_img_mutex.lock();
    if (m_barcode_img.isNull())
      img = m_barcode_v4l->grab_one2();
    else {
      img = m_barcode_img;
      m_barcode_img = QImage();
    }
    m_barcode_img_mutex.unlock();

    if (!img.isNull()) {
      QImage preview = img.scaled( 320, 240, Qt::KeepAspectRatio );
      Q_EMIT gotImage( preview );
      Barcode_EAN13 barcode = recognize( img );
      if (barcode.isValid() && (old != barcode)) {
        Q_EMIT recognized( barcode.toString() );
        old = barcode;
      }
    }
    msleep( 10 ); // reduce load

    m_stop_mutex.lock();
    stop = m_stop;
    m_stop_mutex.unlock();
  }
}

void barcodeRecognitionThread::stop()
{
  // attention! This function is called from GUI context
  m_stop_mutex.lock();
  m_stop = true;
  m_stop_mutex.unlock();
}

void barcodeRecognitionThread::recognizeBarcode( QImage img )
{
  // attention! This function is called from GUI context
  m_barcode_img_mutex.lock();
  m_barcode_img = img;
  m_barcode_img_mutex.unlock();
}

Barcode_EAN13 barcodeRecognitionThread::recognize( QImage img )
{
  // PARAMETERS:
  int amount_scanlines = 30;
  int w = img.width();
  int h = img.height();

  // the array which will contain the result:
  QVector< QVector<int> > numbers( amount_scanlines, QVector<int>(13,-1) ); // no init in java source!!!!!!!!!

  // generate and initialize the array that will contain all detected
  // digits at a specific code position:
  int possible_numbers[10][13][2];
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 13; j++) {
      possible_numbers[i][j][0] = -1;
      possible_numbers[i][j][1] = 0;
    }
  }

  // try to detect the barcode along scanlines:
  for (int i = 0; i < amount_scanlines; i++) {
    int x1 = 0;
    int y = (h / amount_scanlines) * i;
    int x2 = w - 1;

    // try to recognize a barcode along that path:
    Barcode_EAN13 ean13_code = recognizeCode( img, x1, x2, y );
    numbers[i] = ean13_code.getNumbers();

    if (ean13_code.isValid()) {
      // add the recognized digits to the array of possible numbers:
      addNumberToPossibleNumbers( numbers[i], possible_numbers, true );
    } else {
      numbers[i] = ean13_code.getNumbers();
      // add the recognized digits to the array of possible numbers:
      addNumberToPossibleNumbers(numbers[i], possible_numbers, false);
    }

#ifdef BarcodeDecoder_DEBUG
    // show the information that has been recognized along the scanline:
    qDebug( "Scanline %i result: %s\n", i, ean13_code.toString().latin1() );
#endif
  }

  // sort the detected digits at each code position, in accordance to the
  // amount of their detection:
  sortDigits(possible_numbers);

#ifdef BarcodeDecoder_DEBUG
  fprintf( stderr, "detected digits:\n" );
  printArray( possible_numbers, 0 );
  fprintf( stderr, "# of their occurrence:\n" );
  printArray( possible_numbers, 1 );
#endif

  // get the most likely barcode:
  Barcode_EAN13 code = extractBarcode(possible_numbers);

  return code;
}

void barcodeRecognitionThread::printArray( int array[10][13][2], int level )
{
  for (int i = 0; i < 10; i++) {
    QString temp;
    temp = QString::number( i ) + QLatin1String(" :   ");
    for (int j = 0; j < 13; j++) {
      if (array[i][j][level] == -1)
        temp += QLatin1String("x  ");
      else
      temp += QString::number( array[i][j][level] ) + QLatin1String("  ");
    }
    myDebug() << "printArray:" << temp;
  }
}

barcodeRecognition::Barcode_EAN13 barcodeRecognitionThread::recognizeCode( QImage img, int x1, int x2, int y )
{
  QVector<QRgb> raw_path(x2-x1+1);
  for (int x=x1; x<=x2; x++)
    raw_path[x-x1] = img.pixel(x,y);
  // convert the given path into a string of black and white pixels:
  QVector<int> string = transformPathToBW( raw_path );

  // convert the string of black&white pixels into a list, containing
  // information about the black and white fields
  // first index = field nr.
  // second index: 0 = color of the field
  //               1 = field length
  QVector< QVector<int> > fields = extractFieldInformation( string );

  // try to recognize a EAN13 code:
  Barcode_EAN13 barcode = Decoder_EAN13::recognize( fields );

  return barcode;
}

void barcodeRecognitionThread::addNumberToPossibleNumbers( QVector<int> number, int possible_numbers[10][13][2], bool correct_code )
{
  int i;
  bool digit_contained;
  for (int j = 0; j < 13; j++) {
    if (number[j] >= 0) {
      i = 0;
      digit_contained = false;
      while ((i < 10) && (possible_numbers[i][j][0] >= 0)) {
        if (possible_numbers[i][j][0] == number[j]) {
          digit_contained = true;
          if (correct_code)
            possible_numbers[i][j][1] = possible_numbers[i][j][1] + 100;
          else
            possible_numbers[i][j][1]++;
          break;
        }
        i++;
      }
      if ((i < 10) && (!digit_contained)) {
        // add new digit:
        possible_numbers[i][j][0] = number[j];
        if (correct_code)
          possible_numbers[i][j][1] = possible_numbers[i][j][1] + 100;
        else
          possible_numbers[i][j][1]++;
      }
    }
  }
}

void barcodeRecognitionThread::sortDigits( int possible_numbers[10][13][2] )
{
  int i;
  int temp_value;
  int temp_occurrence;
  bool changes;

  for (int j = 0; j < 13; j++) {
    i = 1;
    changes = false;
    while (true) {
      if ((possible_numbers[i - 1][j][0] >= 0) && (possible_numbers[i][j][0] >= 0)) {
        if (possible_numbers[i - 1][j][1] < possible_numbers[i][j][1]) {
          temp_value = possible_numbers[i - 1][j][0];
          temp_occurrence = possible_numbers[i - 1][j][1];
          possible_numbers[i - 1][j][0] = possible_numbers[i][j][0];
          possible_numbers[i - 1][j][1] = possible_numbers[i][j][1];
          possible_numbers[i][j][0] = temp_value;
          possible_numbers[i][j][1] = temp_occurrence;

          changes = true;
        }
      }

      if ((i >= 9) || (possible_numbers[i][j][0] < 0)) {
        if (!changes)
          break;
        else {
          i = 1;
          changes = false;
        }
      } else
        i++;
    }
  }
}

Barcode_EAN13 barcodeRecognitionThread::extractBarcode( int possible_numbers[10][13][2] )
{
  // create and initialize the temporary variables:
  QVector<int> temp_code(13);
  for (int i = 0; i < 13; i++)
    temp_code[i] = possible_numbers[0][i][0];

#ifdef Barcode_DEBUG
  fprintf( stderr, "barcodeRecognitionThread::extractBarcode(): " );
  for (int i=0; i<13; i++)
    fprintf( stderr, "%i", temp_code[i] );
  fprintf( stderr, "\n" );
#endif

  return Barcode_EAN13(temp_code);
}

Barcode_EAN13 barcodeRecognitionThread::detectValidBarcode ( int possible_numbers[10][13][2], int max_amount_of_considered_codes )
{
  // create and initialize the temporary variables:
  QVector<int> temp_code(13);
  for ( int i = 0; i < 13; i++ )
    temp_code[i] = possible_numbers[0][i][0];

  int alternative_amount = 0;

  QVector<int> counter( 13 ); // no init in java source!!!
  int counter_nr = 11;

  // check if there is at least one complete code present:
  for ( int i = 0; i < 13; i++ ) {
    // exit and return the "most likely" code parts:
    if ( temp_code[i] < 0 )
      return Barcode_EAN13( temp_code );
  }

  // if there is at least one complete node, try to detect a valid barcode:
  while ( alternative_amount < max_amount_of_considered_codes ) {
    // fill the temporary code array with one possible version:
    for ( int i = 0; i < 13; i++ )
      temp_code[i] = possible_numbers[counter[i]][i][0];

    alternative_amount++;

    // check if this version represents a valid code:
    if (isValid( temp_code ))
      return Barcode_EAN13( temp_code );

    // increment the counters:
    if ( ( counter[counter_nr] < 9 ) && ( possible_numbers[counter[counter_nr] + 1][counter_nr][0] >= 0 ) ) {
      // increment the actual counter.
      counter[counter_nr]++;
    } else {
      // check if we have reached the end and no valid barcode has been found:
      if ( counter_nr == 1 ) {
        // exit and return the "most likely" code parts:
        for ( int i = 0; i < 13; i++ )
          temp_code[i] = possible_numbers[0][i][0];
        return Barcode_EAN13( temp_code );
      } else {
        // reset the actual counter and increment the next one(s):
        counter[counter_nr] = 0;

        while ( true ) {
          if ( counter_nr > 2 )
            counter_nr--;
          else {
            for ( int i = 0; i < 13; i++ )
              temp_code[i] = possible_numbers[0][i][0];
            return Barcode_EAN13( temp_code );
          }
          if ( counter[counter_nr] < 9 ) {
            counter[counter_nr]++;
            if ( possible_numbers[counter[counter_nr]][counter_nr][0] < 0 )
              counter[counter_nr] = 0;
            else
              break;
          } else
            counter[counter_nr] = 0;
        }
        counter_nr = 12;
      }
    }
  }

  for ( int i = 0; i < 13; i++ )
    temp_code[i] = possible_numbers[0][i][0];
  return Barcode_EAN13( temp_code );
}

bool barcodeRecognitionThread::isValid( int numbers[13] )
{
  QVector<int> temp(13);
  for (int i=0; i<13; i++)
    temp[i] = numbers[i];
  return isValid( temp );
}
bool barcodeRecognitionThread::isValid( QVector<int> numbers )
{
  Q_ASSERT( numbers.count() == 13 );
  // calculate the checksum of the barcode:
  int sum1 = numbers[0] + numbers[2] + numbers[4] + numbers[6] + numbers[8] + numbers[10];
  int sum2 = 3 * (numbers[1] + numbers[3] + numbers[5] + numbers[7] + numbers[9] + numbers[11]);
  int checksum_value = sum1 + sum2;
  int checksum_digit = 10 - (checksum_value % 10);
  if (checksum_digit == 10)
    checksum_digit = 0;

#ifdef Barcode_DEBUG
  fprintf( stderr, "barcodeRecognitionThread::isValid(): " );
  for (int i=0; i<13; i++)
    fprintf( stderr, "%i", numbers[i] );
  fprintf( stderr, "\n" );
#endif

  return (numbers[12] == checksum_digit);
}

QVector<int> barcodeRecognitionThread::transformPathToBW( QVector<QRgb> line )
{
  int w = line.count();
  QVector<int> bw_line(w,0);
  bw_line[0] = 255;

  // create greyscale values:
  QVector<int> grey_line(w,0);
  int average_illumination = 0;
  for (int x = 0; x < w; x++) {
    grey_line[x] = (qRed(line.at(x)) + qGreen(line.at(x)) + qBlue(line.at(x))) / 3;
    average_illumination = average_illumination + grey_line[x];
  }
  average_illumination = average_illumination / w;

  // perform the binarization:
  int range = w / 20;

  // temp values:
  int moving_sum;
  int moving_average;
  int v1_index = -range + 1;
  int v2_index = range;
  int v1 = grey_line[0];
  int v2 = grey_line[range];
  int current_value;
  int comparison_value;

  // initialize the moving sum:
  moving_sum = grey_line[0] * range;
  for (int i = 0; i < range; i++)
    moving_sum = moving_sum + grey_line[i];

  // apply the adaptive thresholding algorithm:
  for (int i = 1; i < w - 1; i++) {
    if (v1_index > 0) v1 = grey_line[v1_index];
    if (v2_index < w) v2 = grey_line[v2_index];
    else v2 = grey_line[w - 1];
    moving_sum = moving_sum - v1 + v2;
    moving_average = moving_sum / (range << 1);
    v1_index++;
    v2_index++;

    current_value = (grey_line[i - 1] + grey_line[i]) >> 1;

    // decide if the current pixel should be black or white:
    comparison_value = (3 * moving_average + average_illumination) >> 2;
    if ((current_value < comparison_value - 3)) bw_line[i] = 0;
    else bw_line[i] = 255;
  }

  // filter the values: (remove too small fields)

  if (w >= 640) {
    for (int x = 1; x < w - 1; x++) {
      if ((bw_line[x] != bw_line[x - 1]) && (bw_line[x] != bw_line[x + 1])) bw_line[x] = bw_line[x - 1];
    }
  }

  QVector<int> ret(w,0);
  for (int i=0; i<w; i++)
    ret[i] = bw_line[i];

#ifdef Barcode_DEBUG
  fprintf( stderr, "barcodeRecognitionThread::transformPathToBW(): " );
  for (int i=0; i<ret.count(); i++)
    if (bw_line[i] == 0)
      fprintf( stderr, "0" );
    else
      fprintf( stderr, "#" );
  fprintf( stderr, "\n" );
#endif

  return ret;
}

QVector< QVector<int> > barcodeRecognitionThread::extractFieldInformation( QVector<int> string )
{
  QVector< QVector<int> > temp_fields( string.count(), QVector<int>(2,0) );

  if (string.count() == 0)
    return QVector< QVector<int> >();

  int field_counter = 0;
  int last_value = string.at(0);
  int last_fields = 1;
  for (int i = 1; i < string.size(); i++) {
    if ((string.at(i) == last_value) && (i < string.size() - 1)) {
      last_fields++;
    } else {
      // create new field entry:
      temp_fields[field_counter][0] = last_value;
      temp_fields[field_counter][1] = last_fields;

      last_value = string.at(i);
      last_fields = 0;
      field_counter++;
    }
  }

  temp_fields.resize( field_counter );

#ifdef Barcode_DEBUG
  fprintf( stderr, "barcodeRecognitionThread::extractFieldInformation(): " );
  for (int i=0; i<temp_fields.count(); i++)
    fprintf( stderr, "%i,%i ", temp_fields.at(i).at(0), temp_fields.at(i).at(1) );
  fprintf( stderr, "\n" );
#endif

  return temp_fields;
}

//ok
Barcode_EAN13::Barcode_EAN13() : m_numbers(13,-1)
{
  m_null = true;
}

//ok
Barcode_EAN13::Barcode_EAN13( QVector<int> code )
{
  setCode( code );
}

//ok
void Barcode_EAN13::setCode( QVector<int> code )
{
  if (code.count() != 13) {
    m_numbers.clear();
    m_numbers.insert(0,13,-1);
    m_null = true;
    return;
  }
  m_numbers = code;
  m_null = false;
}

//ok
bool Barcode_EAN13::isValid() const
{
  if (m_null)
    return false;

  for (int i = 0; i < 13; i++)
    if ((m_numbers[i] < 0) || (m_numbers[i] > 9))
      return false;

  // calculate the checksum of the barcode:
  int sum1 = m_numbers[0] + m_numbers[2] + m_numbers[4] + m_numbers[6] + m_numbers[8] + m_numbers[10];
  int sum2 = 3 * (m_numbers[1] + m_numbers[3] + m_numbers[5] + m_numbers[7] + m_numbers[9] + m_numbers[11]);
  int checksum_value = sum1 + sum2;
  int checksum_digit = 10 - (checksum_value % 10);
  if (checksum_digit == 10)
    checksum_digit = 0;

  return (m_numbers[12] == checksum_digit);
}

//ok
QVector<int> Barcode_EAN13::getNumbers() const
{
  return m_numbers;
}

//ok
QString Barcode_EAN13::toString() const
{
  QString s;
  for (int i = 0; i < 13; i++)
    if ((m_numbers[i] >= 0) && (m_numbers[i] <= 9))
      s += QString::number(m_numbers[i]);
    else
      s += QChar::fromLatin1('?');
  return s;
}

//ok
bool Barcode_EAN13::operator!= ( const Barcode_EAN13 &code )
{
  if (m_null != code.m_null)
    return true;
  if (!m_null)
    for (int i=0; i<13; i++)
      if (m_numbers[i] != code.m_numbers[i])
        return true;
  return false;
}

//ok
Barcode_EAN13 Decoder_EAN13::recognize( QVector< QVector<int> > fields )
{
  // try to extract the encoded information from the field series:
  QVector<int> numbers = decode( fields, 0, fields.count() );
  Barcode_EAN13 barcode( numbers );

  // return the results:
  return barcode;
}

QVector<int> Decoder_EAN13::decode( QVector< QVector<int> > fields, int start_i, int end_i )
{
  // determine the length of the path in pixels
  int length = 0;
  for (int i = 0; i < fields.size(); i++)
    length += fields.at(i).at(1);

  // set the parameters accordingly:
  int max_start_sentry_bar_differences;
  int max_unit_length;
  int min_unit_length;

  if (length <= 800) {
      max_start_sentry_bar_differences = 6;
      max_unit_length = 10;
      min_unit_length = 1;
  } else {
      max_start_sentry_bar_differences = 30;
      max_unit_length = 50;
      min_unit_length = 1;
  }

  // consistency checks:
  if (fields.count() <= 0)
    return QVector<int>();
  if (start_i > end_i - 3)
    return QVector<int>();
  if (end_i - start_i < 30)
    return QVector<int>(); // (just a rough value)

  // relevant indexes:
  int start_sentinel_i;
  int end_sentinel_i;
  int left_numbers_i;
  int middle_guard_i;
  int right_numbers_i;

  // results:
  QVector<int> numbers( 13, -1 ); // the java source does no initialization

  // determine the relevant positions:

  // Try to detect the start sentinel (a small black-white-black serie):
  start_sentinel_i = -1;
  for (int i = start_i; i < end_i - 56; i++) {
    if (fields[i][0] == 0) {
      if ((fields[i][1] >= min_unit_length) && (fields[i][1] <= max_unit_length)) {
        if ((qAbs(fields[i][1] - fields[i + 1][1]) <= max_start_sentry_bar_differences)
                  && (qAbs(fields[i][1] - fields[i + 2][1]) <= max_start_sentry_bar_differences) && (fields[i + 3][1] < fields[i][1] << 3)) {
          start_sentinel_i = i;
          break;
        }
      }
    }
  }

#ifdef Decoder_EAN13_DEBUG
  fprintf( stderr, "start_sentinal_index: %i\n", start_sentinel_i );
#endif

  if (start_sentinel_i < 0)
    return QVector<int>();

  // calculate the other positions:
  left_numbers_i = start_sentinel_i + 3;
  middle_guard_i = left_numbers_i + 6 * 4;
  right_numbers_i = middle_guard_i + 5;
  end_sentinel_i = right_numbers_i + 6 * 4;

  if (end_sentinel_i + 3 > end_i)
    return QVector<int>();

  QVector< QVector<int> > current_number_field( 4, QVector<int>(2,0) );

  if (left_numbers_i + 1 > end_i)
    return QVector<int>();

  // test the side from which we are reading the barcode:
  for (int j = 0; j < 4; j++) {
    current_number_field[j][0] = fields[left_numbers_i + j][0];
    current_number_field[j][1] = fields[left_numbers_i + j][1];
  }
  MatchMakerResult matchMakerResult = recognizeNumber( current_number_field, BOTH_TABLES );

  if (matchMakerResult.isEven()) {
    // we are reading the barcode from the back side:

    // use the already obtained information:
    numbers[12] = matchMakerResult.getDigit();

    // try to recognize the "right" numbers:
    int counter = 11;
    for (int i = left_numbers_i + 4; i < left_numbers_i + 24; i = i + 4) {
            for (int j = 0; j < 4; j++) {
                    current_number_field[j][0] = fields[i + j][0];
                    current_number_field[j][1] = fields[i + j][1];
            }
            matchMakerResult = recognizeNumber(current_number_field, EVEN_TABLE);
            numbers[counter] = matchMakerResult.getDigit();
            counter--;
    }

    bool parity_pattern[6];  // true = even, false = odd

    //(counter has now the value 6)

    // try to recognize the "left" numbers:
    for (int i = right_numbers_i; i < right_numbers_i + 24; i = i + 4) {
      for (int j = 0; j < 4; j++) {
        current_number_field[j][0] = fields[i + j][0];
        current_number_field[j][1] = fields[i + j][1];
      }
      matchMakerResult = recognizeNumber(current_number_field, BOTH_TABLES);
      numbers[counter] = matchMakerResult.getDigit();
      parity_pattern[counter-1] = !matchMakerResult.isEven();
      counter--;
    }

    // try to determine the system code:
    matchMakerResult = recognizeSystemCode(parity_pattern);
    numbers[0] = matchMakerResult.getDigit();
  } else {
    // we are reading the barcode from the "correct" side:

    bool parity_pattern[6];  // true = even, false = odd

    // use the already obtained information:
    numbers[1] = matchMakerResult.getDigit();
    parity_pattern[0] = matchMakerResult.isEven();

    // try to recognize the left numbers:
    int counter = 2;
    for (int i = left_numbers_i + 4; i < left_numbers_i + 24; i = i + 4) {
      for (int j = 0; j < 4; j++) {
        current_number_field[j][0] = fields[i + j][0];
        current_number_field[j][1] = fields[i + j][1];
      }
      matchMakerResult = recognizeNumber(current_number_field, BOTH_TABLES);
      numbers[counter] = matchMakerResult.getDigit();
      parity_pattern[counter-1] = matchMakerResult.isEven();
      counter++;
    }

    // try to determine the system code:
    matchMakerResult = recognizeSystemCode(parity_pattern);
    numbers[0] = matchMakerResult.getDigit();

    // try to recognize the right numbers:
    counter = 0;
    for (int i = right_numbers_i; i < right_numbers_i + 24; i = i + 4) {
      for (int j = 0; j < 4; j++) {
        current_number_field[j][0] = fields[i + j][0];
        current_number_field[j][1] = fields[i + j][1];
      }
      matchMakerResult = recognizeNumber(current_number_field, ODD_TABLE);
      numbers[counter + 7] = matchMakerResult.getDigit();
      counter++;
    }
  }

  return numbers;
}

MatchMakerResult Decoder_EAN13::recognizeNumber( QVector< QVector<int> > fields, int code_table_to_use)
{
  // convert the pixel lengths of the four black&white fields into
  // normed values that have together a length of 70;
  int pixel_sum = fields[0][1] + fields[1][1] + fields[2][1] + fields[3][1];
  int b[4];
  for (int i = 0; i < 4; i++) {
    b[i] = ::round((((float) fields[i][1]) / ((float) pixel_sum)) * 70);
  }

#ifdef Decoder_EAN13_DEBUG
  fprintf( stderr, "Recognize Number (code table to use: %i):\n", code_table_to_use );
  fprintf( stderr, "lengths: %i %i %i %i\n", fields[0][1], fields[1][1], fields[2][1], fields[3][1] );
  fprintf( stderr, "normed lengths: %i %i %i %i\n", b[0], b[1], b[2], b[3] );
#endif

  // try to detect the digit that is encoded by the set of four normed bar lengths:
  int max_difference_for_acceptance = 60;
  int temp;

  int even_min_difference = 100000;
  int even_min_difference_index = 0;
  int odd_min_difference = 100000;
  int odd_min_difference_index = 0;

  if ((code_table_to_use == BOTH_TABLES)||(code_table_to_use == EVEN_TABLE)) {
    QVector<int> even_differences(10,0);

    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 4; j++) {
        // calculate the differences in the even group:
        temp = b[j] - code_even[i][j];
        if (temp < 0)
          even_differences[i] = even_differences[i] + ((-temp) << 1);
        else
          even_differences[i] = even_differences[i] + (temp << 1);
      }
      if (even_differences[i] < even_min_difference) {
        even_min_difference = even_differences[i];
        even_min_difference_index = i;
      }
    }
  }

  if ((code_table_to_use == BOTH_TABLES) || (code_table_to_use == ODD_TABLE)) {
    QVector<int> odd_differences(10,0);

    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 4; j++) {
        // calculate the differences in the odd group:
        temp = b[j] - code_odd[i][j];
        if (temp < 0)
          odd_differences[i] = odd_differences[i] + ((-temp) << 1);
        else
          odd_differences[i] = odd_differences[i] + (temp << 1);
      }
      if (odd_differences[i] < odd_min_difference) {
        odd_min_difference = odd_differences[i];
        odd_min_difference_index = i;
      }
    }
  }

  // select the digit and parity with the lowest difference to the found pattern:
  if (even_min_difference <= odd_min_difference) {
    if (even_min_difference < max_difference_for_acceptance)
      return MatchMakerResult( true, even_min_difference_index );
  } else {
    if (odd_min_difference < max_difference_for_acceptance)
      return MatchMakerResult( false, odd_min_difference_index );
  }

  return MatchMakerResult( false, -1 );
}

MatchMakerResult Decoder_EAN13::recognizeSystemCode( bool parity_pattern[6] )
{
  // search for a fitting parity pattern:
  bool fits = false;
  for (int i = 0; i < 10; i++) {
    fits = true;
    for (int j = 0; j < 6; j++) {
      if (parity_pattern_list[i][j] != parity_pattern[j]) {
        fits = false;
        break;
      }
    }
    if (fits)
      return MatchMakerResult( false, i );
  }

  return MatchMakerResult( false, -1 );
}

//ok
MatchMakerResult::MatchMakerResult( bool even, int digit )
{
  m_even = even;
  m_digit = digit;
}
