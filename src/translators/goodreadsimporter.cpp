/***************************************************************************
    Copyright (C) 2011 Robby Stephenson <robby@periapsis.org>
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

#include "goodreadsimporter.h"
#include "xslthandler.h"
#include "tellicoimporter.h"
#include "filehandler.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLineEdit>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDomDocument>
#include <QRegExp>

namespace {
  static const char* GOODREADS_LIST_URL = "http://www.goodreads.com/review/list.xml";
  static const char* GOODREADS_USER_URL = "http://www.goodreads.com/user/show.xml";
  static const char* GOODREADS_API_KEY = "dpgbQvOWk0n4cwL32jQRA";
}

using Tellico::Import::GoodreadsImporter;

GoodreadsImporter::GoodreadsImporter() : Import::Importer(), m_widget(0) {
  QString xsltFile = DataFileRegistry::self()->locate(QLatin1String("goodreads2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    m_xsltURL = QUrl::fromLocalFile(xsltFile);
  } else {
    myWarning() << "unable to find goodreads2tellico.xsl!";
  }

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("ImportOptions - Goodreads"));
  m_user = config.readEntry("User ID");
  m_key = config.readEntry("Developer Key");
  if(m_key.isEmpty()) {
    m_key = QLatin1String(GOODREADS_API_KEY);
  }
}

bool GoodreadsImporter::canImport(int type) const {
  return type == Data::Collection::Book;
}

Tellico::Data::CollPtr GoodreadsImporter::collection() {
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
  // if the user is not all digits, assume it's a user name and
  // convert it to a user id
  if(!QRegExp(QLatin1String("\\d+")).exactMatch(m_user)) {
    m_user = idFromName(m_user);
  }
  if(m_user.isEmpty()) {
    setStatusMessage(i18n("A valid user ID must be entered."));
    return Data::CollPtr();
  }

  XSLTHandler handler(m_xsltURL);
  if(!handler.isValid()) {
    setStatusMessage(i18n("Tellico encountered an error in XSLT processing."));
    return Data::CollPtr();
  }

//  myDebug() << text();
  QString str = handler.applyStylesheet(text());
//  myDebug() << str;

  Import::TellicoImporter imp(str);
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("ImportOptions - Goodreads"));
  config.writeEntry("User ID", m_user);
  config.writeEntry("Developer Key", m_key);

  return m_coll;
}

QWidget* GoodreadsImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("Goodreads Options"), m_widget);
  QFormLayout* lay = new QFormLayout(gbox);

  m_userEdit = new KLineEdit(gbox);
  m_userEdit->setText(m_user);

  lay->addRow(i18n("User ID"), m_userEdit);

  l->addWidget(gbox);
  l->addStretch(1);

  return m_widget;
}

QString GoodreadsImporter::text() const {
  QUrl u(QString::fromLatin1(GOODREADS_LIST_URL));
  u.addQueryItem(QLatin1String("v"), QLatin1String("2"));
  u.addQueryItem(QLatin1String("id"), m_user);
  u.addQueryItem(QLatin1String("key"), m_key);
//  myDebug() << u;
  return FileHandler::readTextFile(u, false, true);
}

QString GoodreadsImporter::idFromName(const QString& name_) const {
  QUrl u(QString::fromLatin1(GOODREADS_USER_URL));
  u.addQueryItem(QLatin1String("username"), name_);
  u.addQueryItem(QLatin1String("key"), m_key);
//  myDebug() << u;

  QDomDocument dom = FileHandler::readXMLDocument(u, false /* process namespace */);
  return dom.documentElement().namedItem(QLatin1String("user"))
                              .namedItem(QLatin1String("id"))
                              .toElement()
                              .text()
                              .trimmed();
}

