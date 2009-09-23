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

#include "xsltexporter.h"
#include "xslthandler.h"
#include "tellicoxmlexporter.h"
#include "../collection.h"
#include "../core/filehandler.h"

#include <klocale.h>
#include <kurlrequester.h>
#include <KUser>
#include <KConfigGroup>

#include <QLabel>
#include <QGroupBox>
#include <QDomDocument>
#include <QHBoxLayout>

using Tellico::Export::XSLTExporter;

XSLTExporter::XSLTExporter(Data::CollPtr coll_) : Export::Exporter(coll_),
    m_widget(0),
    m_URLRequester(0) {
}

QString XSLTExporter::formatString() const {
  return i18n("XSLT");
}

QString XSLTExporter::fileFilter() const {
  return i18n("*|All Files");
}


bool XSLTExporter::exec() {
  KUrl u = m_URLRequester->url();
  if(u.isEmpty() || !u.isValid()) {
    return false;
  }
  //  XSLTHandler handler(FileHandler::readXMLFile(url));
  XSLTHandler handler(u);
  handler.addStringParam("date", QDate::currentDate().toString(Qt::ISODate).toLatin1());
  handler.addStringParam("time", QTime::currentTime().toString(Qt::ISODate).toLatin1());
  handler.addStringParam("user", KUser(KUser::UseRealUserID).loginName().toLatin1());

  TellicoXMLExporter exporter(collection());
  exporter.setEntries(entries());
  exporter.setOptions(options());
  QDomDocument dom = exporter.exportXML();
  return FileHandler::writeTextURL(url(), handler.applyStylesheet(dom.toString()),
                                   options() & ExportUTF8, options() & Export::ExportForce);
}

QWidget* XSLTExporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("XSLT Options"), m_widget);
  QHBoxLayout* hlay = new QHBoxLayout(gbox);

  QLabel* label = new QLabel(i18n("XSLT file:"), gbox);
  m_URLRequester = new KUrlRequester(gbox);
  m_URLRequester->setWhatsThis(i18n("Choose the XSLT file used to transform the data."));
  label->setBuddy(m_URLRequester);

  hlay->addWidget(label);
  hlay->addWidget(m_URLRequester);

  l->addWidget(gbox);

  QString filter = i18n("*.xsl|XSL Files (*.xsl)") + QLatin1Char('\n') + i18n("*|All Files");
  m_URLRequester->setFilter(filter);
  m_URLRequester->setMode(KFile::File | KFile::ExistingOnly);
  if(!m_xsltFile.isEmpty()) {
    m_URLRequester->setUrl(m_xsltFile);
  }

  l->addStretch(1);
  return m_widget;
}

void XSLTExporter::readOptions(KSharedConfigPtr config_) {
  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_xsltFile = group.readEntry("Last File", KUrl());
}

void XSLTExporter::saveOptions(KSharedConfigPtr config_) {
  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_xsltFile = m_URLRequester->url();
  group.writeEntry("Last File", m_xsltFile);
}
