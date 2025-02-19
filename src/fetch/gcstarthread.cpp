/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#include "gcstarthread.h"

#include <KProcess>

using Tellico::Fetch::GCstarThread;

GCstarThread::GCstarThread(QObject* obj) : QThread(obj) {
}

void GCstarThread::setProgram(const QString& program_, const QStringList& args_) {
  m_program = program_;
  m_args = args_;
}

void GCstarThread::run() {
  KProcess proc;
  connect(&proc, &QProcess::readyReadStandardOutput, this, &GCstarThread::slotData);
  connect(&proc, &QProcess::readyReadStandardError, this, &GCstarThread::slotError);
  proc.setOutputChannelMode(KProcess::SeparateChannels);
  proc.setProgram(m_program, m_args);
  proc.execute();
}

void GCstarThread::slotData() {
  Q_EMIT standardOutput(static_cast<KProcess*>(sender())->readAllStandardOutput());
}

void GCstarThread::slotError() {
  Q_EMIT standardError(static_cast<KProcess*>(sender())->readAllStandardError());
}
