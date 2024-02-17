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

#include "discogsimporter.h"
#include "../collections/musiccollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/mapvalue.h"
#include "../tellico_debug.h"

#include <KConfigGroup>
#include <KLocalizedString>

#include <QLineEdit>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

namespace {
  static const char* DISCOGS_API_URL = "https://api.discogs.com";
}

using Tellico::Import::DiscogsImporter;

DiscogsImporter::DiscogsImporter() : Import::Importer(), m_widget(nullptr), m_userEdit(nullptr), m_tokenEdit(nullptr) {
}

void DiscogsImporter::setConfig(KSharedConfig::Ptr config_) {
  m_config = config_;
  KConfigGroup cg(m_config, QStringLiteral("ImportOptions - Discogs"));
  m_user = cg.readEntry("User ID");
  m_token = cg.readEntry("API Key"); // same config name as used in discogsfetcher
}

bool DiscogsImporter::canImport(int type) const {
  return type == Data::Collection::Album;
}

Tellico::Data::CollPtr DiscogsImporter::collection() {
  if(m_coll) {
    return m_coll;
  }

  if(!m_config) {
    setConfig(KSharedConfig::openConfig());
  }

  if(m_widget) {
    m_user = m_userEdit->text().trimmed();
    m_token = m_tokenEdit->text().trimmed();
  }

  if(m_user.isEmpty()) {
    myLog() << "Discogs importer is missing the user ID";
    setStatusMessage(i18n("A valid user ID must be entered."));
    return Data::CollPtr();
  }

  m_coll = new Data::MusicCollection(true);
  loadPage(1);
  return m_coll;
}

void DiscogsImporter::loadPage(int page_) {
  QUrl u(QString::fromLatin1(DISCOGS_API_URL));
  u.setPath(QStringLiteral("/users/%1/collection/folders/0/releases").arg(m_user));
  QUrlQuery q;
  q.addQueryItem(QLatin1String("page"), QString::number(page_));
  if(!m_token.isEmpty()) {
    q.addQueryItem(QStringLiteral("token"), m_token);
  }
  u.setQuery(q);
//  myDebug() << u;
  const QByteArray data = FileHandler::readDataFile(u, true /* quiet */);

#if 0
  myWarning() << "Remove output debug from discogsimporter.cpp";
  QFile f(QLatin1String("/tmp/discogs.json"));
  if(f.open(QIODevice::WriteOnly)) {
    QDataStream t(&f);
    t << data;
  }
  f.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  const QVariantMap resultMap = doc.object().toVariantMap();
  if(resultMap.contains(QStringLiteral("message")) && mapValue(resultMap, "id").isEmpty()) {
    const auto& msg = mapValue(resultMap, "message");
    myLog() << "DiscogsFetcher -" << msg;
  }
  const int totalPages = mapValue(resultMap, "pagination", "pages").toInt();
  myLog() << "Reading page" << page_ << "of" << totalPages << "from Discogs collection";
  foreach(const QVariant& release, resultMap.value(QLatin1String("releases")).toList()) {
    Data::EntryPtr entry(new Data::Entry(m_coll));
    const auto releaseMap = release.toMap();
    populateEntry(entry, releaseMap.value(QLatin1String("basic_information")).toMap());

    const QString rating = mapValue(releaseMap, "rating");
    if(!rating.isEmpty() && rating != QLatin1String("0")) {
      entry->setField(QStringLiteral("rating"), rating);
    }
    entry->setField(QStringLiteral("comments"), mapValue(releaseMap, "notes", "value"));
    m_coll->addEntries(entry);
  }

  if(totalPages > page_) {
    loadPage(page_+1);
  }
}

void DiscogsImporter::populateEntry(Data::EntryPtr entry_, const QVariantMap& releaseMap_) {
  entry_->setField(QStringLiteral("title"), mapValue(releaseMap_, "title"));
  const QString year = mapValue(releaseMap_, "year");
  if(year != QLatin1String("0")) {
    entry_->setField(QStringLiteral("year"), year);
  }
  // the styles value seems more like genres than the actual genres value
  entry_->setField(QStringLiteral("genre"),  mapValue(releaseMap_, "styles"));

  QStringList labels;
  foreach(const QVariant& label, releaseMap_.value(QLatin1String("labels")).toList()) {
    labels << mapValue(label.toMap(), "name");
  }
  entry_->setField(QStringLiteral("label"), labels.join(FieldFormat::delimiterString()));

  QStringList artists;
  foreach(const QVariant& artist, releaseMap_.value(QLatin1String("artists")).toList()) {
    artists << mapValue(artist.toMap(), "name");
  }
  artists.removeDuplicates(); // sometimes the same value is repeated
  entry_->setField(QStringLiteral("artist"), artists.join(FieldFormat::delimiterString()));

  foreach(const QVariant& format, releaseMap_.value(QLatin1String("formats")).toList()) {
    const QString formatName = mapValue(format.toMap(), "name");
    if(formatName == QLatin1String("CD")) {
      entry_->setField(QStringLiteral("medium"), i18n("Compact Disc"));
    } else if(formatName == QLatin1String("Vinyl")) {
      entry_->setField(QStringLiteral("medium"), i18n("Vinyl"));
    } else if(formatName == QLatin1String("Cassette")) {
      entry_->setField(QStringLiteral("medium"), i18n("Cassette"));
    } else if(formatName == QLatin1String("DVD")) {
      // sometimes a CD and DVD both are included. If we're using the CD, ignore the DVD
      entry_->setField(QStringLiteral("medium"), i18n("DVD"));
    }
  }

  QString coverUrl = mapValue(releaseMap_, "cover_image");
  if(coverUrl.isEmpty()) {
    coverUrl = mapValue(releaseMap_, "thumb");
  }
  if(!coverUrl.isEmpty()) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(coverUrl), true /* quiet */);
    // empty image ID is ok
    entry_->setField(QStringLiteral("cover"), id);
  }
}

QWidget* DiscogsImporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }
  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("Discogs Options"), m_widget);
  QFormLayout* lay = new QFormLayout(gbox);

  m_userEdit = new QLineEdit(gbox);
  m_userEdit->setText(m_user);
  m_tokenEdit = new QLineEdit(gbox);
  m_tokenEdit->setText(m_token);

  lay->addRow(i18n("User ID:"), m_userEdit);
  lay->addRow(i18n("User token:"), m_tokenEdit);

  l->addWidget(gbox);
  l->addStretch(1);

  return m_widget;
}
