/***************************************************************************
    Copyright (C) 2023 Robby Stephenson <robby@periapsis.org>
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

#include "opdsfetcher.h"
#include "../fieldformat.h"
#include "../collection.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../core/filehandler.h"
#include "../utils/datafileregistry.h"
#include "../utils/guiproxy.h"
#include "../utils/isbnvalidator.h"
#include "../translators/tellico_xml.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KJobUiDelegate>
#include <KJobWidgets>
#include <KAcceleratorManager>
#include <KUrlRequester>

#include <QLabel>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QPushButton>

using namespace Tellico;
using Tellico::Fetch::OPDSFetcher;

OPDSFetcher::Reader::Reader(const QUrl& catalog_) : catalog(catalog_), isAcquisition(false) {
}

// read the catalog file and return the search description url
bool OPDSFetcher::Reader::parse() {
  opdsText = FileHandler::readDataFile(catalog);
  QXmlStreamReader xml(opdsText);
  int depth = 0;
  while(xml.readNext() != QXmlStreamReader::Invalid) {
    switch(xml.tokenType()) {
      case QXmlStreamReader::StartElement:
        ++depth;
        if(depth == 2 && xml.namespaceUri() == Tellico::XML::nsAtom) {
          if(xml.name() == QLatin1StringView("link")) {
            auto attributes = xml.attributes();
            if(attributes.value(QLatin1StringView("rel")) == QLatin1StringView("search")) {
              // found the search url
              const auto href = QUrl(attributes.value(QLatin1StringView("href")).toString());
              searchUrl = catalog.resolved(href);
              myLog() << "Search url is" << searchUrl.toDisplayString();
            } else if(attributes.value(QLatin1StringView("rel")) == QLatin1StringView("self")) {
              // for now, consider the feed an acquisition feed if the self link is labeled as an acquisition feed
              isAcquisition = attributes.value(QLatin1StringView("type")).contains(QLatin1StringView("kind=acquisition"));
              myLog() << "Catalog kind is 'acquisition'";
            }
          }
        }
        break;
      case QXmlStreamReader::EndElement:
        --depth;
        break;
      default:
        break;
    }
  }
  // valid catalog either has a search url or is an acquisition feed
  const bool ret = !searchUrl.isEmpty() || isAcquisition;
  if(!ret) {
    if(searchUrl.isEmpty()) errorString = QStringLiteral("Search Url is empty");
    else errorString = QStringLiteral("Catalog is not an acquisition catalog");
  }
  return ret;
}

bool OPDSFetcher::Reader::readSearchTemplate() {
  myLog() << "Reading catalog:" << catalog.toDisplayString();
  if(searchUrl.isEmpty() && !isAcquisition && !parse()) return false;
  if(searchUrl.isEmpty()) return false;
  //    myDebug() << "Reading search description:" << searchDescriptionUrl;
  // read the search description and find the search template
  const QByteArray descText = FileHandler::readDataFile(searchUrl);
  QXmlStreamReader xml(descText);
  int depth = 0;
  QString text, shortName, longName;
  while(xml.readNext() != QXmlStreamReader::Invalid) {
    switch(xml.tokenType()) {
      case QXmlStreamReader::StartElement:
        ++depth;
        if(depth == 2 && xml.name() == QLatin1StringView("Url") &&
                         xml.namespaceUri() == XML::nsOpenSearch) {
          auto attributes = xml.attributes();
          if(attributes.value(QLatin1StringView("type")) == QLatin1StringView("application/atom+xml")) {
            searchTemplate = attributes.value(QLatin1StringView("template")).toString();
          }
        }
        break;
      case QXmlStreamReader::EndElement:
        if(depth == 2) {
          if(xml.name() == QLatin1StringView("LongName")) {
            longName = text.simplified();
          } else if(xml.name() == QLatin1StringView("ShortName")) {
            shortName = text.simplified();
          } else if(xml.name() == QLatin1StringView("Image")) {
            icon = text.simplified();
          } else if(xml.name() == QLatin1StringView("Attribution")) {
            attribution = text.simplified();
          }
        }
        --depth;
        text.clear();
        break;
      case QXmlStreamReader::Characters:
        text += xml.text();
        break;
      default:
        break;
    }
  }
  name = longName.isEmpty() ? shortName : longName;
  myLog() << "Search template is" << searchTemplate;
  return !searchTemplate.isEmpty();
}

OPDSFetcher::OPDSFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(nullptr), m_started(false) {
}

OPDSFetcher::~OPDSFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = nullptr;
}

QString OPDSFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString OPDSFetcher::attribution() const {
  return m_attribution;
}

QString OPDSFetcher::icon() const {
  return favIcon(QUrl(m_icon));
}

bool OPDSFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Keyword || k == ISBN;
}

bool OPDSFetcher::canFetch(int type) const {
  return type == Data::Collection::Book || type == Data::Collection::Bibtex;
}

void OPDSFetcher::readConfigHook(const KConfigGroup& config_) {
  m_catalog = config_.readEntry("Catalog");
  m_searchTemplate = config_.readEntry("SearchTemplate");
  m_icon = config_.readEntry("Icon");
  m_attribution = config_.readEntry("Attribution");
}

void OPDSFetcher::saveConfigHook(KConfigGroup& config_) {
  if(!m_searchTemplate.isEmpty()) {
    config_.writeEntry("SearchTemplate", m_searchTemplate);
  }
  if(!m_icon.isEmpty()) {
    config_.writeEntry("Icon", m_icon);
  }
  if(!m_attribution.isEmpty()) {
    config_.writeEntry("Attribution", m_attribution);
  }
}

void OPDSFetcher::search() {
  m_started = true;
  if(m_catalog.isEmpty()) {
    myDebug() << source() << "- url is not set";
    stop();
    return;
  }

  Reader reader(QUrl::fromUserInput(m_catalog));
  if(m_searchTemplate.isEmpty()) {
    if(!reader.parse()) {
      myDebug() << source() << "- failed to parse:" << reader.errorString;
      message(i18n("Tellico is unable to read the search description in the OPDS catalog."), MessageHandler::Error);
      stop();
      return;
    }
    if(reader.isAcquisition) {
      parseData(reader.opdsText, true /* manualSearch */);
      return;
    }
    if(!reader.readSearchTemplate()) {
      myDebug() << source() << "- no search template";
      message(i18n("Tellico is unable to read the search description in the OPDS catalog."), MessageHandler::Error);
      stop();
      return;
    }
  }
  // continue with search
  if(m_searchTemplate.isEmpty()) {
    m_searchTemplate = reader.searchTemplate;
    m_icon = reader.icon;
    m_attribution = reader.attribution;
  }

  QString searchTerm;
  switch(request().key()) {
    case Title:
    case Keyword:
      searchTerm = request().value();
      break;

    case ISBN:
      {
        QString isbn = request().value().section(QLatin1Char(';'), 0);
        isbn.remove(QLatin1Char('-'));
        searchTerm = isbn;
      }
      break;

    default:
      myWarning() << "key not recognized: " << request().key();
      stop();
      break;
  }

  QString searchUrl = m_searchTemplate;
  searchUrl.replace(QLatin1StringView("{searchTerms}"), searchTerm);
  QUrl u(searchUrl);
  myLog() << "Searching" << u.toDisplayString();

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &OPDSFetcher::slotComplete);
}

void OPDSFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }

  m_started = false;
  Q_EMIT signalDone(this);
}

void OPDSFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;
  parseData(data);
}

void OPDSFetcher::parseData(const QByteArray& data_, bool manualSearch_) {
#if 0
  myWarning() << "Remove debug from opdsfetcher.cpp";
  QFile f(QString::fromLatin1("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data_;
  }
  f.close();
#endif

  if(!m_xsltHandler) {
    initXSLTHandler();
    if(!m_xsltHandler) { // probably an error somewhere in the stylesheet loading
      stop();
      return;
    }
  }

  // assume result is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data_.constData(), data_.size()));
  Import::TellicoImporter imp(str);
  imp.setBaseUrl(QUrl(m_searchTemplate.isEmpty() ? m_catalog : m_searchTemplate));
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << source() << " - no collection pointer";
    stop();
    return;
  }

  foreach(Data::EntryPtr entry, coll->entries()) {
    // if manual search, do poor man's comparison
    if(manualSearch_ && !matchesEntry(entry)) continue;
    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }
  stop();
}

bool OPDSFetcher::matchesEntry(Data::EntryPtr entry_) const {
  switch(request().key()) {
    case Title:
      return entry_->title().contains(request().value(), Qt::CaseInsensitive);
    case ISBN:
      {
        ISBNComparison comp;
        return comp(entry_->field(QStringLiteral("isbn")), request().value());
      }
    case Keyword:
      return entry_->title().contains(request().value(), Qt::CaseInsensitive) ||
        entry_->field(QStringLiteral("author")).contains(request().value(), Qt::CaseInsensitive) ||
        entry_->field(QStringLiteral("keyword")).contains(request().value(), Qt::CaseInsensitive) ||
        entry_->field(QStringLiteral("publisher")).contains(request().value(), Qt::CaseInsensitive) ||
        entry_->field(QStringLiteral("genre")).contains(request().value(), Qt::CaseInsensitive) ||
        entry_->field(QStringLiteral("pub_year")).contains(request().value(), Qt::CaseInsensitive) ||
        entry_->field(QStringLiteral("plot")).contains(request().value(), Qt::CaseInsensitive);
    default:
      break;
  }
  return false;
}

Tellico::Data::EntryPtr OPDSFetcher::fetchEntryHook(uint uid_) {
  auto entry = m_entries[uid_];
  if(!entry) return entry;
  // check whether the summary shows content from Calibre server and try to compensate
  QString plot = entry->field(QStringLiteral("plot"));
  static const QByteArray xhtml("<div xmlns=\"http://www.w3.org/1999/xhtml\">");
  if(plot.startsWith(QLatin1StringView(xhtml))) {
    plot = plot.mid(xhtml.length());
    myLog() << "Detected Calibre-style plot format";
    myLog() << "Removing XHTML div";
    static const QByteArray divEnd("</div>");
    if(plot.endsWith(QLatin1String(divEnd))) {
      plot.chop(divEnd.length());
    }
    static const QRegularExpression ratingRx(QStringLiteral("RATING: (★+)<br/>"));
    auto ratingMatch = ratingRx.match(plot);
    if(ratingMatch.hasMatch()) {
      // length of star match is the rating number
      entry->setField(QStringLiteral("rating"), QString::number(ratingMatch.captured(1).length()));
      plot.remove(ratingMatch.captured());
    }
    static const QRegularExpression tagsRX(QStringLiteral("TAGS: (.+?)<br/>"));
    auto tagsMatch = tagsRX.match(plot);
    if(tagsMatch.hasMatch()) {
      entry->setField(QStringLiteral("genre"),
                      FieldFormat::splitValue(tagsMatch.captured(1), FieldFormat::CommaRegExpSplit)
                                  .join(FieldFormat::delimiterString()));
      plot.remove(tagsMatch.captured());
    }
    static const QRegularExpression seriesRx(QStringLiteral("SERIES: (.+?) \\[(\\d+)\\]<br/>"));
    auto seriesMatch = seriesRx.match(plot);
    if(seriesMatch.hasMatch()) {
      entry->setField(QStringLiteral("series"), seriesMatch.captured(1));
      entry->setField(QStringLiteral("series_num"), seriesMatch.captured(2));
      plot.remove(seriesMatch.captured());
    }
    plot.remove(QLatin1StringView("SUMMARY:<br/>"));
    plot = plot.simplified();
    if(plot.startsWith(QLatin1StringView("<p class=\"description\">"))) {
      plot = plot.mid(23);
      if(plot.endsWith(QLatin1StringView("</p>"))) {
        plot.chop(4);
      }
    }
    entry->setField(QStringLiteral("plot"), plot);
  }
  return entry;
}

void OPDSFetcher::initXSLTHandler() {
  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("atom2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate atom2tellico.xsl.";
    return;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in atom2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = nullptr;
  }
}

Tellico::Fetch::FetchRequest OPDSFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }
  return FetchRequest();
}

QString OPDSFetcher::defaultName() {
  return i18n("OPDS Catalog");
}

QString OPDSFetcher::defaultIcon() {
  return QStringLiteral("folder-book");
}

// static
Tellico::StringHash OPDSFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("url")] = i18n("URL");
  return hash;
}

Tellico::Fetch::ConfigWidget* OPDSFetcher::configWidget(QWidget* parent_) const {
  return new ConfigWidget(parent_, this);
}

OPDSFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const OPDSFetcher* fetcher_ /*=0*/)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;
  QLabel* label = new QLabel(i18n("Catalog: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_catalogEdit = new KUrlRequester(optionsWidget());
  connect(m_catalogEdit, &KUrlRequester::textEdited, this, &ConfigWidget::slotSetModified);
  l->addWidget(m_catalogEdit, row, 1);
  QString w = i18n("Enter the link to the OPDS server.");
  label->setWhatsThis(w);
  m_catalogEdit->setWhatsThis(w);
  label->setBuddy(m_catalogEdit);

  auto verifyButton = new QPushButton(i18n("&Verify Catalog"), optionsWidget());
  connect(verifyButton, &QPushButton::clicked,
          this, &ConfigWidget::verifyCatalog);
  l->addWidget(verifyButton, ++row, 0);
  m_statusLabel = new QLabel(optionsWidget());
  l->addWidget(m_statusLabel, row, 1);

  l->setRowStretch(++row, 1);

  // now add additional fields widget
  addFieldsWidget(OPDSFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  if(fetcher_) {
    m_catalogEdit->setText(fetcher_->m_catalog);
    m_searchTemplate = fetcher_->m_searchTemplate;
    m_icon = fetcher_->m_icon;
    m_attribution = fetcher_->m_attribution;
  }
  KAcceleratorManager::manage(optionsWidget());
}

void OPDSFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  QString s = m_catalogEdit->text().trimmed();
  if(!s.isEmpty()) {
    config_.writeEntry("Catalog", s);
    config_.writeEntry("SearchTemplate", m_searchTemplate);
    config_.writeEntry("Icon", m_icon);
    config_.writeEntry("Attribution", m_attribution);
  }
}

QString OPDSFetcher::ConfigWidget::preferredName() const {
  auto u = m_catalogEdit->url();
  return m_name.isEmpty() ? (u.isEmpty() ? OPDSFetcher::defaultName() : u.host()) : m_name;
}

void OPDSFetcher::ConfigWidget::verifyCatalog() {
  OPDSFetcher::Reader reader(m_catalogEdit->url());
  const int imgSize = 0.8*m_statusLabel->height();
  if(reader.readSearchTemplate()) {
    m_statusLabel->setPixmap(QIcon::fromTheme(QStringLiteral("emblem-checked")).pixmap(imgSize, imgSize));
    slotSetModified();
    if(!reader.name.isEmpty()) {
      Q_EMIT signalName(reader.name);
    }
    m_name = reader.name;
    m_searchTemplate = reader.searchTemplate;
    m_icon = reader.icon;
    m_attribution = reader.attribution;
  } else if(reader.isAcquisition) {
    m_statusLabel->setPixmap(QIcon::fromTheme(QStringLiteral("emblem-added")).pixmap(imgSize, imgSize));
    m_searchTemplate.clear();
  } else {
    m_statusLabel->setPixmap(QIcon::fromTheme(QStringLiteral("emblem-error")).pixmap(imgSize, imgSize));
    m_searchTemplate.clear();
    m_icon.clear();
    m_attribution.clear();
  }
}
