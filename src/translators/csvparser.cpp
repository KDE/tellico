/***************************************************************************
    copyright            : (C) 2009 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "csvparser.h"

#include <QTextStream>
#include <QStringList>

extern "C" {
#include "libcsv/libcsv.h"
}

typedef int(*SpaceFunc)(char);

static void writeToken(char* buffer, size_t len, void* data);
static void writeRow(char buffer, void* data);
static int isSpace(char c);
static int isSpaceOrTab(char c);
static int isTab(char c);

using Tellico::CSVParser;

class CSVParser::Private {
public:
  Private(QString str) {
    stream = new QTextStream(&str);
    csv_init(&parser, 0);
  }
  ~Private() {
    csv_free(parser);
    delete stream;
  }

  struct csv_parser* parser;
  QTextStream* stream;
  QStringList tokens;
  bool done;
};

CSVParser::CSVParser(QString str) : d(new Private(str)) {
}

CSVParser::~CSVParser() {
  delete d;
}

void CSVParser::setDelimiter(const QString& s) {
  Q_ASSERT(s.length() == 1);
  csv_set_delim(d->parser, s[0].toLatin1());
  if(s[0] == '\t')     csv_set_space_func(d->parser, isSpace);
  else if(s[0] == ' ') csv_set_space_func(d->parser, isTab);
  else                 csv_set_space_func(d->parser, isSpaceOrTab);
}

void CSVParser::reset(QString str) {
  delete d->stream;
  d->stream = new QTextStream(&str);
};

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
    csv_parse(d->parser, line, line.length(), &writeToken, &writeRow, this);
  }
  csv_fini(d->parser, &writeToken, &writeRow, this);
  return d->tokens;
}

static void writeToken(char* buffer, size_t len, void* data) {
  CSVParser* p = static_cast<CSVParser*>(data);
  p->addToken(QString::fromUtf8(buffer, len));
}

static void writeRow(char c, void* data) {
  Q_UNUSED(c);
  CSVParser* p = static_cast<CSVParser*>(data);
  p->setRowDone(true);
}

static int isSpace(char c) {
  if (c == CSV_SPACE) return 1;
  return 0;
}

static int isSpaceOrTab(char c) {
  if (c == CSV_SPACE || c == CSV_TAB) return 1;
  return 0;
}

static int isTab(char c) {
  if (c == CSV_TAB) return 1;
  return 0;
}
