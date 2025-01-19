/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "xsltimporter.h"
#include "xslthandler.h"
#include "tellicoimporter.h"
#include "../core/filehandler.h"
#include "../collection.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KUrlRequester>
#include <kio_version.h>

#include <QLabel>
#include <QGroupBox>
#include <QTextStream>
#include <QHBoxLayout>
#include <QScopedPointer>

using Tellico::Import::XSLTImporter;

namespace {

static bool isUTF8(const QUrl& url_) {
  // read first line to check encoding
  const QScopedPointer<Tellico::FileHandler::FileRef> ref(Tellico::FileHandler::fileRef(url_));
  if(!ref->isValid()) {
    return false;
  }

  ref->open();
  QTextStream stream(ref->file());
  QString line = stream.readLine().toLower();
  return line.indexOf(QLatin1String("utf-8")) > 0;
}

}

// always use utf8 for xslt
XSLTImporter::XSLTImporter(const QUrl& url_) : Tellico::Import::TextImporter(url_, isUTF8(url_)),
    m_widget(nullptr),
    m_URLRequester(nullptr) {
}

XSLTImporter::XSLTImporter(const QString& text_) : Tellico::Import::TextImporter(text_),
    m_widget(nullptr),
    m_URLRequester(nullptr) {
}

Tellico::Data::CollPtr XSLTImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  if(m_xsltURL.isEmpty()) {
    // if there's also no widget, then something went wrong
    if(!m_widget) {
      setStatusMessage(i18n("A valid XSLT file is needed to import the file."));
      return Data::CollPtr();
    }
    m_xsltURL = m_URLRequester->url();
  }
  if(m_xsltURL.isEmpty() || !m_xsltURL.isValid()) {
    setStatusMessage(i18n("A valid XSLT file is needed to import the file."));
    return Data::CollPtr();
  }

  XSLTHandler handler(m_xsltURL);
  if(!handler.isValid()) {
    myDebug() << "invalid xslt handler";
    setStatusMessage(i18n("Tellico encountered an error in XSLT processing."));
    return Data::CollPtr();
  }
  beginXSLTHandler(&handler);
//  myDebug() << text();
  const QString str = handler.applyStylesheet(text());
//  myDebug() << str;

  Import::TellicoImporter imp(str);
  imp.setBaseUrl(url()); // ignore case of multiple urls for now
  imp.setOptions(options());
  m_coll = imp.collection();
  setStatusMessage(imp.statusMessage());
  return m_coll;
}

QWidget* XSLTImporter::widget(QWidget* parent_) {
  // if the url has already been set, then there's no widget
  if(!m_xsltURL.isEmpty()) {
    return nullptr;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("XSLT Options"), m_widget);
  QHBoxLayout* hlay = new QHBoxLayout(gbox);

  QLabel* label = new QLabel(i18n("XSLT file:"), gbox);
  m_URLRequester = new KUrlRequester(gbox);
  m_URLRequester->setWhatsThis(i18n("Choose the XSLT file used to transform the data."));
  label->setBuddy(m_URLRequester);

  const QStringList filters = {i18n("XSL Files") + QLatin1String(" (*.xsl)"),
                               i18n("All Files") + QLatin1String(" (*)")};
  m_URLRequester->setNameFilters(filters);

  hlay->addWidget(label);
  hlay->addWidget(m_URLRequester);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void XSLTImporter::slotCancel() {
  myDebug() << "unimplemented";
}
