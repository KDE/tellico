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

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#include <QPaintEngine>

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

  if(!printPrepare()) return;

  m_view->setHtml(m_html, Data::Document::self()->URL());
  m_printer->setDocName(Data::Document::self()->URL().fileName());

  // don't have busy cursor when showing the print dialog
  cs.restore();

  QPrintDialog dialog(m_printer.get(), m_view.get());
  if(dialog.exec() != QDialog::Accepted) {
    return;
  }

  printDocument(dialog.printer());
}

void PrintHandler::printPreview() {
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

  // do not use m_printer, this one is specific to preview widget
  QPrinter previewPrinter;
  QPrintPreviewDialog preview(&previewPrinter, m_view.get());
  QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                   this, &PrintHandler::printDocument);
  {
    // this is a workaround for ensuring the initial dialog open shows the preview already
    // with Qt 5.15.2, it didn't seem to get previewed initially
    auto list = preview.findChildren<QPrintPreviewWidget*>();
    if(!list.isEmpty()) list.first()->updatePreview();
  }
  preview.exec();
  m_inPrintPreview = false;
}

void PrintHandler::printDocument(QPrinter* printer_) {
  if(printer_->paintEngine()->type() == QPaintEngine::Picture) {
    myLog() << "Printing preview...";
  } else if(printer_->outputFormat() == QPrinter::PdfFormat) {
    myLog() << "Printing PDF to" << printer_->outputFileName();
  } else {
    myLog() << "Printing to" << printer_->printerName();
  }

  if(printer_->outputFormat() == QPrinter::PdfFormat) {
    m_view->printToPdf(printer_->outputFileName(), printer_->pageLayout());
  } else {
    m_view->print(printer_);
  }
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

  if(!m_view) {
    m_view.reset(new QWebEngineView);
    connect(m_view.get(), &QWebEngineView::printFinished, this, &PrintHandler::printFinished);
    connect(m_view.get(), &QWebEngineView::pdfPrintingFinished, this, &PrintHandler::pdfPrintFinished);

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
  return true;
}
