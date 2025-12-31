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
#include "../core/filehandler.h"
#include "../utils/datafileregistry.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QLineEdit>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDomDocument>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QFile>

namespace {
  static const char* GOODREADS_LIST_URL = "https://www.goodreads.com/review/list.xml";
  static const char* GOODREADS_USER_URL = "https://www.goodreads.com/user/show.xml";
  static const char* GOODREADS_API_KEY = "23626735f5a498f2f2c003300549c0b73f5caa9e87e9faca81eac394eaa5a0d66b3a6d0f563182f29efa";
}

using Tellico::Import::GoodreadsImporter;

GoodreadsImporter::GoodreadsImporter() : Import::Importer(), m_widget(nullptr), m_userEdit(nullptr) {
  QString xsltFile = DataFileRegistry::self()->locate(QStringLiteral("goodreads2tellico.xsl"));
  if(!xsltFile.isEmpty()) {
    m_xsltURL = QUrl::fromLocalFile(xsltFile);
  } else {
    myWarning() << "unable to find goodreads2tellico.xsl!";
  }
}

void GoodreadsImporter::setConfig(KSharedConfig::Ptr config_) {
  m_config = config_;
}

bool GoodreadsImporter::canImport(int type) const {
  return type == Data::Collection::Book;
}

Tellico::Data::CollPtr GoodreadsImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  if(!m_config) {
    m_config = KSharedConfig::openConfig();
  }

  KConfigGroup cg(m_config, QStringLiteral("ImportOptions - Goodreads"));
  m_user = cg.readEntry("User ID");
  m_key = cg.readEntry("Developer Key");
  if(m_key.isEmpty()) {
    m_key = Tellico::reverseObfuscate(GOODREADS_API_KEY);
  }

  if(m_xsltURL.isEmpty() || !m_xsltURL.isValid()) {
    setStatusMessage(i18n("A valid XSLT file is needed to import the file."));
    return Data::CollPtr();
  }

  if(m_widget) {
    m_user = m_userEdit->text().trimmed();
  } else if(m_user.isEmpty()) {
    myWarning() << "no widget and no user!";
    return Data::CollPtr();
  }

  // if the user is not all digits, assume it's a user name and
  // convert it to a user id
  static const QRegularExpression digitsRx(QStringLiteral("^\\d+$"));
  if(!digitsRx.match(m_user).hasMatch()) {
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

  const QString text = this->text();
#if 0
  myWarning() << "Remove output debug from goodreadsimporter.cpp";
  QFile f(QLatin1String("/tmp/goodreads.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << text;
  }
  f.close();
#endif

  QString str = handler.applyStylesheet(text);
//  myDebug() << str;

  Import::TellicoImporter imp(str);
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());

  cg.writeEntry("User ID", m_user);
  cg.writeEntry("Developer Key", m_key);

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

  m_userEdit = new QLineEdit(gbox);
  m_userEdit->setText(m_user);

  lay->addRow(i18n("User ID:"), m_userEdit);

  l->addWidget(gbox);
  l->addStretch(1);

  return m_widget;
}

QString GoodreadsImporter::text() const {
  QUrl u(QString::fromLatin1(GOODREADS_LIST_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("v"), QStringLiteral("2"));
  q.addQueryItem(QStringLiteral("id"), m_user);
  q.addQueryItem(QStringLiteral("key"), m_key);
  u.setQuery(q);
//  myDebug() << u;
  return FileHandler::readTextFile(u, true /* quiet */, true);
}

QString GoodreadsImporter::idFromName(const QString& name_) const {
  QUrl u(QString::fromLatin1(GOODREADS_USER_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("username"), name_);
  q.addQueryItem(QStringLiteral("key"), m_key);
  u.setQuery(q);
//  myDebug() << u;

  QDomDocument dom = FileHandler::readXMLDocument(u, false /* process namespace */, true /* quiet */);
  return dom.documentElement().namedItem(QStringLiteral("user"))
                              .namedItem(QStringLiteral("id"))
                              .toElement()
                              .text()
                              .trimmed();
}
