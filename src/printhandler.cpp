/***************************************************************************
    Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>
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
#include "printhandler.h"
#include "document.h"
#include "controller.h"
#include "translators/htmlexporter.h"
#include "utils/cursorsaver.h"
#include "config/tellico_config.h"
#include "../tellico_debug.h"

#ifdef USE_KHTML
#include <KHTMLPart>
#include <KHTMLView>
#include <KAboutData>
class QWebEngineView {};
#else
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#endif

using Tellico::PrintHandler;

PrintHandler::PrintHandler(QObject* parent_) : QObject(parent_)
    , m_inPrintPreview(false) {
}

PrintHandler::~PrintHandler() {
}

void PrintHandler::setEntries(const Tellico::Data::EntryList& entries_) {
  m_entries = entries_;
  m_html.clear();
}

void PrintHandler::setColumns(const QStringList& columns_) {
  m_columns = columns_;
  m_html.clear();
}

void PrintHandler::print() {
  GUI::CursorSaver cs(Qt::WaitCursor);

#ifdef USE_KHTML
  KHTMLPart w;

  // KHTMLPart printing was broken in KDE until KHTML 5.16
  // see https://git.reviewboard.kde.org/r/125681/
  const QString version =  w.componentData().version();
  const uint major = version.section(QLatin1Char('.'), 0, 0).toUInt();
  const uint minor = version.section(QLatin1Char('.'), 1, 1).toUInt();
  if(major == 5 && minor < 16) {
    myWarning() << "Printing is broken for KDE Frameworks < 5.16. Please upgrade";
    return;
  }

  w.setJScriptEnabled(false);
  w.setJavaEnabled(false);
  w.setMetaRefreshEnabled(false);
  w.setPluginsEnabled(false);
  w.begin(Data::Document::self()->URL());
  w.write(m_html);
  w.end();
  w.view()->print();
#else

  if(!printPrepare()) return;

  m_view->setHtml(m_html, Data::Document::self()->URL());
  m_printer->setDocName(Data::Document::self()->URL().fileName());

  // don't have busy cursor when showing the print dialog
  cs.restore();

  QPrintDialog dialog(m_printer.get(), m_view.get());
  if(dialog.exec() != QDialog::Accepted) {
    return;
  }

  if(dialog.printer()->outputFormat() == QPrinter::PdfFormat) {
    myLog() << "Printing PDF to" << dialog.printer()->outputFileName();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_view->page()->printToPdf(dialog.printer()->outputFileName(), dialog.printer()->pageLayout());
#else
    m_view->printToPdf(dialog.printer()->outputFileName(), dialog.printer()->pageLayout());
#endif
    m_waitForResult.exec(QEventLoop::ExcludeUserInputEvents);
  } else {
    printDocument(m_printer.get());
  }
#endif
}

void PrintHandler::printPreview() {
// print preview only works with WebEngine
#ifndef USE_KHTML
  if(m_inPrintPreview) {
    return;
  }
  myLog() << "Previewing print job";
  GUI::CursorSaver cs(Qt::WaitCursor);

  if(!printPrepare()) return;

  m_inPrintPreview = true;
  m_view->setHtml(m_html, Data::Document::self()->URL());

  // don't have busy cursor when showing the dialog
  cs.restore();

  QPrintPreviewDialog preview(m_printer.get(), m_view.get());
  QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                   this, &PrintHandler::printDocument);
  {
    // this is a workaround for ensuring the initial dialog open shows the preview already
    // with Qt 5.15.2, it didn't seem to get previewed initially
    auto list = preview.findChildren<QPrintPreviewWidget*>();
    auto w = list.first();
//    QTimer::singleShot(0, w, &QPrintPreviewWidget::updatePreview);
    if(w) w->updatePreview();
  }
  preview.show();
  preview.exec();
  m_inPrintPreview = false;
#endif
}

void PrintHandler::printDocument(QPrinter* printer_) {
  myLog() << "Printing to" << printer_->printerName();
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_view->page()->print(printer_, [=](bool b) { printFinished(b); });
#else
  m_view->print(printer_);
#endif
  // User input in the print preview dialog while we're waiting on a print task
  // can mess up the internal state and cause a crash.
  m_waitForResult.exec(QEventLoop::ExcludeUserInputEvents);
}

void PrintHandler::printFinished(bool success_) {
  if(success_) {
    myLog() << "Printing completed";
  } else {
    myLog() << "Printing failed";
  }
  m_waitForResult.quit();
}

void PrintHandler::pdfPrintFinished(const QString&, bool success_) {
  if(success_) {
    myLog() << "PDF printing completed";
  } else {
    myLog() << "PDF printing failed";
  }
  m_waitForResult.quit();
}

QString PrintHandler::generateHtml() const {
  Export::HTMLExporter exporter(Data::Document::self()->collection());
  // only print visible entries
  exporter.setEntries(m_entries);
  exporter.setXSLTFile(QStringLiteral("tellico-printing.xsl"));
  exporter.setPrintHeaders(Config::printFieldHeaders());
  exporter.setPrintGrouped(Config::printGrouped());
  exporter.setGroupBy(Controller::self()->expandedGroupBy());
  if(!Config::printGrouped()) { // the sort titles are only used if the entries are not grouped
    exporter.setSortTitles(Controller::self()->sortTitles());
  }
  exporter.setColumns(m_columns);
  exporter.setMaxImageSize(Config::maxImageWidth(), Config::maxImageHeight());
  if(Config::printFormatted()) {
    exporter.setOptions(Export::ExportUTF8 | Export::ExportFormatted);
  } else {
    exporter.setOptions(Export::ExportUTF8);
  }

  return exporter.text();
}

bool PrintHandler::printPrepare() {
  if(m_html.isEmpty()) {
    m_html = generateHtml();
    if(m_html.isEmpty()) {
      myDebug() << "PrintHandler - empty html output";
      return false;
    }
  }

#ifndef USE_KHTML
  if(!m_view) {
    m_view.reset(new QWebEngineView);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    // no printFInished signal in Qt5, so there's a callback specified directly
    connect(m_view->page(), &QWebEnginePage::pdfPrintingFinished, this, &PrintHandler::pdfPrintFinished);
#else
    connect(m_view.get(), &QWebEngineView::printFinished, this, &PrintHandler::printFinished);
    connect(m_view.get(), &QWebEngineView::pdfPrintingFinished, this, &PrintHandler::pdfPrintFinished);
#endif

    auto settings = m_view->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
  }

  if(!m_printer) {
    auto info = QPrinterInfo::defaultPrinter();
    if(info.isNull()) {
      m_printer.reset(new QPrinter(QPrinter::HighResolution));
    } else {
      m_printer.reset(new QPrinter(info, QPrinter::HighResolution));
    }
    m_printer->setCreator(QStringLiteral("Tellico/%1").arg(QStringLiteral(TELLICO_VERSION)));
  }
#endif
  return true;
}
