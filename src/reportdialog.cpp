/***************************************************************************
    Copyright (C) 2005-2020 Robby Stephenson <robby@periapsis.org>
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

#include <config.h>
#include "reportdialog.h"
#include "translators/htmlexporter.h"
#include "collection.h"
#include "document.h"
#include "controller.h"
#include "tellico_debug.h"
#include "gui/combobox.h"
#include "utils/cursorsaver.h"
#include "utils/datafileregistry.h"
#include "utils/tellico_utils.h"
#include "config/tellico_config.h"
#ifdef HAVE_QCHARTS
#include "charts/chartmanager.h"
#include "charts/chartreport.h"
#endif

#include <KLocalizedString>
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
#include <QStackedWidget>
#include <QScrollArea>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QTemporaryFile>

#ifdef USE_KHTML
#include <KHTMLPart>
#include <KHTMLView>
#else
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#endif

namespace {
  static const int REPORT_MIN_WIDTH = 600;
  static const int REPORT_MIN_HEIGHT = 420;
  static const char* dialogOptionsString = "Report Dialog Options";
  static const int INDEX_HTML = 0;
  static const int INDEX_CHART = 1;
  static const int ALL_ENTRIES = -1;
}

using Tellico::ReportDialog;

// default button is going to be used as a print button, so it's separated
ReportDialog::ReportDialog(QWidget* parent_)
    : QDialog(parent_), m_exporter(nullptr), m_tempFile(nullptr) {
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
  QMap<QString, QVariant> templates; // gets sorted by title
  foreach(const QString& file, files) {
    QFileInfo fi(file);
    const QString lfile = fi.fileName();
    // the Group Summary report template doesn't work with QWebView
#ifndef USE_KHTML
    if(lfile == QStringLiteral("Group_Summary.xsl")) {
      continue;
    }
#endif
    QString name = lfile.section(QLatin1Char('.'), 0, -2);
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    QString title = i18nc((name + QLatin1String(" XSL Template")).toUtf8().constData(), name.toUtf8().constData());
    templates.insert(title, lfile);
  }
#ifdef HAVE_QCHARTS
  // add the chart reports
  foreach(const auto& report, ChartManager::self()->allReports()) {
    templates.insert(report->title(), report->uuid());
  }
#endif
  // special case for concatenating all entry templates
  templates.insert(i18n("One Entry Per Page"), ALL_ENTRIES);

  m_templateCombo = new GUI::ComboBox(mainWidget);
  for(auto it = templates.constBegin(); it != templates.constEnd(); ++it) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    const auto metaType = static_cast<QMetaType::Type>(it.value().type());
#else
    const auto metaType = static_cast<QMetaType::Type>(it.value().typeId());
#endif
    m_templateCombo->addItem(QIcon::fromTheme(metaType == QMetaType::QUuid ? QStringLiteral("kchart")
                                                                           : QStringLiteral("text-rdf")),
                             it.key(), it.value());
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

  QColor color = palette().color(QPalette::Link);
  QString text = QString::fromLatin1("<html><style>p{font-family:sans-serif;font-weight:bold;width:50%;"
                                     "margin:20% auto auto auto;text-align:center;"
                                     "color:%1;}</style><body><p>").arg(color.name())
               + i18n("Select a report template and click <em>Generate</em>.") + QLatin1Char(' ')
               + i18n("Some reports may take several seconds to generate for large collections.")
               + QLatin1String("</p></body></html>");

  m_reportView = new QStackedWidget(mainWidget);
  topLayout->addWidget(m_reportView);

#ifdef USE_KHTML
  m_HTMLPart = new KHTMLPart(m_reportView);
  m_HTMLPart->setJScriptEnabled(true);
  m_HTMLPart->setJavaEnabled(false);
  m_HTMLPart->setMetaRefreshEnabled(false);
  m_HTMLPart->setPluginsEnabled(false);

  m_HTMLPart->begin();
  m_HTMLPart->write(text);
  m_HTMLPart->end();
  m_reportView->insertWidget(INDEX_HTML, m_HTMLPart->view());
#else
  m_webView = new QWebEngineView(m_reportView);
  connect(m_webView, &QWebEngineView::loadFinished, this, [](bool b) {
    if(!b) myDebug() << "ReportDialog - failed to load view";
  });
  QWebEngineSettings* settings = m_webView->page()->settings();
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

  m_webView->setHtml(text);
  m_reportView->insertWidget(INDEX_HTML, m_webView);
#endif

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
  delete m_tempFile;
  m_tempFile = nullptr;

  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String(dialogOptionsString));
  KWindowConfig::saveWindowSize(windowHandle(), config);
}

void ReportDialog::slotGenerate() {
  GUI::CursorSaver cs(Qt::WaitCursor);

  QVariant curData = m_templateCombo->currentData();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  const auto metaType = static_cast<QMetaType::Type>(curData.type());
#else
  const auto metaType = static_cast<QMetaType::Type>(curData.typeId());
#endif
  if(metaType == QMetaType::QUuid) {
    generateChart();
    m_reportView->setCurrentIndex(INDEX_CHART);
  } else if(curData == ALL_ENTRIES) {
    generateAllEntries();
    m_reportView->setCurrentIndex(INDEX_HTML);
  } else {
    generateHtml();
    m_reportView->setCurrentIndex(INDEX_HTML);
  }
}

void ReportDialog::generateChart() {
#ifdef HAVE_QCHARTS
  const QUuid uuid = m_templateCombo->currentData().toUuid();
  auto oldWidget = m_reportView->widget(INDEX_CHART);
  auto newWidget = ChartManager::self()->report(uuid)->createWidget();
  if(newWidget) {
    m_reportView->insertWidget(INDEX_CHART, newWidget);
  }
  if(oldWidget) {
    m_reportView->removeWidget(oldWidget);
    delete oldWidget;
  }
#endif
}

void ReportDialog::generateHtml() {
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

void ReportDialog::generateAllEntries() {
  auto coll = Data::Document::self()->collection();
  QString entryXSLTFile = Config::templateName(coll);
  QString xsltFile = DataFileRegistry::self()->locate(QLatin1String("entry-templates/") +
                                                      entryXSLTFile + QLatin1String(".xsl"));
  if(xsltFile.isEmpty()) {
    myWarning() << "can't locate " << entryXSLTFile << ".xsl";
    return;
  }
  m_xsltFile = xsltFile;

  delete m_exporter;
  m_exporter = new Export::HTMLExporter(coll);
  m_exporter->setXSLTFile(m_xsltFile);
  m_exporter->setPrintHeaders(false); // the templates should take care of this themselves
  m_exporter->setPrintGrouped(true); // allow templates to take advantage of added DOM
  m_exporter->setGroupBy(Controller::self()->expandedGroupBy());
  m_exporter->setSortTitles(Controller::self()->sortTitles());
  m_exporter->setColumns(Controller::self()->visibleColumns());
  long options = Export::ExportUTF8 | Export::ExportComplete | Export::ExportImages;
  if(Config::autoFormat()) {
    options |= Export::ExportFormatted;
  }
  m_exporter->setOptions(options);

  if(Controller::self()->visibleEntries().isEmpty()) {
    slotRefresh();
    return;
  }

  // do some surgery on the HTML since we've got <html> elements in every page
  static const QRegularExpression bodyRx(QLatin1String("<body[^>]*>"));
  m_exporter->setEntries(Controller::self()->visibleEntries());
  QString html = m_exporter->text();
  auto bodyMatch = bodyRx.match(html);
  Q_ASSERT(bodyMatch.hasMatch());
  const QString htmlStart = html.left(bodyMatch.capturedStart() + bodyMatch.capturedLength());
  const QString htmlEnd = html.mid(html.lastIndexOf(QLatin1String("</body")));
  const QString htmlBetween = QStringLiteral("<p style=\"page-break-after: always;\">&nbsp;</p>"
                                             "<p style=\"page-break-before: always;\">&nbsp;</p>");

  // estimate how much space in the string to reserve
  const auto estimatedSize = Controller::self()->visibleEntries().size() * html.size();
  html = htmlStart;
  html.reserve(1.1*estimatedSize);
  foreach(Data::EntryPtr entry, Controller::self()->visibleEntries()) {
    m_exporter->setEntries(Data::EntryList() << entry);
    QString fullText = m_exporter->text();
    // extract the body portion
    auto bodyMatch = bodyRx.match(fullText);
    if(bodyMatch.hasMatch()) {
      const auto bodyEnd = fullText.lastIndexOf(QLatin1String("</body"));
      const auto bodyLength = bodyEnd - bodyMatch.capturedEnd();
      html += fullText.mid(bodyMatch.capturedEnd(), bodyLength) + htmlBetween;
    }
  }
  html += htmlEnd;
  m_exporter->setCustomHtml(html);
  showText(html, QUrl::fromLocalFile(m_xsltFile));
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
  showText(m_exporter->text(), QUrl::fromLocalFile(m_xsltFile));
}

void ReportDialog::showText(const QString& text_, const QUrl& url_) {
#ifdef USE_KHTML
  m_HTMLPart->begin(url_);
  m_HTMLPart->write(text_);
  m_HTMLPart->end();
#else
  // limit is 2 MB after percent encoding, etc., so give some padding
  if(text_.size() > 1200000) {
    delete m_tempFile;
    m_tempFile = new QTemporaryFile(QDir::tempPath() + QLatin1String("/tellicoreport_XXXXXX") + QLatin1String(".html"));
    m_tempFile->open();
    QTextStream ts(m_tempFile);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ts.setCodec("UTF-8");
#else
    ts.setEncoding(QStringConverter::Utf8);
#endif
    ts << text_;
    m_webView->load(QUrl::fromLocalFile(m_tempFile->fileName()));
  } else {
    m_webView->setHtml(text_, url_);
  }
#endif
#if 0
  myDebug() << "Remove debug from reportdialog.cpp";
  QFile f(QLatin1String("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << text_;
  }
  f.close();
#endif
}

// actually the print button
void ReportDialog::slotPrint() {
  if(m_reportView->currentIndex() == INDEX_CHART) {
    QPrinter printer;
    printer.setResolution(600);
    QPointer<QPrintDialog> dialog = new QPrintDialog(&printer, this);
    if(dialog->exec() == QDialog::Accepted) {
      QWidget* widget = m_reportView->currentWidget();
      // there might be a widget inside a scroll area
      if(QScrollArea* scrollArea = qobject_cast<QScrollArea*>(widget)) {
        widget = scrollArea->widget();
      }
      QPainter painter;
      painter.begin(&printer);
      auto const paintRect = printer.pageLayout().paintRectPixels(printer.resolution());
      const double xscale = paintRect.width() / double(widget->width());
      const double yscale = paintRect.height() / double(widget->height());
      const double scale = 0.95*qMin(xscale, yscale);
      auto const paperRect = printer.pageLayout().fullRectPixels(printer.resolution());
      painter.translate(paperRect.center());
      painter.scale(scale, scale);
      painter.translate(-widget->width()/2, -widget->height()/2);
      widget->render(&painter);
    }
  } else {
#ifdef USE_KHTML
    m_HTMLPart->view()->print();
#else
    QPrinter printer;
    printer.setResolution(300);
    QPointer<QPrintDialog> dialog = new QPrintDialog(&printer, this);
    if(dialog->exec() == QDialog::Accepted) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
      GUI::CursorSaver cs(Qt::WaitCursor);
      QEventLoop loop;
      m_webView->page()->print(&printer, [&](bool) { loop.quit(); });
      loop.exec();
#else
      m_webView->print(&printer);
#endif
    }
#endif
  }
}

void ReportDialog::slotSaveAs() {
  if(m_reportView->currentIndex() == INDEX_CHART) {
    QString filter = i18n("PNG Files") + QLatin1String(" (*.png)")
                  + QLatin1String(";;")
                  + i18n("All Files") + QLatin1String(" (*)");
    QUrl u = QFileDialog::getSaveFileUrl(this, QString(), QUrl(), filter);
    if(!u.isEmpty() && u.isValid()) {
      QWidget* widget = m_reportView->currentWidget();
      // there might be a widget inside a scroll area
      if(QScrollArea* scrollArea = qobject_cast<QScrollArea*>(widget)) {
        widget = scrollArea->widget();
      }
      QPixmap pixmap(widget->size());
      widget->render(&pixmap);
      pixmap.save(u.toLocalFile());
    }
  } else if(m_exporter) {
    QString filter = i18n("HTML Files") + QLatin1String(" (*.html)")
                   + QLatin1String(";;")
                   + i18n("All Files") + QLatin1String(" (*)");
    QUrl u = QFileDialog::getSaveFileUrl(this, QString(), QUrl(), filter);
    if(!u.isEmpty() && u.isValid()) {
      KConfigGroup config(KSharedConfig::openConfig(), QLatin1String("ExportOptions"));
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
}

void ReportDialog::slotUpdateSize() {
  KConfigGroup config(KSharedConfig::openConfig(), QLatin1String(dialogOptionsString));
  KWindowConfig::restoreWindowSize(windowHandle(), config);
}
