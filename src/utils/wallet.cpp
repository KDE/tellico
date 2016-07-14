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

#include "wallet.h"
#include "guiproxy.h"

#include <KWallet>

#include <QWidget>

using Tellico::Wallet;

Tellico::Wallet* Wallet::self() {
  static Wallet wallet;
  return &wallet;
}

Wallet::Wallet() : m_wallet(0) {
}

bool Wallet::prepareWallet() {
  if(GUI::Proxy::widget() && (!m_wallet || !m_wallet->isOpen())) {
    delete m_wallet;
    m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), GUI::Proxy::widget()->effectiveWinId());
  }
  if(!m_wallet || !m_wallet->isOpen()) {
    delete m_wallet;
    m_wallet = 0;
    return false;
  }

  if(!m_wallet->hasFolder(KWallet::Wallet::PasswordFolder()) &&
     !m_wallet->createFolder(KWallet::Wallet::PasswordFolder())) {
    return false;
  }

  return m_wallet->setFolder(KWallet::Wallet::PasswordFolder());
}

QByteArray Wallet::readWalletEntry(const QString& key_) {
  QByteArray value;

  if(!prepareWallet()) {
    return value;
  }

  if(m_wallet->readEntry(key_, value) != 0) {
    return QByteArray();
  }

  return value;
}

QMap<QString, QString> Wallet::readWalletMap(const QString& key_) {
  QMap<QString, QString> map;

  if(!prepareWallet()) {
    return map;
  }

  if(m_wallet->readMap(key_, map) != 0) {
    return QMap<QString, QString>();
  }

  return map;
}

