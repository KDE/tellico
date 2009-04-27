/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "griffithimporter.h"
#include "../collections/videocollection.h"
#include "tellicoimporter.h"
#include "../tellico_debug.h"

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kprocess.h>

#include <QDir>
#include <QFile>

using Tellico::Import::GriffithImporter;

GriffithImporter::~GriffithImporter() {
  if(m_process) {
    m_process->kill();
    delete m_process;
    m_process = 0;
  }
}

Tellico::Data::CollPtr GriffithImporter::collection() {
  QString filename = QDir::homePath() + QLatin1String("/.griffith/griffith.db");
  if(!QFile::exists(filename)) {
    myWarning() << "GriffithImporter::collection() - database not found: " << filename << endl;
    return Data::CollPtr();
  }

  QString python = KStandardDirs::findExe(QLatin1String("python"));
  if(python.isEmpty()) {
    myWarning() << "GriffithImporter::collection() - python not found!" << endl;
    return Data::CollPtr();
  }

  QString griffith = KGlobal::dirs()->findResource("appdata", QLatin1String("griffith2tellico.py"));
  if(griffith.isEmpty()) {
    myWarning() << "GriffithImporter::collection() - griffith2tellico.py not found!" << endl;
    return Data::CollPtr();
  }

  m_process = new KProcess();
  connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(slotData()));
  connect(m_process, SIGNAL(readyReadStandardError()), SLOT(slotError()));
  connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(slotProcessExited()));
  m_process->setProgram(python, QStringList(griffith));
  if(!m_process->execute()) {
    myDebug() << "ExecExternalFetcher::startSearch() - process failed to start" << endl;
    return Data::CollPtr();
  }

  return m_coll;
}

void GriffithImporter::slotData() {
  m_data.append(m_process->readAllStandardOutput());
}

void GriffithImporter::slotError() {
  QString msg = QString::fromLocal8Bit(m_process->readAllStandardError());
  myDebug() << "GriffithImporter::slotError() - " << msg << endl;
  setStatusMessage(msg);
}


void GriffithImporter::slotProcessExited() {
//  myDebug() << "GriffithImporter::slotProcessExited()" << endl;
  if(m_process->exitStatus() != QProcess::NormalExit || m_process->exitCode() != 0) {
    myDebug() << "GriffithImporter::slotProcessExited() - process did not exit successfully" << endl;
    return;
  }

  if(m_data.isEmpty()) {
    myDebug() << "GriffithImporter::slotProcessExited() - no data" << endl;
    return;
  }

  QString text = QString::fromUtf8(m_data, m_data.size());
  TellicoImporter imp(text);

  m_coll = imp.collection();
  if(!m_coll) {
    myDebug() << "GriffithImporter::slotProcessExited() - no collection pointer" << endl;
  } else {
    myLog() << "GriffithImporter::slotProcessExited() - results found: " << m_coll->entryCount() << endl;
  }
}

bool GriffithImporter::canImport(int type) const {
  return type == Data::Collection::Video;
}

#include "griffithimporter.moc"
