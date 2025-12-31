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

#include "exportdialog.h"
#include "collection.h"
#include "controller.h"
#include "tellico_debug.h"

#include "translators/exporter.h"
#include "translators/tellicoxmlexporter.h"
#include "translators/tellicozipexporter.h"
#include "translators/htmlexporter.h"
#include "translators/csvexporter.h"
#include "translators/bibtexexporter.h"
#include "translators/bibtexmlexporter.h"
#include "translators/xsltexporter.h"
#include "translators/alexandriaexporter.h"
#include "translators/onixexporter.h"
#include "translators/gcstarexporter.h"
#include "utils/string_utils.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QScopedPointer>

using namespace Tellico;
using Tellico::ExportDialog;

ExportDialog::ExportDialog(Tellico::Export::Format format_, Tellico::Data::CollPtr coll_, const QUrl& baseUrl_, QWidget* parent_)
  : QDialog(parent_),
    m_format(format_),
    m_coll(coll_),
    m_exporter(exporter(format_, coll_, baseUrl_)) {
  setModal(true);
  setWindowTitle(i18n("Export Options"));

  QWidget* widget = new QWidget(this);
  QVBoxLayout* topLayout = new QVBoxLayout(widget);
  setLayout(topLayout);
  topLayout->addWidget(widget);

  QGroupBox* group1 = new QGroupBox(i18n("Formatting"), widget);
  topLayout->addWidget(group1, 0);
  QVBoxLayout* vlay = new QVBoxLayout(group1);

  m_formatFields = new QCheckBox(i18n("Format all fields"), group1);
  m_formatFields->setChecked(false);
  m_formatFields->setWhatsThis(i18n("If checked, the values of the fields will be "
                                    "automatically formatted according to their format type."));
  vlay->addWidget(m_formatFields);

  m_exportSelected = new QCheckBox(i18n("Export selected entries only"), group1);
  m_exportSelected->setChecked(false);
  m_exportSelected->setWhatsThis(i18n("If checked, only the currently selected entries will "
                                      "be exported."));
  vlay->addWidget(m_exportSelected);

  m_exportFields = new QCheckBox(i18n("Export visible fields only"), group1);
  m_exportFields->setChecked(false);
  m_exportFields->setWhatsThis(i18n("If checked, only the fields currently visible in the view will "
                                    "be exported."));
  vlay->addWidget(m_exportFields);

  QGroupBox* group2 = new QGroupBox(i18n("Encoding"), widget);
  topLayout->addWidget(group2, 0);

  QVBoxLayout* vlay2 = new QVBoxLayout(group2);

  m_encodeUTF8 = new QRadioButton(i18n("Encode in Unicode (UTF-8)"), group2);
  m_encodeUTF8->setChecked(true);
  m_encodeUTF8->setWhatsThis(i18n("Encode the exported file in Unicode (UTF-8)."));
  vlay2->addWidget(m_encodeUTF8);

  const auto localeName = Tellico::localeEncodingName();
  const QString localStr = i18n("Encode in user locale (%1)",
                                QLatin1String(localeName));
  m_encodeLocale = new QRadioButton(localStr, group2);
  m_encodeLocale->setWhatsThis(i18n("Encode the exported file in the local encoding."));
  vlay2->addWidget(m_encodeLocale);

  if(localeName == QByteArray("UTF-8")) {
    m_encodeUTF8->setChecked(true);
    m_encodeLocale->setEnabled(false);
  }

  QButtonGroup* bg = new QButtonGroup(widget);
  bg->addButton(m_encodeUTF8);
  bg->addButton(m_encodeLocale);

  QWidget* w = m_exporter->widget(widget);
  if(w) {
    w->layout()->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(w, 0);
  }

  topLayout->addStretch();

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
  connect(okButton, &QAbstractButton::clicked, this, &ExportDialog::slotSaveOptions);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  topLayout->addWidget(buttonBox);

  readOptions();
  if(format_ == Export::Alexandria) {
    // no encoding options enabled
    group2->setEnabled(false);
  }
}

ExportDialog::~ExportDialog() {
  delete m_exporter;
  m_exporter = nullptr;
}

QString ExportDialog::fileFilter() {
  return m_exporter ? m_exporter->fileFilter() : QString();
}

void ExportDialog::readOptions() {
  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ExportOptions"));
  bool format = config.readEntry("FormatFields", false);
  m_formatFields->setChecked(format);
  bool selected = config.readEntry("ExportSelectedOnly", false);
  m_exportSelected->setChecked(selected);
  bool encode = config.readEntry("EncodeUTF8", true);
  if(encode && m_encodeUTF8->isEnabled()) {
    m_encodeUTF8->setChecked(true);
  } else {
    m_encodeLocale->setChecked(true);
  }
}

void ExportDialog::slotSaveOptions() {
  KSharedConfigPtr config = KSharedConfig::openConfig();
  // each exporter sets its own group
  m_exporter->saveOptions(config);

  KConfigGroup configGroup(config, QStringLiteral("ExportOptions"));
  configGroup.writeEntry("FormatFields", m_formatFields->isChecked());
  configGroup.writeEntry("ExportSelectedOnly", m_exportSelected->isChecked());
  configGroup.writeEntry("EncodeUTF8", m_encodeUTF8->isChecked());
}

// static
Tellico::Export::Exporter* ExportDialog::exporter(Tellico::Export::Format format_, Data::CollPtr coll_, const QUrl& baseUrl_) {
  Export::Exporter* exporter = nullptr;

  switch(format_) {
    case Export::TellicoXML:
      exporter = new Export::TellicoXMLExporter(coll_, baseUrl_);
      break;

    case Export::TellicoZip:
      exporter = new Export::TellicoZipExporter(coll_, baseUrl_);
      break;

    case Export::HTML:
      {
        Export::HTMLExporter* htmlExp = new Export::HTMLExporter(coll_, baseUrl_);
        htmlExp->setGroupBy(Controller::self()->expandedGroupBy());
        htmlExp->setSortTitles(Controller::self()->sortTitles());
        htmlExp->setColumns(Controller::self()->visibleColumns());
        exporter = htmlExp;
      }
      break;

    case Export::CSV:
      exporter = new Export::CSVExporter(coll_);
      break;

    case Export::Bibtex:
      exporter = new Export::BibtexExporter(coll_);
      break;

    case Export::Bibtexml:
      exporter = new Export::BibtexmlExporter(coll_);
      break;

    case Export::XSLT:
      exporter = new Export::XSLTExporter(coll_, baseUrl_);
      break;

    case Export::Alexandria:
      exporter = new Export::AlexandriaExporter(coll_);
      break;

    case Export::ONIX:
      exporter = new Export::ONIXExporter(coll_, baseUrl_);
      break;

    case Export::GCstar:
      exporter = new Export::GCstarExporter(coll_, baseUrl_);
      break;

    default:
      myDebug() << "not implemented!";
      break;
  }
  if(exporter) {
    exporter->readOptions(KSharedConfig::openConfig());
  }
  return exporter;
}

bool ExportDialog::exportURL(const QUrl& url_/*=QUrl()*/) const {
  if(!m_exporter) {
    return false;
  }

  // exporter might need to know final URL, say for writing images or something
  m_exporter->setURL(url_);
  if(m_exportSelected->isChecked()) {
    m_exporter->setEntries(Controller::self()->selectedEntries());
  } else {
    m_exporter->setEntries(m_coll->entries());
  }
  if(m_exportFields->isChecked()) {
    Data::FieldList fields;
    foreach(const QString& title, Controller::self()->visibleColumns()) {
      Data::FieldPtr field = m_coll->fieldByTitle(title);
      if(field) {
        fields << field;
      }
    }
    m_exporter->setFields(fields);
  } else {
    m_exporter->setFields(m_coll->fields());
  }
  long opt = Export::ExportImages | Export::ExportComplete | Export::ExportProgress; // for now, always export images
  if(m_formatFields->isChecked()) {
    opt |= Export::ExportFormatted;
  }
  if(m_encodeUTF8->isChecked()) {
    opt |= Export::ExportUTF8;
  }
  // since we already asked about overwriting the file, force the save
  opt |= Export::ExportForce;

  m_exporter->setOptions(opt);

  return m_exporter->exec();
}

// static
// alexandria is exported to known directory
// all others are files
Tellico::Export::Target ExportDialog::exportTarget(Tellico::Export::Format format_) {
  switch(format_) {
    case Export::Alexandria:
      return Export::None;
    default:
      return Export::File;
  }
}

// static
bool ExportDialog::exportCollection(Data::CollPtr coll_, Data::EntryList entries_, Export::Format format_,
                                    const QUrl& baseUrl_, const QUrl& targetUrl_) {
  QScopedPointer<Export::Exporter> exp(exporter(format_, coll_, baseUrl_));
  exp->setURL(targetUrl_);
  exp->setEntries(entries_);

  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("ExportOptions"));
  long options = 0;
  if(config.readEntry("FormatFields", false)) {
    options |= Export::ExportFormatted;
  }
  if(config.readEntry("EncodeUTF8", true)) {
    options |= Export::ExportUTF8;
  }
  exp->setOptions(options | Export::ExportForce);

  return exp->exec();
}
