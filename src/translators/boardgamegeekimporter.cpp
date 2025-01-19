/***************************************************************************
    Copyright (C) 2014 Robby Stephenson <robby@periapsis.org>
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

#include "boardgamegeekimporter.h"
#include "../collections/boardgamecollection.h"
#include "xslthandler.h"
#include "tellicoimporter.h"
#include "../core/filehandler.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <QLineEdit>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QDomDocument>
#include <QFile>
#include <QApplication>
#include <QUrlQuery>
#include <QThread>
#include <QElapsedTimer>

namespace {
  static const char* BGG_THING_URL  = "http://boardgamegeek.com/xmlapi2/thing";
  static const char* BGG_COLLECTION_URL = "http://boardgamegeek.com/xmlapi2/collection";
  static int BGG_STEPSIZE = 25;
}

using Tellico::Import::BoardGameGeekImporter;

BoardGameGeekImporter::BoardGameGeekImporter() : Import::Importer(), m_cancelled(false), m_widget(nullptr)
    , m_userEdit(nullptr), m_checkOwned(nullptr) {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("boardgamegeek2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    m_xsltURL = QUrl::fromLocalFile(xsltFile);
  } else {
    myWarning() << "unable to find boardgamegeek2tellico.xsl!";
  }

  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - BoardGameGeek"));
  m_user = config.readEntry("User ID");
  m_ownedOnly = config.readEntry("Owned", false);
}

bool BoardGameGeekImporter::canImport(int type) const {
  return type == Data::Collection::BoardGame;
}

Tellico::Data::CollPtr BoardGameGeekImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  if(m_xsltURL.isEmpty() || !m_xsltURL.isValid()) {
    setStatusMessage(i18n("A valid XSLT file is needed to import the file."));
    return Data::CollPtr();
  }

  if(!m_widget) {
    myWarning() << "no widget!";
    return Data::CollPtr();
  }

  m_user = m_userEdit->text().trimmed();
  if(m_user.isEmpty()) {
    setStatusMessage(i18n("A valid user ID must be entered."));
    return Data::CollPtr();
  }

  XSLTHandler handler(m_xsltURL);
  if(!handler.isValid()) {
    setStatusMessage(i18n("Tellico encountered an error in XSLT processing."));
    return Data::CollPtr();
  }

  m_ownedOnly = m_checkOwned->isChecked();

  // first get the bgg id list
  QUrl u(QString::fromLatin1(BGG_COLLECTION_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("username"), m_user);
  q.addQueryItem(QStringLiteral("subtype"), QStringLiteral("boardgame"));
  q.addQueryItem(QStringLiteral("brief"), QStringLiteral("1"));
  if(m_ownedOnly) {
    q.addQueryItem(QStringLiteral("own"), QStringLiteral("1"));
  }
  u.setQuery(q);

  QStringList idList;
  QDomDocument dom = FileHandler::readXMLDocument(u, false, true);
  // could return HTTP 202 while the caching system generates the file
  // see https://boardgamegeek.com/thread/1188687/export-collections-has-been-updated-xmlapi-develop
  // also has a root node of message. Try 5 times, waiting by 2 seconds each time
  bool hasMessage = dom.documentElement().tagName() == QStringLiteral("message");
  for(int loopCount = 0; hasMessage && loopCount < 5; ++loopCount) {
    // wait 2 seconds and try again
    QElapsedTimer timer;
    timer.start();
    while(timer.elapsed() < 2000) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
      QThread::msleep(500);
    }
    dom = FileHandler::readXMLDocument(u, false, true);
    hasMessage = dom.documentElement().tagName() == QStringLiteral("message");
  }

  if(hasMessage) {
    myDebug() << "BoardGameGeekImporter still has message and no collection";
#if 0
    myWarning() << "Remove debug from boardgamegeekimporter.cpp";
    QFile f(QStringLiteral("/tmp/bgg-message.xml"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t << dom.toString();
    }
    f.close();
#endif
  }

  QDomNodeList items = dom.documentElement().elementsByTagName(QStringLiteral("item"));
  for(int i = 0; i < items.count(); ++i) {
    if(!items.at(i).isElement()) {
      continue;
    }
    const QString id = items.at(i).toElement().attribute(QStringLiteral("objectid"));
    if(!id.isEmpty()) {
      idList += id;
    }
  }

  if(idList.isEmpty()) {
    myLog() << "No items found in BGG collection";
    return Data::CollPtr();
  }

  const bool showProgress = options() & ImportProgress;
  if(showProgress) {
    // use 10 as the amount for reading the ids
    Q_EMIT signalTotalSteps(this, 10 + 100);
    Q_EMIT signalProgress(this, 10);
  }

  m_coll = new Data::BoardGameCollection(true);

  for(int j = 0; j < idList.size() && !m_cancelled; j += BGG_STEPSIZE) {
    QStringList ids;
    const int maxSize = qMin(j+BGG_STEPSIZE, idList.size());
    for(int k = j; k < maxSize; ++k) {
      ids += idList.at(k);
    }

#if 0
    const QString xmlData = text(ids);
    myWarning() << "Remove debug from boardgamegeekimporter.cpp";
    QFile f(QStringLiteral("/tmp/test.xml"));
    if(f.open(QIODevice::WriteOnly)) {
      QTextStream t(&f);
      t << xmlData;
    }
    f.close();
#endif

    QString str = handler.applyStylesheet(text(ids));
//    QString str = handler.applyStylesheet(xmlData);
    //  myDebug() << str;
#if 0
    myWarning() << "Remove debug2 from boardgamegeekimporter.cpp";
    QFile f2(QStringLiteral("/tmp/test.tc"));
    if(f2.open(QIODevice::WriteOnly)) {
      QTextStream t(&f2);
      t << str;
    }
    f2.close();
#endif

    Import::TellicoImporter imp(str);
    imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
    Data::CollPtr c = imp.collection();
    if(!c) {
      continue;
    }
    // assume we always want the 3 extra fields defined in boardgamegeek2tellico.xsl
    if(!m_coll->hasField(QStringLiteral("bggid"))) {
      m_coll->addField(Data::FieldPtr(new Data::Field(*c->fieldByName(QStringLiteral("bggid")))));
      m_coll->addField(Data::FieldPtr(new Data::Field(*c->fieldByName(QStringLiteral("boardgamegeek-link")))));
      Data::FieldPtr f(new Data::Field(*c->fieldByName(QStringLiteral("artist"))));
      // also, let's assume that the artist field title should be illustrator instead of musician
      f->setTitle(i18nc("Comic Book Illustrator", "Artist"));
      m_coll->addField(f);
    }
    m_coll->addEntries(c->entries());
    setStatusMessage(imp.statusMessage());

    if(showProgress) {
      Q_EMIT signalProgress(this, 10 + 100*maxSize/idList.size());
      qApp->processEvents();
    }
  }

  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - BoardGameGeek"));
  config.writeEntry("User ID", m_user);
  config.writeEntry("Owned", m_ownedOnly);

  if(m_cancelled) {
    m_coll = Data::CollPtr();
  }
  return m_coll;
}

QWidget* BoardGameGeekImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("BoardGameGeek Options"), m_widget);
  QFormLayout* lay = new QFormLayout(gbox);

  m_userEdit = new QLineEdit(gbox);
  m_userEdit->setText(m_user);

  m_checkOwned = new QCheckBox(i18n("Import owned items only"), gbox);
  m_checkOwned->setChecked(m_ownedOnly);

  lay->addRow(i18n("User ID:"), m_userEdit);
  lay->addRow(m_checkOwned);

  l->addWidget(gbox);
  l->addStretch(1);

  return m_widget;
}

QString BoardGameGeekImporter::text(const QStringList& idList_) const {
//  myDebug() << idList_;
  QUrl u(QString::fromLatin1(BGG_THING_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("id"), idList_.join(QLatin1String(",")));
  q.addQueryItem(QStringLiteral("type"), QStringLiteral("boardgame,boardgameexpansion"));
  u.setQuery(q);
//  myDebug() << u;
  return FileHandler::readTextFile(u, true, true);
}

void BoardGameGeekImporter::slotCancel() {
  m_cancelled = true;
}
