/***************************************************************************
    Copyright (C) 2005-2009 Robby Stephenson <robby@periapsis.org>
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

#include "reportdialog.h"
#include "translators/htmlexporter.h"
#include "images/imagefactory.h"
#include "tellico_kernel.h"
#include "collection.h"
#include "document.h"
#include "entry.h"
#include "controller.h"
#include "tellico_debug.h"
#include "gui/combobox.h"
#include "utils/cursorsaver.h"
#include "utils/datafileregistry.h"
#include "utils/tellico_utils.h"
#include "config/tellico_config.h"

#include <KLocalizedString>
#include <KHTMLPart>
#include <KHTMLView>
#include <KStandardGuiItem>
#include <KWindowConfig>
#include <KConfigGroup>

#include <QFile>
#include <QLabel>
#include <QFileInfo>
#include <QTimer>
#include <QTextStream>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>

namespace {
  static const int REPORT_MIN_WIDTH = 600;
  static const int REPORT_MIN_HEIGHT = 420;
  static const char* dialogOptionsString = "Report Dialog Options";
}

using Tellico::ReportDialog;

// default button is going to be used as a print button, so it's separated
ReportDialog::ReportDialog(QWidget* parent_)
    : QDialog(parent_), m_exporter(nullptr) {
  setModal(false);
  setWindowTitle(i18n("Collection Report"));

  QWidget* mainWidget = new QWidget(this);
  QVBoxLayout* mainLayout = new QVBoxLayout();
  setLayout(mainLayout);
  mainLayout->addWidget(mainWidget);

  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);

  QBoxLayout* hlay = new QHBoxLayout();
  topLayout->addLayout(hlay);
  QLabel* l = new QLabel(i18n("&Report template:"), mainWidget);
  hlay->addWidget(l);

  QStringList files = Tellico::locateAllFiles(QStringLiteral("tellico/report-templates/*.xsl"));
  QMap<QString, QString> templates; // gets sorted by title
  foreach(const QString& file, files) {
    QFileInfo fi(file);
    const QString lfile = fi.fileName();
    QString name = lfile.section(QLatin1Char('.'), 0, -2);
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    QString title = i18nc((name + QLatin1String(" XSL Template")).toUtf8().constData(), name.toUtf8().constData());
    templates.insert(title, lfile);
  }

  m_templateCombo = new GUI::ComboBox(mainWidget);
  for(QMap<QString, QString>::ConstIterator it = templates.constBegin(); it != templates.constEnd(); ++it) {
    m_templateCombo->addItem(QIcon::fromTheme(QStringLiteral("text-rdf")), it.key(), it.value());
  }
  hlay->addWidget(m_templateCombo);
  l->setBuddy(m_templateCombo);

  QPushButton* pb1 = new QPushButton(mainWidget);
  KGuiItem::assign(pb1, KGuiItem(i18n("&Generate"), QStringLiteral("application-x-executable")));
  hlay->addWidget(pb1);
  connect(pb1, &QAbstractButton::clicked, this, &ReportDialog::slotGenerate);

  hlay->addStretch();

  QPushButton* pb2 = new QPushButton(mainWidget);
  KGuiItem::assign(pb2, KStandardGuiItem::saveAs());
  hlay->addWidget(pb2);
  connect(pb2, &QAbstractButton::clicked, this, &ReportDialog::slotSaveAs);

  QPushButton* pb3 = new QPushButton(mainWidget);
  KGuiItem::assign(pb3, KStandardGuiItem::print());
  hlay->addWidget(pb3);
  connect(pb3, &QAbstractButton::clicked, this, &ReportDialog::slotPrint);

  m_HTMLPart = new KHTMLPart(mainWidget);
  m_HTMLPart->setJScriptEnabled(true);
  m_HTMLPart->setJavaEnabled(false);
  m_HTMLPart->setMetaRefreshEnabled(false);
  m_HTMLPart->setPluginsEnabled(false);
  topLayout->addWidget(m_HTMLPart->view());

  QColor color = palette().color(QPalette::Link);
  QString text = QString::fromLatin1("<html><style>p{font-weight:bold;width:50%;"
                                     "margin:20% auto auto auto;text-align:center;"
                                     "color:%1;}</style><body><p>").arg(color.name())
               + i18n("Select a report template and click <em>Generate</em>.") + QLatin1Char(' ')
               + i18n("Some reports may take several seconds to generate for large collections.")
               + QLatin1String("</p></body></html>");
  m_HTMLPart->begin();
  m_HTMLPart->write(text);
  m_HTMLPart->end();

  QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
  mainLayout->addWidget(buttonBox);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  buttonBox->button(QDialogButtonBox::Close)->setDefault(true);

  setMinimumWidth(qMax(minimumWidth(), REPORT_MIN_WIDTH));
  setMinimumHeight(qMax(minimumHeight(), REPORT_MIN_HEIGHT));

  QTimer::singleShot(0, this, &ReportDialog::slotUpdateSize);
}

ReportDialog::~ReportDialog() {
  delete m_exporter;
  m_exporter = nullptr;

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String(dialogOptionsString));
  KWindowConfig::saveWindowSize(windowHandle(), config);
}

void ReportDialog::slotGenerate() {
  GUI::CursorSaver cs(Qt::WaitCursor);

  QString fileName = QLatin1String("report-templates/") + m_templateCombo->currentData().toString();
  QString xsltFile = DataFileRegistry::self()->locate(fileName);
  if(xsltFile.isEmpty()) {
    myWarning() << "can't locate " << m_templateCombo->currentData().toString();
    return;
  }
  // if it's the same XSL file, no need to reload the XSLTHandler, just refresh
  if(xsltFile == m_xsltFile) {
    slotRefresh();
    return;
  }

  m_xsltFile = xsltFile;

  delete m_exporter;
  m_exporter = new Export::HTMLExporter(Data::Document::self()->collection());
  m_exporter->setXSLTFile(m_xsltFile);
  m_exporter->setPrintHeaders(false); // the templates should take care of this themselves
  m_exporter->setPrintGrouped(true); // allow templates to take advantage of added DOM

  slotRefresh();
}

void ReportDialog::slotRefresh() {
  if(!m_exporter) {
    myWarning() << "no exporter";
    return;
  }

  m_exporter->setGroupBy(Controller::self()->expandedGroupBy());
  m_exporter->setSortTitles(Controller::self()->sortTitles());
  m_exporter->setColumns(Controller::self()->visibleColumns());
  // only print visible entries
  m_exporter->setEntries(Controller::self()->visibleEntries());

  long options = Export::ExportUTF8 | Export::ExportComplete | Export::ExportImages;
  if(Config::autoFormat()) {
    options |= Export::ExportFormatted;
  }
  m_exporter->setOptions(options);

  // by setting the xslt file as the URL, any images referenced in the xslt "theme" can be found
  // by simply using a relative path in the xslt file
  QUrl u = QUrl::fromLocalFile(m_xsltFile);
  m_HTMLPart->begin(u);
  m_HTMLPart->write(m_exporter->text());
#if 0
  myDebug() << "Remove debug from reportdialog.cpp";
  QFile f(QLatin1String("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << m_exporter->text();
  }
  f.close();
#endif
  m_HTMLPart->end();
  // is this needed?
//  view()->layout();
}

// actually the print button
void ReportDialog::slotPrint() {
  m_HTMLPart->view()->print();
}

void ReportDialog::slotSaveAs() {
  QString filter = i18n("HTML Files") + QLatin1String(" (*.html)")
                 + QLatin1String(";;")
                 + i18n("All Files") + QLatin1String(" (*)");
  QUrl u = QFileDialog::getSaveFileUrl(this, QString(), QUrl(), filter);
  if(!u.isEmpty() && u.isValid()) {
    KConfigGroup config(KSharedConfig::openConfig(), "ExportOptions");
    bool encode = config.readEntry("EncodeUTF8", true);
    long oldOpt = m_exporter->options();

    // turn utf8 off
    long options = oldOpt & ~Export::ExportUTF8;
    // now turn it on if true
    if(encode) {
      options |= Export::ExportUTF8;
    }

    QUrl oldURL = m_exporter->url();
    m_exporter->setOptions(options);
    m_exporter->setURL(u);

    m_exporter->exec();

    m_exporter->setURL(oldURL);
    m_exporter->setOptions(oldOpt);
  }
}

void ReportDialog::slotUpdateSize() {
  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String(dialogOptionsString));
  KWindowConfig::restoreWindowSize(windowHandle(), config);
}
