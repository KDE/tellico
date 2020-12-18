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
#else
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QPrinter>
#include <QPrintDialog>
#include <QEventLoop>
#endif

using Tellico::PrintHandler;

PrintHandler::PrintHandler(QObject* parent_) : QObject(parent_)
    , m_inPrintPreview(false) {
}

PrintHandler::~PrintHandler() {
}

void PrintHandler::setEntries(const Data::EntryList& entries_) {
  m_entries = entries_;
}

void PrintHandler::setColumns(const QStringList& columns_) {
  m_columns = columns_;
}

void PrintHandler::print() {
  GUI::CursorSaver cs(Qt::WaitCursor);

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

  const QString html = exporter.text();
  if(html.isEmpty()) {
    myDebug() << "PrintHandler - empty html output";
    return;
  }

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
  w.write(html);
  w.end();
  w.view()->print();
#else
  QScopedPointer<QWebEngineView> view(new QWebEngineView);
  QWebEngineSettings* settings = view->page()->settings();
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
  settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

  view->setHtml(html, Data::Document::self()->URL());

  // don't have busy cursor when showing the print dialog
  cs.restore();

  QPrinter printer;
  printer.setResolution(300);
  QPointer<QPrintDialog> dialog = new QPrintDialog(&printer, view.data());
  if(dialog->exec() != QDialog::Accepted) {
    return;
  }
  printDocument(&printer, view->page());
#endif
}

void PrintHandler::printPreview() {
}

void PrintHandler::printDocument(QPrinter* printer_, QWebEnginePage* page_) {
#ifdef USE_KHTML
  Q_UNUSED(printer_);
  Q_UNUSED(page_);
#else
  GUI::CursorSaver cs(Qt::WaitCursor);
  QEventLoop loop;
  page_->print(printer_, [&](bool) { loop.quit(); });
  loop.exec();
#endif
}
