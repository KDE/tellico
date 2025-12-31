/***************************************************************************
    Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>
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

#include "arxivfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../translators/tellico_xml.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../utils/datafileregistry.h"
#include "../collection.h"
#include "../entry.h"
#include "../core/netaccess.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KConfigGroup>
#include <KJobWidgets>

#include <QDomDocument>
#include <QLabel>
#include <QTextStream>
#include <QPixmap>
#include <QVBoxLayout>
#include <QFile>
#include <QUrlQuery>

namespace {
  static const int ARXIV_RETURNS_PER_REQUEST = 20;
  static const char* ARXIV_BASE_URL = "http://export.arxiv.org/api/query";
}

using namespace Tellico;
using namespace Tellico::Fetch;
using Tellico::Fetch::ArxivFetcher;

ArxivFetcher::ArxivFetcher(QObject* parent_)
    : Fetcher(parent_), m_xsltHandler(nullptr), m_start(0), m_total(-1), m_job(nullptr), m_started(false) {
}

ArxivFetcher::~ArxivFetcher() {
  delete m_xsltHandler;
  m_xsltHandler = nullptr;
}

QString ArxivFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString ArxivFetcher::attribution() const {
  // https://info.arxiv.org/help/api/index.html
  return i18nc("Acknowledgment from https://info.arxiv.org/help/api/index.html",
               "Thank you to arXiv for use of its open access interoperability.");
}

bool ArxivFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == Person || k == Keyword || k == ArxivID;
}

bool ArxivFetcher::canFetch(int type) const {
  return type == Data::Collection::Bibtex;
}

void ArxivFetcher::readConfigHook(const KConfigGroup&) {
}

void ArxivFetcher::search() {
  m_started = true;
  m_start = 0;
  m_total = -1;
  doSearch();
}

void ArxivFetcher::continueSearch() {
  m_started = true;
  doSearch();
}

void ArxivFetcher::doSearch() {
  QUrl u = searchURL(request().key(), request().value());
  if(u.isEmpty()) {
    stop();
    return;
  }

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &ArxivFetcher::slotComplete);
}

void ArxivFetcher::stop() {
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

void ArxivFetcher::slotComplete(KJob*) {
//  myDebug();

  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;
#if 0
  myWarning() << "Remove debug from arxivfetcher.cpp";
  QFile f(QLatin1String("/tmp/test.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << data;
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

  if(m_total == -1) {
    QDomDocument dom;
#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0))
    if(!dom.setContent(data, true /*namespace*/)) {
#else
    if(!dom.setContent(data, QDomDocument::ParseOption::UseNamespaceProcessing)) {
#endif
      myWarning() << "server did not return valid XML.";
      stop();
      return;
    }
    // total is top level element, with attribute totalResultsAvailable
    QDomNodeList list = dom.elementsByTagNameNS(XML::nsOpenSearch,
                                                QStringLiteral("totalResults"));
    if(list.count() > 0) {
      m_total = list.item(0).toElement().text().toInt();
    }
  }

  // assume result is always utf-8
  QString str = m_xsltHandler->applyStylesheet(QString::fromUtf8(data.constData(), data.size()));
  Import::TellicoImporter imp(str);
  Data::CollPtr coll = imp.collection();

  if(!coll) {
    myDebug() << "no valid result";
    stop();
    return;
  }

  foreach(Data::EntryPtr entry, coll->entries()) {
    if(!m_started) {
      // might get aborted
      break;
    }
    FetchResult* r = new FetchResult(this, entry);
    m_entries.insert(r->uid, entry);
    Q_EMIT signalResultFound(r);
  }

  m_start = m_entries.count();
  m_hasMoreResults = m_start < m_total;
  stop(); // required
}

Tellico::Data::EntryPtr ArxivFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries[uid_];
  // if URL but no cover image, fetch it
  if(!entry->field(QStringLiteral("url")).isEmpty()) {
    Data::CollPtr coll = entry->collection();
    Data::FieldPtr field = coll->fieldByName(QStringLiteral("cover"));
    if(!field && !coll->imageFields().isEmpty()) {
      field = coll->imageFields().front();
    } else if(!field) {
      field = Data::Field::createDefaultField(Data::Field::FrontCoverField);
      coll->addField(field);
    }
    if(entry->field(field).isEmpty()) {
      QPixmap pix = NetAccess::filePreview(QUrl::fromUserInput(entry->field(QStringLiteral("url"))));
      if(!pix.isNull()) {
        QString id = ImageFactory::addImage(pix, QStringLiteral("PNG"));
        if(!id.isEmpty()) {
          entry->setField(field, id);
        }
      }
    }
  }
  static const QRegularExpression versionRx(QStringLiteral("v\\d+$"));
  // if the original search was not for a versioned ID, remove it
  if(request().key() != ArxivID || !request().value().contains(versionRx)) {
    QString arxiv = entry->field(QStringLiteral("arxiv"));
    arxiv.remove(versionRx);
    entry->setField(QStringLiteral("arxiv"), arxiv);
  }
  return entry;
}

void ArxivFetcher::initXSLTHandler() {
  QString xsltfile = DataFileRegistry::self()->locate(QStringLiteral("arxiv2tellico.xsl"));
  if(xsltfile.isEmpty()) {
    myWarning() << "can not locate arxiv2tellico.xsl.";
    return;
  }

  QUrl u = QUrl::fromLocalFile(xsltfile);

  delete m_xsltHandler;
  m_xsltHandler = new XSLTHandler(u);
  if(!m_xsltHandler->isValid()) {
    myWarning() << "error in arxiv2tellico.xsl.";
    delete m_xsltHandler;
    m_xsltHandler = nullptr;
    return;
  }
}

QUrl ArxivFetcher::searchURL(FetchKey key_, const QString& value_) const {
  QUrl u(QString::fromLatin1(ARXIV_BASE_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("start"), QString::number(m_start));
  q.addQueryItem(QStringLiteral("max_results"), QString::number(ARXIV_RETURNS_PER_REQUEST));

  // quotes should be used if spaces are present
  QString value = value_;
  value.replace(QLatin1Char(' '), QLatin1Char('+'));
  // seems to have problems with dashes, too
  value.replace(QLatin1Char('-'), QLatin1Char('+'));

  QString query, id;
  switch(key_) {
    case Title:
      query = QStringLiteral("ti:%1").arg(value);
      break;

    case Person:
      query = QStringLiteral("au:%1").arg(value);
      break;

    case Keyword:
      // keyword gets to use all the words without being quoted
      query = QStringLiteral("all:%1").arg(value);
      break;

    case ArxivID:
      {
      // remove prefix and/or version number
      static const QRegularExpression arxivRx(QStringLiteral("^arxiv:"),
                                              QRegularExpression::CaseInsensitiveOption);
      static const QRegularExpression vRx(QStringLiteral("v\\d+$"));
      id = value_;
      id.remove(arxivRx);
      id.remove(vRx);
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  q.addQueryItem(QStringLiteral("search_query"), query);
  q.addQueryItem(QStringLiteral("id_list"), id);
  u.setQuery(q);

//  myDebug() << "url: " << u;
  return u;
}

Tellico::Fetch::FetchRequest ArxivFetcher::updateRequest(Data::EntryPtr entry_) {
  QString id = entry_->field(QStringLiteral("arxiv"));
  if(!id.isEmpty()) {
    // remove prefix and/or version number
    static const QRegularExpression arxivRx(QStringLiteral("^arxiv:"),
                                            QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression vRx(QStringLiteral("v\\d+$"));
    id.remove(arxivRx);
    id.remove(vRx);
    return FetchRequest(Fetch::ArxivID, id);
  }

  // optimistically try searching for title and rely on Collection::sameEntry() to figure things out
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Title, t);
  }

  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* ArxivFetcher::configWidget(QWidget* parent_) const {
  return new ArxivFetcher::ConfigWidget(parent_, this);
}

QString ArxivFetcher::defaultName() {
  return QStringLiteral("arXiv.org"); // no translation
}

QString ArxivFetcher::defaultIcon() {
  return favIcon("http://arxiv.org");
}

ArxivFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const ArxivFetcher*)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();
}

void ArxivFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString ArxivFetcher::ConfigWidget::preferredName() const {
  return ArxivFetcher::defaultName();
}
