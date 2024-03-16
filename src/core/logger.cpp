/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "logger.h"

namespace {
  const int MAX_LINES_BEFORE_FLUSH = 1000;
}
using Tellico::Logger;

Logger* Logger::self() {
  static Logger logger;
  return &logger;
}

Logger::Logger(QObject* parent_) : QObject(parent_) {
  m_oldHandler = qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
                                          Logger::self()->log(type, ctx, msg);
                                        });
  qSetMessagePattern(QStringLiteral("[%{time hh:mm:ss}] %{if-debug}[ %{file}:%{line} "
                                    " %{function} ] %{endif}%{if-warning}WARNING %{endif}%{message}"));
}

Logger::~Logger() {
  if(m_logStream) {
    m_logStream->flush();
  }
  qInstallMessageHandler(m_oldHandler);
}

void Logger::log(const QString& msg_) {
  static int lineCounter = 0;
  if(!m_logStream) return;

  QMutexLocker lock(&m_mutex);
  ++lineCounter;
  if(lineCounter > MAX_LINES_BEFORE_FLUSH) {
    lineCounter = 1;
    m_logStream->flush();
  }

  (*m_logStream) << msg_ << "\n";
  emit updated();
}

void Logger::log(QtMsgType type_, const QMessageLogContext& ctx_, const QString& msg_) {
  if(!m_logStream) {
    // if there's no logging output defined, revert to previous behavior
    // but swallow info messages if they're in Tellico's category
    if(type_ != QtInfoMsg || strcmp(ctx_.category, "tellico") != 0) {
      m_oldHandler(type_, ctx_, msg_);
     }
    return;
  }

  const auto& logMsg = qFormatLogMessage(type_, ctx_, msg_);
  log(logMsg);
  // also include default handling of warning and critical messages
  if(type_ != QtInfoMsg && type_ != QtDebugMsg) {
    m_oldHandler(type_, ctx_, msg_);
  }
}

void Logger::setLogFile(const QString& logFile_) {
  QMutexLocker lock(&m_mutex);
  if(m_logStream) {
    m_logStream.reset(nullptr);
    m_logFile.close();
  }

  if(logFile_.isEmpty()) {
    return;
  }

  if(logFile_ == QLatin1String("-")) {
    m_logFile.open(stdout, QIODevice::WriteOnly);
  } else {
    m_logFile.setFileName(logFile_);
    m_logFile.open(QIODevice::WriteOnly);
  }

  m_logStream.reset(new QTextStream(&m_logFile));
}

QString Logger::logFile() const {
  return m_logFile.fileName();
}

void Logger::flush() {
  if(m_logStream) m_logStream->flush();
}
