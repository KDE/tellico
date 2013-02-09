/***************************************************************************
    Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>
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

#include "allocinefetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../gui/guiproxy.h"
#include "../tellico_utils.h"
#include "../tellico_debug.h"

#include <KLocale>
#include <KConfigGroup>
#include <KIntSpinBox>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>

namespace {
  static const int ALLOCINE_MAX_RETURNS_TOTAL = 20;
  static const char* ALLOCINE_API_KEY = "YW5kcm9pZC12M3M";
  static const char* ALLOCINE_API_URL = "http://api.allocine.fr/rest/v3/";
}

using namespace Tellico;
using Tellico::Fetch::AbstractAllocineFetcher;
using Tellico::Fetch::AllocineFetcher;

AbstractAllocineFetcher::AbstractAllocineFetcher(QObject* parent_, const QString& baseUrl_)
    : XMLFetcher(parent_)
    , m_apiKey(QLatin1String(ALLOCINE_API_KEY))
    , m_baseUrl(baseUrl_)
    , m_numCast(10) {
  setLimit(ALLOCINE_MAX_RETURNS_TOTAL);
  setXSLTFilename(QLatin1String("allocine2tellico.xsl"));
  Q_ASSERT(!m_baseUrl.isEmpty());
}

AbstractAllocineFetcher::~AbstractAllocineFetcher() {
}

bool AbstractAllocineFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

bool AbstractAllocineFetcher::canSearch(FetchKey k) const {
  return k == Keyword;
}

void AbstractAllocineFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("API Key", ALLOCINE_API_KEY);
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
  m_numCast = config_.readEntry("Max Cast", 10);
}

void AbstractAllocineFetcher::resetSearch() {
}

KUrl AbstractAllocineFetcher::searchUrl() {
  KUrl u(m_baseUrl);
  u.addPath(QLatin1String("search"));
  u.addQueryItem(QLatin1String("format"), QLatin1String("xml"));
  u.addQueryItem(QLatin1String("partner"), m_apiKey);

  switch(request().key) {
    case Keyword:
      u.addQueryItem(QLatin1String("q"), request().value);
      u.addQueryItem(QLatin1String("filter"), QLatin1String("movie"));
      break;

    default:
      myWarning() << "key not recognized: " << request().key;
      return KUrl();
  }

//  myDebug() << "url: " << u.url();
  return u;
}

void AbstractAllocineFetcher::parseData(QByteArray& data_) {
  Q_UNUSED(data_);
}

Tellico::Data::EntryPtr AbstractAllocineFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  QString code = entry_->field(QLatin1String("allocine-code"));
  if(code.isEmpty()) {
    myWarning() << "no allocine release found";
    return entry_;
  }

  KUrl u(m_baseUrl);
  u.addPath(QLatin1String("movie"));
  u.addQueryItem(QLatin1String("format"), QLatin1String("xml"));
  u.addQueryItem(QLatin1String("profile"), QLatin1String("large"));
  u.addQueryItem(QLatin1String("filter"), QLatin1String("movie"));
  u.addQueryItem(QLatin1String("partner"), m_apiKey);
  u.addQueryItem(QLatin1String("code"), code);
//  myDebug() << "url: " << u.url();

  // quiet
  QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from allocinefetcher.cpp";
  QFile f(QLatin1String("/tmp/test-allocine.xml"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << output;
  }
  f.close();
#endif

  Import::TellicoImporter imp(xsltHandler()->applyStylesheet(output));
  // be quiet when loading images
  imp.setOptions(imp.options() ^ Import::ImportShowImageErrors);
  Data::CollPtr coll = imp.collection();
  if(!coll || coll->entryCount() == 0) {
    myWarning() << "no collection pointer";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QLatin1String("allocine-code"));
  Data::EntryPtr entry = coll->entries().front();
  QStringList castRows = FieldFormat::splitTable(entry->field(QLatin1String("cast")));
  while(castRows.count() > m_numCast) {
    castRows.removeLast();
  }
  entry->setField(QLatin1String("cast"), castRows.join(FieldFormat::rowDelimiterString()));
  return entry;
}

Tellico::Fetch::FetchRequest AbstractAllocineFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QLatin1String("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

AbstractAllocineFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AbstractAllocineFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* label = new QLabel(i18n("&Maximum cast: "), optionsWidget());
  l->addWidget(label, ++row, 0);
  m_numCast = new KIntSpinBox(0, 99, 1, 10, optionsWidget());
  connect(m_numCast, SIGNAL(valueChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_numCast, row, 1);
  QString w = i18n("The list of cast members may include many people. Set the maximum number returned from the search.");
  label->setWhatsThis(w);
  m_numCast->setWhatsThis(w);
  label->setBuddy(m_numCast);

  l->setRowStretch(++row, 10);

  m_numCast->setValue(fetcher_ ? fetcher_->m_numCast : 10);
}

void AbstractAllocineFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  config_.writeEntry("Max Cast", m_numCast->value());
}

/**********************************************************************************************/

AllocineFetcher::AllocineFetcher(QObject* parent_)
    : AbstractAllocineFetcher(parent_, QLatin1String(ALLOCINE_API_URL)) {
}

QString AllocineFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

Tellico::Fetch::ConfigWidget* AllocineFetcher::configWidget(QWidget* parent_) const {
  return new AllocineFetcher::ConfigWidget(parent_, this);
}

QString AllocineFetcher::defaultName() {
  return QString::fromUtf8("AlloCiné.fr");
}

QString AllocineFetcher::defaultIcon() {
  return favIcon("http://www.allocine.fr");
}

Tellico::StringHash AllocineFetcher::allOptionalFields() {
  StringHash hash;
  hash[QLatin1String("origtitle")] = i18n("Original Title");
  hash[QLatin1String("allocine")]  = i18n("Allocine Link");
  return hash;
}

AllocineFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const AbstractAllocineFetcher* fetcher_)
    : AbstractAllocineFetcher::ConfigWidget(parent_, fetcher_) {
  // now add additional fields widget
  addFieldsWidget(AllocineFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString AllocineFetcher::ConfigWidget::preferredName() const {
  return AllocineFetcher::defaultName();
}

#include "allocinefetcher.moc"
