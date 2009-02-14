/***************************************************************************
    copyright            : (C) 2005-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "reportdialog.h"
#include "translators/htmlexporter.h"
#include "imagefactory.h"
#include "tellico_kernel.h"
#include "collection.h"
#include "document.h"
#include "entry.h"
#include "controller.h"
#include "tellico_utils.h"
#include "tellico_debug.h"
#include "gui/combobox.h"
#include "core/tellico_config.h"

#include <klocale.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <ksortablevaluelist.h>
#include <kfiledialog.h>

#include <qlayout.h>
#include <qfile.h>
#include <qlabel.h>
#include <qfileinfo.h>
#include <qtimer.h>

namespace {
  static const int REPORT_MIN_WIDTH = 600;
  static const int REPORT_MIN_HEIGHT = 420;
}

using Tellico::ReportDialog;

// default button is going to be used as a print button, so it's separated
ReportDialog::ReportDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(parent_, name_, false, i18n("Collection Report"), Close, Close),
      m_exporter(0) {
  QWidget* mainWidget = new QWidget(this, "ReportDialog mainWidget");
  setMainWidget(mainWidget);
  QVBoxLayout* topLayout = new QVBoxLayout(mainWidget, 0, KDialog::spacingHint());

  QHBoxLayout* hlay = new QHBoxLayout(topLayout);
  QLabel* l = new QLabel(i18n("&Report template:"), mainWidget);
  hlay->addWidget(l);

// KStandardDirs::findAllResources(const char *type, const QString &filter, bool recursive, bool uniq)
  QStringList files = KGlobal::dirs()->findAllResources("appdata", QString::fromLatin1("report-templates/*.xsl"),
                                                        false, true);
  KSortableValueList<QString, QString> templates;
  for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
    QFileInfo fi(*it);
    QString file = fi.fileName();
    QString name = file.section('.', 0, -2);
    name.replace('_', ' ');
    QString title = i18n((name + QString::fromLatin1(" XSL Template")).utf8(), name.utf8());
    templates.insert(title, file);
  }
  templates.sort();
  m_templateCombo = new GUI::ComboBox(mainWidget);
  for(KSortableValueList<QString, QString>::iterator it = templates.begin(); it != templates.end(); ++it) {
    m_templateCombo->insertItem((*it).index(), (*it).value());
  }
  hlay->addWidget(m_templateCombo);
  l->setBuddy(m_templateCombo);

  KPushButton* pb1 = new KPushButton(SmallIconSet(QString::fromLatin1("exec")), i18n("&Generate"), mainWidget);
  hlay->addWidget(pb1);
  connect(pb1, SIGNAL(clicked()), SLOT(slotGenerate()));

  hlay->addStretch();

  KPushButton* pb2 = new KPushButton(KStdGuiItem::saveAs(), mainWidget);
  hlay->addWidget(pb2);
  connect(pb2, SIGNAL(clicked()), SLOT(slotSaveAs()));

  KPushButton* pb3 = new KPushButton(KStdGuiItem::print(), mainWidget);
  hlay->addWidget(pb3);
  connect(pb3, SIGNAL(clicked()), SLOT(slotPrint()));

  m_HTMLPart = new KHTMLPart(mainWidget);
  m_HTMLPart->setJScriptEnabled(false);
  m_HTMLPart->setJavaEnabled(false);
  m_HTMLPart->setMetaRefreshEnabled(false);
  m_HTMLPart->setPluginsEnabled(false);
  topLayout->addWidget(m_HTMLPart->view());

  QString text = QString::fromLatin1("<html><style>p{font-weight:bold;width:50%;"
                                     "margin:20% auto auto auto;text-align:center;"
                                     "background:white;color:%1;}</style><body><p>").arg(contrastColor.name())
               + i18n("Select a report template and click <em>Generate</em>.") + ' '
               + i18n("Some reports may take several seconds to generate for large collections.");
               + QString::fromLatin1("</p></body></html>");
  m_HTMLPart->begin();
  m_HTMLPart->write(text);
  m_HTMLPart->end();

  setMinimumWidth(QMAX(minimumWidth(), REPORT_MIN_WIDTH));
  setMinimumHeight(QMAX(minimumHeight(), REPORT_MIN_HEIGHT));
  resize(configDialogSize(QString::fromLatin1("Report Dialog Options")));
}

ReportDialog::~ReportDialog() {
  delete m_exporter;
  m_exporter = 0;

  saveDialogSize(QString::fromLatin1("Report Dialog Options"));
}

void ReportDialog::slotGenerate() {
  GUI::CursorSaver cs(Qt::waitCursor);

  QString fileName = QString::fromLatin1("report-templates/") + m_templateCombo->currentData().toString();
  QString xsltFile = locate("appdata", fileName);
  if(xsltFile.isEmpty()) {
    kdWarning() << "ReportDialog::setXSLTFile() - can't locate " << m_templateCombo->currentData().toString() << endl;
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
    kdWarning() << "ReportDialog::slotRefresh() - no exporter" << endl;
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
  KURL u;
  u.setPath(m_xsltFile);
  m_HTMLPart->begin(u);
  m_HTMLPart->write(m_exporter->text());
#if 0
  QFile f(QString::fromLatin1("/tmp/test.html"));
  if(f.open(IO_WriteOnly)) {
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
  QString filter = i18n("*.html|HTML Files (*.html)") + QChar('\n') + i18n("*|All Files");
  KURL u = KFileDialog::getSaveURL(QString::null, filter, this);
  if(!u.isEmpty() && u.isValid()) {
    KConfigGroup config(KGlobal::config(), "ExportOptions");
    bool encode = config.readBoolEntry("EncodeUTF8", true);
    long oldOpt = m_exporter->options();

    // turn utf8 off
    long options = oldOpt & ~Export::ExportUTF8;
    // now turn it on if true
    if(encode) {
      options |= Export::ExportUTF8;
    }

    KURL oldURL = m_exporter->url();
    m_exporter->setOptions(options);
    m_exporter->setURL(u);

    m_exporter->exec();

    m_exporter->setURL(oldURL);
    m_exporter->setOptions(oldOpt);
  }
}

#include "reportdialog.moc"
