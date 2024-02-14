/***************************************************************************
    Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>
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

#include "marcimporter.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../core/filehandler.h"
#include "../utils/datafileregistry.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KComboBox>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QProcess>
#include <QStandardPaths>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QFormLayout>

using Tellico::Import::MarcImporter;

MarcImporter::MarcImporter(const QUrl& url_) : Tellico::Import::Importer(url_)
    , m_coll(nullptr)
    , m_cancelled(false)
    , m_isUnimarc(false)
    , m_MARCHandler(nullptr)
    , m_MODSHandler(nullptr)
    , m_widget(nullptr)
    , m_charSetCombo(nullptr)
    , m_marcFormatCombo(nullptr) {
}

MarcImporter::~MarcImporter() {
  delete m_MARCHandler;
  m_MARCHandler = nullptr;
  delete m_MODSHandler;
  m_MODSHandler = nullptr;
}

bool MarcImporter::canImport(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

Tellico::Data::CollPtr MarcImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  m_marcdump = QStandardPaths::findExecutable(QStringLiteral("yaz-marcdump"));
  if(m_marcdump.isEmpty()) {
    myDebug() << "Could not find yaz-marcdump executable";
    return Data::CollPtr();
  }

  if(urls().count() > 1) {
    myDebug() << "MarcImporter only importing first file";
  }

  const QUrl url = this->url();
  if(url.isEmpty() || !url.isLocalFile()) {
    myDebug() << "MarcImporter can only read local files";
    return Data::CollPtr();
  }

  if(m_widget) {
    m_marcCharSet = m_charSetCombo->currentText().toUpper();
    QStringList charSets;
    for(int i = 0; i < m_charSetCombo->count(); ++i) {
      charSets += m_charSetCombo->itemText(i).toUpper();
    }
    charSets += m_marcCharSet;
    charSets.removeDuplicates();
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - MARC"));
    config.writeEntry("CharacterSets", charSets);
    config.writeEntry("Last Character Set", m_marcCharSet);

    const QString format = m_marcFormatCombo->currentText();
    m_isUnimarc = format == QLatin1String("UNIMARC");
    config.writeEntry("Format", format);
  }
  if(m_marcCharSet.isEmpty()) {
    m_marcCharSet = QStringLiteral("UTF-8");
  }
  QStringList dumpArgs = { QStringLiteral("-f"),
                           m_marcCharSet,
                           QStringLiteral("-t"),
                           QStringLiteral("utf-8"),
                           QStringLiteral("-o"),
                           QStringLiteral("marcxml"),
                           url.toLocalFile()
  };
  QProcess dumpProc;
  dumpProc.start(m_marcdump, dumpArgs);
  if(!dumpProc.waitForStarted() || !dumpProc.waitForFinished()) {
    myDebug() << "yaz-marcdump failed to start or finish";
    myDebug() << "arguments:" << dumpArgs;
    return Data::CollPtr();
  }

  const QByteArray marcxml = dumpProc.readAllStandardOutput();
  if(!initMARCHandler() || !initMODSHandler()) {
    return Data::CollPtr();
  }
  if(m_cancelled) {
    return Data::CollPtr();
  }
  // reading a non-MARC file results in "<!-- Skipping bad byte"
  if(marcxml.isEmpty() || marcxml.startsWith("<!--")) {
    setStatusMessage(i18n("Tellico was unable to read any data."));
    return Data::CollPtr();
  }

  const QString mods = m_MARCHandler->applyStylesheet(QString::fromUtf8(marcxml));
  const QString output = m_MODSHandler->applyStylesheet(mods);
  Import::TellicoImporter imp(output);
  imp.setOptions(imp.options() ^ Import::ImportProgress); // no progress needed
  m_coll = imp.collection();
  return m_coll;
}

QWidget* MarcImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("MARC Options"), m_widget);
  QFormLayout* lay = new QFormLayout(gbox);

  m_charSetCombo = new KComboBox(gbox);
  m_charSetCombo->setEditable(true);
  lay->addRow(i18n("Character set:"), m_charSetCombo);

  m_marcFormatCombo = new KComboBox(gbox);
  m_marcFormatCombo->addItem(QStringLiteral("MARC21"));
  m_marcFormatCombo->addItem(QStringLiteral("UNIMARC"));
  lay->addRow(i18n("MARC Format:"), m_marcFormatCombo);

  l->addWidget(gbox);
  l->addStretch(1);

  // now read config options
  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ImportOptions - MARC"));
  QStringList charSets = config.readEntry("Character Sets", QStringList());
  if(charSets.isEmpty()) {
    charSets += QStringLiteral("UTF-8");
    charSets += QStringLiteral("ISO-8859-1");
  }
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
  auto textWidth = m_widget->fontMetrics().width(charSets.last());
#else
  auto textWidth = m_widget->fontMetrics().horizontalAdvance(charSets.last());
#endif
  m_charSetCombo->setMinimumWidth(1.5*textWidth);
  QString lastCharSet = config.readEntry("Last Character Set");
  if(!lastCharSet.isEmpty()) {
    if(!charSets.contains(lastCharSet)) charSets += lastCharSet;
    m_charSetCombo->setCurrentText(lastCharSet);
  }
  m_charSetCombo->addItems(charSets);
  const QString marcFormat = config.readEntry("Format");
  if(!marcFormat.isEmpty()) {
    m_marcFormatCombo->setCurrentText(marcFormat);
  }
  return m_widget;
}

void MarcImporter::setCharacterSet(const QString& charSet_) {
  m_marcCharSet = charSet_;
  if(m_widget) {
    m_charSetCombo->setEditText(charSet_);
  }
}

void MarcImporter::slotCancel() {
  m_cancelled = true;
}

bool MarcImporter::initMARCHandler() {
  if(m_MARCHandler) {
    return true;
  }

  QString xsltName = m_isUnimarc ? QStringLiteral("UNIMARC2MODS3.xsl")
                                 : QStringLiteral("MARC21slim2MODS3.xsl");
  QString xsltfile = DataFileRegistry::self()->locate(xsltName);
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate" << xsltName;
    return false;
  }

  m_MARCHandler = new XSLTHandler(QUrl::fromLocalFile(xsltfile));
  if(!m_MARCHandler->isValid()) {
    myWarning() << "error in MARC21slim2MODS3.xsl.";
    delete m_MARCHandler;
    m_MARCHandler = nullptr;
    return false;
  }
  return true;
}

bool MarcImporter::initMODSHandler() {
  if(m_MODSHandler) {
    return true;
  }

  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("mods2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate mods2tellico.xsl.";
    return false;
  }

  m_MODSHandler = new XSLTHandler(QUrl::fromLocalFile(xsltfile));
  if(!m_MODSHandler->isValid()) {
    myWarning() << "error in mods2tellico.xsl.";
    delete m_MODSHandler;
    m_MODSHandler = nullptr;
    return false;
  }
  return true;
}
