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
#include "gui/cursorsaver.h"
#include "core/tellico_config.h"

#include <klocale.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <ksortablelist.h>
#include <kfiledialog.h>

#include <QFile>
#include <QLabel>
#include <QFileInfo>
#include <QTimer>
#include <QTextStream>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {
  static const int REPORT_MIN_WIDTH = 600;
  static const int REPORT_MIN_HEIGHT = 420;
}

using Tellico::ReportDialog;

// default button is going to be used as a print button, so it's separated
ReportDialog::ReportDialog(QWidget* parent_)
    : KDialog(parent_),
      m_exporter(0) {
  setModal(false);
  setCaption(i18n("Collection Report"));
  setButtons(Close);
  setDefaultButton(Close);

  QWidget* mainWidget = new QWidget(this);
  setMainWidget(mainWidget);
  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget);

  QBoxLayout* hlay = new QHBoxLayout();
  topLayout->addLayout(hlay);
  QLabel* l = new QLabel(i18n("&Report template:"), mainWidget);
  hlay->addWidget(l);

  QStringList files = KGlobal::dirs()->findAllResources("appdata", QLatin1String("report-templates/*.xsl"),
                                                        KStandardDirs::NoDuplicates);
  KSortableList<QString, QString> templates;
  foreach(const QString& file, files) {
    QFileInfo fi(file);
    QString lfile = fi.fileName();
    QString name = lfile.section(QLatin1Char('.'), 0, -2);
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    QString title = i18nc((name + QLatin1String(" XSL Template")).toUtf8(), name.toUtf8());
    templates.insert(title, lfile);
  }
  templates.sort();
  m_templateCombo = new GUI::ComboBox(mainWidget);
  for(KSortableList<QString, QString>::iterator it = templates.begin(); it != templates.end(); ++it) {
    m_templateCombo->addItem((*it).key(), (*it).value());
  }
  hlay->addWidget(m_templateCombo);
  l->setBuddy(m_templateCombo);

  KPushButton* pb1 = new KPushButton(KGuiItem(i18n("&Generate"), QLatin1String("application-x-executable")), mainWidget);
  hlay->addWidget(pb1);
  connect(pb1, SIGNAL(clicked()), SLOT(slotGenerate()));

  hlay->addStretch();

  KPushButton* pb2 = new KPushButton(KStandardGuiItem::saveAs(), mainWidget);
  hlay->addWidget(pb2);
  connect(pb2, SIGNAL(clicked()), SLOT(slotSaveAs()));

  KPushButton* pb3 = new KPushButton(KStandardGuiItem::print(), mainWidget);
  hlay->addWidget(pb3);
  connect(pb3, SIGNAL(clicked()), SLOT(slotPrint()));

  m_HTMLPart = new KHTMLPart(mainWidget);
  m_HTMLPart->setJScriptEnabled(false);
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

  setMinimumWidth(qMax(minimumWidth(), REPORT_MIN_WIDTH));
  setMinimumHeight(qMax(minimumHeight(), REPORT_MIN_HEIGHT));

  KConfigGroup config(KGlobal::config(), QLatin1String("Report Dialog Options"));
  restoreDialogSize(config);
}

ReportDialog::~ReportDialog() {
  delete m_exporter;
  m_exporter = 0;

  KConfigGroup config(KGlobal::config(), QLatin1String("Report Dialog Options"));
  saveDialogSize(config);
}

void ReportDialog::slotGenerate() {
  GUI::CursorSaver cs(Qt::WaitCursor);

  QString fileName = QLatin1String("report-templates/") + m_templateCombo->currentData().toString();
  QString xsltFile = KStandardDirs::locate("appdata", fileName);
  if(xsltFile.isEmpty()) {
    kWarning() << "ReportDialog::setXSLTFile() - can't locate " << m_templateCombo->currentData().toString();
    return;
  }
  // if it's the same XSL file, no need to reload the XSLTHandler, just refresh
  if(xsltFile == m_xsltFile) {
    slotRefresh();
    return;
  }

  m_xsltFile = xsltFile;

  delete m_exporter;
  m_exporter = new Export::HTMLExporter();
  m_exporter->setXSLTFile(m_xsltFile);
  m_exporter->setPrintHeaders(false); // the templates should take care of this themselves
  m_exporter->setPrintGrouped(true); // allow templates to take advantage of added DOM

  slotRefresh();
}

void ReportDialog::slotRefresh() {
  if(!m_exporter) {
    kWarning() << "ReportDialog::slotRefresh() - no exporter";
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
  KUrl u;
  u.setPath(m_xsltFile);
  m_HTMLPart->begin(u);
  m_HTMLPart->write(m_exporter->text());
#if 0
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
  QString filter = i18n("*.html|HTML Files (*.html)") + QLatin1Char('\n') + i18n("*|All Files");
  KUrl u = KFileDialog::getSaveUrl(KUrl(), filter, this);
  if(!u.isEmpty() && u.isValid()) {
    KConfigGroup config(KGlobal::config(), "ExportOptions");
    bool encode = config.readEntry("EncodeUTF8", true);
    long oldOpt = m_exporter->options();

    // turn utf8 off
    long options = oldOpt & ~Export::ExportUTF8;
    // now turn it on if true
    if(encode) {
      options |= Export::ExportUTF8;
    }

    KUrl oldURL = m_exporter->url();
    m_exporter->setOptions(options);
    m_exporter->setURL(u);

    m_exporter->exec();

    m_exporter->setURL(oldURL);
    m_exporter->setOptions(oldOpt);
  }
}

#include "reportdialog.moc"
