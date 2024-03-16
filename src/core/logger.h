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

#ifndef TELLICO_LOGGER_H
#define TELLICO_LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>

namespace Tellico {

class Logger : public QObject {
Q_OBJECT

public:
  static Logger* self();

  void log(const QString& msg);
  void log(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);
  void setLogFile(const QString& logFile);
  QString logFile() const;
  void flush();

Q_SIGNALS:
  void updated();

private:
  explicit Logger(QObject* parent = nullptr);
  virtual ~Logger();

  QtMessageHandler m_oldHandler;
  QFile m_logFile;
  QScopedPointer<QTextStream> m_logStream;
  mutable QMutex m_mutex;
};

} // end namespace
#endif
