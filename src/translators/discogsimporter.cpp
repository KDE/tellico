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
#include "../utils/objvalue.h"
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
  const auto obj = doc.object();
  if(obj.contains(QLatin1StringView("message")) && objValue(obj, "id").isEmpty()) {
    const auto msg = objValue(obj, "message");
    myLog() << "DiscogsFetcher -" << msg;
  }
  const int totalPages = obj[QLatin1StringView("pagination")][QLatin1StringView("pages")].toInt();
  myLog() << "Reading page" << page_ << "of" << totalPages << "from Discogs collection";
  const auto releaseArray = obj[QLatin1StringView("releases")].toArray();
  for(const auto& release : releaseArray) {
    const auto releaseObj = release.toObject();
    Data::EntryPtr entry(new Data::Entry(m_coll));
    populateEntry(entry, releaseObj.value(QLatin1StringView("basic_information")).toObject());

    const QString rating = objValue(releaseObj, "rating");
    if(!rating.isEmpty() && rating != QLatin1String("0")) {
      entry->setField(QStringLiteral("rating"), rating);
    }
    entry->setField(QStringLiteral("comments"), objValue(releaseObj, "notes", "value"));
    m_coll->addEntries(entry);
  }

  if(totalPages > page_) {
    loadPage(page_+1);
  }
}

void DiscogsImporter::populateEntry(Data::EntryPtr entry_, const QJsonObject& releaseObj_) {
  entry_->setField(QStringLiteral("title"), objValue(releaseObj_, "title"));
  const QString year = objValue(releaseObj_, "year");
  if(year != QLatin1String("0")) {
    entry_->setField(QStringLiteral("year"), year);
  }
  // the styles value seems more like genres than the actual genres value
  entry_->setField(QStringLiteral("genre"), objValue(releaseObj_, "styles"));

  QStringList labels;
  const auto labelArray = releaseObj_[QLatin1StringView("labels")].toArray();
  for(const auto& label : labelArray) {
    labels << objValue(label.toObject(), "name");
  }
  entry_->setField(QStringLiteral("label"), labels.join(FieldFormat::delimiterString()));

  QStringList artists;
  const auto artistArray = releaseObj_[QLatin1StringView("artists")].toArray();
  for(const auto& artist : artistArray) {
    artists << objValue(artist.toObject(), "name");
  }
  artists.removeDuplicates(); // sometimes the same value is repeated
  entry_->setField(QStringLiteral("artist"), artists.join(FieldFormat::delimiterString()));

  const auto formatArray = releaseObj_[QLatin1StringView("formats")].toArray();
  for(const auto& format : formatArray) {
    const QString formatName = objValue(format.toObject(), "name");
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

  QString coverUrl = objValue(releaseObj_, "cover_image");
  if(coverUrl.isEmpty()) {
    coverUrl = objValue(releaseObj_, "thumb");
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
