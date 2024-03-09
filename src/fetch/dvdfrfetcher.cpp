/***************************************************************************
    Copyright (C) 2010 Robby Stephenson <robby@periapsis.org>
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

#include "dvdfrfetcher.h"
#include "../translators/xslthandler.h"
#include "../translators/tellicoimporter.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QUrlQuery>

namespace {
  static const int DVDFR_MAX_RETURNS_TOTAL = 20;
  static const char* DVDFR_SEARCH_API_URL = "http://www.dvdfr.com/api/search.php";
  static const char* DVDFR_DETAIL_API_URL = "http://www.dvdfr.com/api/dvd.php";
}

using namespace Tellico;
using Tellico::Fetch::DVDFrFetcher;

DVDFrFetcher::DVDFrFetcher(QObject* parent_)
    : XMLFetcher(parent_) {
  setLimit(DVDFR_MAX_RETURNS_TOTAL);
  setXSLTFilename(QStringLiteral("dvdfr2tellico.xsl"));
}

DVDFrFetcher::~DVDFrFetcher() {
}

QString DVDFrFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool DVDFrFetcher::canSearch(Fetch::FetchKey k) const {
  return k == Title || k == UPC;
}

bool DVDFrFetcher::canFetch(int type) const {
  return type == Data::Collection::Video;
}

QUrl DVDFrFetcher::searchUrl() {
  QUrl u(QString::fromLatin1(DVDFR_SEARCH_API_URL));

  QUrlQuery q;
  switch(request().key()) {
    case Title:
      q.addQueryItem(QStringLiteral("title"), request().value());
      break;

    case UPC:
      q.addQueryItem(QStringLiteral("gencode"), request().value());
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      return QUrl();
  }
  u.setQuery(q);
//  myDebug() << "url: " << u.url();
  return u;
}

Tellico::Data::EntryPtr DVDFrFetcher::fetchEntryHookData(Data::EntryPtr entry_) {
  Q_ASSERT(entry_);

  const QString id = entry_->field(QStringLiteral("dvdfr-id"));
  if(id.isEmpty()) {
    myDebug() << "no dvdfr id found";
    return entry_;
  }

  QUrl u(QString::fromLatin1(DVDFR_DETAIL_API_URL));
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("id"), id);
  u.setQuery(q);
//  myDebug() << "url: " << u;

  // quiet
  const QString output = FileHandler::readXMLFile(u, true);

#if 0
  myWarning() << "Remove output debug from dvdfrfetcher.cpp";
  QFile f(QLatin1String("/tmp/test-dvdfr-details.xml"));
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
  if(!coll) {
    myWarning() << "no collection pointer";
    return entry_;
  }
  if(coll->entryCount() == 0) {
    myWarning() << "no entries";
    return entry_;
  }

  if(coll->entryCount() > 1) {
    myDebug() << "weird, more than one entry found";
  }

  // don't want to include id
  coll->removeField(QStringLiteral("dvdfr-id"));
  return coll->entries().front();
}

Tellico::Fetch::FetchRequest DVDFrFetcher::updateRequest(Data::EntryPtr entry_) {
  QString barcode = entry_->field(QStringLiteral("barcode"));
  if(barcode.isEmpty()) {
    barcode = entry_->field(QStringLiteral("upc"));
  }
  if(!barcode.isEmpty()) {
    return FetchRequest(UPC, barcode);
  }

  const QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Title, title);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* DVDFrFetcher::configWidget(QWidget* parent_) const {
  return new DVDFrFetcher::ConfigWidget(parent_, this);
}

QString DVDFrFetcher::defaultName() {
  return QStringLiteral("DVDFr.com");
}

QString DVDFrFetcher::defaultIcon() {
  return favIcon("https://www.dvdfr.com");
}

Tellico::StringHash DVDFrFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("origtitle")] = i18n("Original Title");
  hash[QStringLiteral("dvdfr")]     = i18n("DVDFr Link");
  hash[QStringLiteral("alttitle")]  = i18n("Alternative Titles");
  hash[QStringLiteral("barcode")]   = i18n("Barcode");
  return hash;
}

DVDFrFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const DVDFrFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(DVDFrFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

void DVDFrFetcher::ConfigWidget::saveConfigHook(KConfigGroup&) {
}

QString DVDFrFetcher::ConfigWidget::preferredName() const {
  return DVDFrFetcher::defaultName();
}
