/***************************************************************************
    Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>
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

#include "csvparser.h"

#include <QTextStream>
#include <QStringList>

#include <config.h>

extern "C" {
#ifdef HAVE_LIBCSV
#include <csv.h>
#else
#include "libcsv/libcsv.h"
#endif
}

typedef int(*SpaceFunc)(char);

static void writeToken(void* buffer, size_t len, void* data);
static void writeRow(int buffer, void* data);
static int isSpace(unsigned char c);
static int isSpaceOrTab(unsigned char c);
static int isTab(unsigned char c);

using Tellico::CSVParser;

class CSVParser::Private {
public:
  Private() {
    stream = 0;
    csv_init(&parser, 0);
  }
  ~Private() {
    csv_free(&parser);
    delete stream;
  }

  struct csv_parser parser;
  QString str;
  QTextStream* stream;
  QStringList tokens;
  bool done;
};

CSVParser::CSVParser(QString str) : d(new Private()) {
  reset(str);
}

CSVParser::~CSVParser() {
  delete d;
}

void CSVParser::setDelimiter(const QString& s) {
  Q_ASSERT(s.length() == 1);
  csv_set_delim(&d->parser, s[0].toLatin1());
  if(s[0] == QLatin1Char('\t'))     csv_set_space_func(&d->parser, isSpace);
  else if(s[0] == QLatin1Char(' ')) csv_set_space_func(&d->parser, isTab);
  else                              csv_set_space_func(&d->parser, isSpaceOrTab);
}

void CSVParser::reset(QString str) {
  delete d->stream;
  d->str = str;
  d->stream = new QTextStream(&d->str);
}

bool CSVParser::hasNext() const {
  return !d->stream->atEnd();
}

void CSVParser::skipLine() {
  d->stream->readLine();
}

void CSVParser::addToken(const QString& t) {
  d->tokens += t;
}

void CSVParser::setRowDone(bool b) {
  d->done = b;
}

QStringList CSVParser::nextTokens() {
  d->tokens.clear();
  d->done = false;
  while(hasNext() && !d->done) {
    QByteArray line = d->stream->readLine().toUtf8() + '\n'; // need the eol char
    csv_parse(&d->parser, line.constData(), line.length(), &writeToken, &writeRow, this);
  }
  csv_fini(&d->parser, &writeToken, &writeRow, this);
  return d->tokens;
}

static void writeToken(void* buffer, size_t len, void* data) {
  CSVParser* p = static_cast<CSVParser*>(data);
  p->addToken(QString::fromUtf8((char *)buffer, len));
}

static void writeRow(int c, void* data) {
  Q_UNUSED(c);
  CSVParser* p = static_cast<CSVParser*>(data);
  p->setRowDone(true);
}

static int isSpace(unsigned char c) {
  if (c == CSV_SPACE) return 1;
  return 0;
}

static int isSpaceOrTab(unsigned char c) {
  if (c == CSV_SPACE || c == CSV_TAB) return 1;
  return 0;
}

static int isTab(unsigned char c) {
  if (c == CSV_TAB) return 1;
  return 0;
}
