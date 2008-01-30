/***************************************************************************
    copyright            : (C) 2007 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "griffithimporter.h"
#include "../collections/videocollection.h"
#include "tellicoimporter.h"
#include "../tellico_debug.h"

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kprocess.h>

#include <qdir.h>
#include <qfile.h>

using Tellico::Import::GriffithImporter;

GriffithImporter::~GriffithImporter() {
  if(m_process) {
    m_process->kill();
    delete m_process;
    m_process = 0;
  }
}

Tellico::Data::CollPtr GriffithImporter::collection() {
  QString filename = QDir::homeDirPath() + QString::fromLatin1("/.griffith/griffith.db");
  if(!QFile::exists(filename)) {
    myWarning() << "GriffithImporter::collection() - database not found: " << filename << endl;
    return 0;
  }

  QString python = KStandardDirs::findExe(QString::fromLatin1("python"));
  if(python.isEmpty()) {
    myWarning() << "GriffithImporter::collection() - python not found!" << endl;
    return 0;
  }

  QString griffith = KGlobal::dirs()->findResource("appdata", QString::fromLatin1("griffith2tellico.py"));
  if(griffith.isEmpty()) {
    myWarning() << "GriffithImporter::collection() - griffith2tellico.py not found!" << endl;
    return 0;
  }

  m_process = new KProcess();
  connect(m_process, SIGNAL(receivedStdout(KProcess*, char*, int)), SLOT(slotData(KProcess*, char*, int)));
  connect(m_process, SIGNAL(receivedStderr(KProcess*, char*, int)), SLOT(slotError(KProcess*, char*, int)));
  connect(m_process, SIGNAL(processExited(KProcess*)), SLOT(slotProcessExited(KProcess*)));
  *m_process << python << griffith;
  if(!m_process->start(KProcess::Block, KProcess::AllOutput)) {
    myDebug() << "ExecExternalFetcher::startSearch() - process failed to start" << endl;
    return 0;
  }

  return m_coll;
}

void GriffithImporter::slotData(KProcess*, char* buffer_, int len_) {
  QDataStream stream(m_data, IO_WriteOnly | IO_Append);
  stream.writeRawBytes(buffer_, len_);
}

void GriffithImporter::slotError(KProcess*, char* buffer_, int len_) {
  QString msg = QString::fromLocal8Bit(buffer_, len_);
  myDebug() << "GriffithImporter::slotError() - " << msg << endl;
  setStatusMessage(msg);
}


void GriffithImporter::slotProcessExited(KProcess*) {
//  myDebug() << "GriffithImporter::slotProcessExited()" << endl;
  if(!m_process->normalExit() || m_process->exitStatus()) {
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
