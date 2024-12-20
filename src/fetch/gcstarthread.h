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

#ifndef TELLICO_GCSTARTHREAD_H
#define TELLICO_GCSTARTHREAD_H

#include <QThread>
#include <QStringList>

namespace Tellico {
  namespace Fetch {

class GCstarThread : public QThread {
Q_OBJECT

public:
  GCstarThread(QObject* obj);

  void setProgram(const QString& program, const QStringList& args);

protected:
  virtual void run() override;

public Q_SLOTS:
  void slotData();
  void slotError();

Q_SIGNALS:
  void standardOutput(const QByteArray& data);
  void standardError(const QByteArray& data);

private:
  QString m_program;
  QStringList m_args;
};

  }
}

#endif
