/***************************************************************************
    Copyright (C) 2017 Robby Stephenson <robby@periapsis.org>
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

#include "igdbfetcher.h"
#include "../collections/gamecollection.h"
#include "../images/imagefactory.h"
#include "../core/filehandler.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>
#include <KJob>
#include <KJobUiDelegate>
#include <KJobWidgets/KJobWidgets>
#include <KIO/StoredTransferJob>

#include <QUrl>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QGridLayout>
#include <QTextCodec>
#include <QJsonDocument>
#include <QJsonArray>

namespace {
  static const int IGDB_MAX_RETURNS_TOTAL = 20;
  static const char* IGDB_API_URL = "https://api-v3.igdb.com";
  static const char* IGDB_MAGIC_TOKEN = "dabb2142d9bca4936809251194acd4b25635dfe74d7f1d2d8ced8ab241700a3320111d7995f741769ca94b7de88b487812738fb9043247707a4e281e48786051";
}

using namespace Tellico;
using Tellico::Fetch::IGDBFetcher;

IGDBFetcher::IGDBFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
  if(m_genreHash.isEmpty()) {
    populateHashes();
  }
}

IGDBFetcher::~IGDBFetcher() {
}

QString IGDBFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

QString IGDBFetcher::attribution() const {
  return i18n("This information was freely provided by <a href=\"https://igdb.com\">IGDB.com</a>.");
}

bool IGDBFetcher::canSearch(FetchKey k) const {
  return k == Keyword;
}

bool IGDBFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void IGDBFetcher::readConfigHook(const KConfigGroup& config_) {
  QString k = config_.readEntry("APIv3 Key");
  if(!k.isEmpty()) {
    m_apiKey = k;
  }
}

void IGDBFetcher::search() {
  continueSearch();
}

void IGDBFetcher::continueSearch() {
  m_started = true;

  if(m_apiKey.isEmpty()) {
    m_apiKey = Tellico::reverseObfuscate(IGDB_MAGIC_TOKEN);
  }

  QUrl u(QString::fromLatin1(IGDB_API_URL));
  u.setPath(u.path() + QStringLiteral("/games/"));

  QStringList clauseList;
  switch(request().key) {
    case Keyword:
      clauseList += QString(QStringLiteral("search \"%1\";")).arg(request().value);
      break;

    default:
      myWarning() << "key not recognized:" << request().key;
      stop();
      return;
  }
  clauseList += QStringLiteral("fields *,cover.url,age_ratings.*,involved_companies.*;");
  // exclude some of the bigger unused fields
  clauseList += QStringLiteral("exclude keywords,screenshots,tags;");
  clauseList += QString(QStringLiteral("limit %1;")).arg(QString::number(IGDB_MAX_RETURNS_TOTAL));
//  myDebug() << u << clauseList.join(QStringLiteral(" "));

  m_job = igdbJob(u, m_apiKey, clauseList.join(QStringLiteral(" ")));
  connect(m_job, SIGNAL(result(KJob*)), SLOT(slotComplete(KJob*)));
}

void IGDBFetcher::stop() {
  if(!m_started) {
    return;
  }
  if(m_job) {
    m_job->kill();
    m_job = nullptr;
  }
  m_started = false;
  emit signalDone(this);
}

Tellico::Data::EntryPtr IGDBFetcher::fetchEntryHook(uint uid_) {
  if(!m_entries.contains(uid_)) {
    myDebug() << "no entry ptr";
    return Data::EntryPtr();
  }

  Data::EntryPtr entry = m_entries.value(uid_);

  QStringList publishers;
  // grab the publisher data
  if(entry->field(QStringLiteral("publisher")).isEmpty()) {
    foreach(const QString& pid, FieldFormat::splitValue(entry->field(QStringLiteral("pub-id")))) {
      const QString publisher = companyName(pid);
      if(!publisher.isEmpty()) {
        publishers << publisher;
      }
    }
  }
  entry->setField(QStringLiteral("publisher"), publishers.join(FieldFormat::delimiterString()));

  QStringList developers;
  // grab the developer data
  if(entry->field(QStringLiteral("developer")).isEmpty()) {
    foreach(const QString& did, FieldFormat::splitValue(entry->field(QStringLiteral("dev-id")))) {
      const QString developer = companyName(did);
      if(!developer.isEmpty()) {
        developers << developer;
      }
    }
  }
  entry->setField(QStringLiteral("developer"), developers.join(FieldFormat::delimiterString()));

  // image might still be a URL
  const QString image_id = entry->field(QStringLiteral("cover"));
  if(image_id.contains(QLatin1Char('/'))) {
    const QString id = ImageFactory::addImage(QUrl::fromUserInput(image_id), true /* quiet */);
    if(id.isEmpty()) {
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry->setField(QStringLiteral("cover"), id);
  }

  // clear the placeholder fields
  entry->setField(QStringLiteral("pub-id"), QString());
  entry->setField(QStringLiteral("dev-id"), QString());
  return entry;
}

Tellico::Fetch::FetchRequest IGDBFetcher::updateRequest(Data::EntryPtr entry_) {
  QString title = entry_->field(QStringLiteral("title"));
  if(!title.isEmpty()) {
    return FetchRequest(Keyword, title);
  }
  return FetchRequest();
}

void IGDBFetcher::slotComplete(KJob* job_) {
  KIO::StoredTransferJob* job = static_cast<KIO::StoredTransferJob*>(job_);

  if(job->error()) {
    job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }
  // see bug 319662. If fetcher is cancelled, job is killed
  // if the pointer is retained, it gets double-deleted
  m_job = nullptr;

#if 0
  myWarning() << "Remove debug from igdbfetcher.cpp";
  QFile file(QStringLiteral("/tmp/test.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t.setCodec("UTF-8");
    t << data;
  }
  file.close();
#endif

  Data::CollPtr coll(new Data::GameCollection(true));
  if(optionalFields().contains(QStringLiteral("pegi"))) {
    QStringList pegi = QStringLiteral("PEGI 3, PEGI 7, PEGI 12, PEGI 16, PEGI 18")
                                    .split(QRegExp(QStringLiteral("\\s*,\\s*")), QString::SkipEmptyParts);
    Data::FieldPtr field(new Data::Field(QStringLiteral("pegi"), i18n("PEGI Rating"), pegi));
    field->setFlags(Data::Field::AllowGrouped);
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  if(optionalFields().contains(QStringLiteral("igdb"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("igdb"), i18n("IGDB Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
  }
  // placeholder for publisher id, to be removed later
  Data::FieldPtr f(new Data::Field(QStringLiteral("pub-id"), QString(), Data::Field::Number));
  f->setFlags(Data::Field::AllowMultiple);
  coll->addField(f);
  // placeholder for developer id, to be removed later
  f = new Data::Field(QStringLiteral("dev-id"), QString(), Data::Field::Number);
  f->setFlags(Data::Field::AllowMultiple);
  coll->addField(f);

  QJsonDocument doc = QJsonDocument::fromJson(data);
  foreach(const QVariant& result, doc.array().toVariantList()) {
    QVariantMap resultMap = result.toMap();
    Data::EntryPtr entry(new Data::Entry(coll));
    populateEntry(entry, resultMap);

    FetchResult* r = new FetchResult(Fetcher::Ptr(this), entry);
    m_entries.insert(r->uid, entry);
    emit signalResultFound(r);
  }

  stop();
}

void IGDBFetcher::populateEntry(Data::EntryPtr entry_, const QVariantMap& resultMap_) {
  entry_->setField(QStringLiteral("title"), mapValue(resultMap_, "name"));
  entry_->setField(QStringLiteral("description"), mapValue(resultMap_, "summary"));

  QString cover = mapValue(resultMap_, "cover", "url");
  if(cover.startsWith(QLatin1Char('/'))) {
    cover.prepend(QStringLiteral("https:"));
  }
  entry_->setField(QStringLiteral("cover"), cover);

  QVariantList genreIDs = resultMap_.value(QStringLiteral("genres")).toList();
  QStringList genres;
  foreach(const QVariant& id, genreIDs) {
    QString g = m_genreHash.value(id.toInt());
    if(!g.isEmpty()) {
      genres << g;
    }
  }
  entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));

  qlonglong release_t = mapValue(resultMap_, "first_release_date").toLongLong();
  if(release_t > 0) {
    // could use QDateTime::fromSecsSinceEpoch but that was introduced in Qt 5.8
    // while I still support Qt 5.6, in theory...
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(release_t * 1000);
    entry_->setField(QStringLiteral("year"), QString::number(dt.date().year()));
  }

  QVariantList platforms = resultMap_.value(QStringLiteral("platforms")).toList();
  if(!platforms.isEmpty()) {
    // just take the first one for now
    // TODO:: suport multiple
    const QString platform = m_platformHash.value(platforms.first().toInt());
    if(platform == QStringLiteral("Nintendo Entertainment System (NES)")) {
      entry_->setField(QStringLiteral("platform"), i18n("Nintendo"));
    } else if(platform == QStringLiteral("Nintendo PlayStation")) {
      entry_->setField(QStringLiteral("platform"), i18n("PlayStation"));
    } else if(platform == QStringLiteral("PlayStation 2")) {
      entry_->setField(QStringLiteral("platform"), i18n("PlayStation2"));
    } else if(platform == QStringLiteral("PlayStation 3")) {
      entry_->setField(QStringLiteral("platform"), i18n("PlayStation3"));
    } else if(platform == QStringLiteral("PlayStation 4")) {
      entry_->setField(QStringLiteral("platform"), i18n("PlayStation4"));
    } else if(platform == QStringLiteral("PlayStation Portable")) {
      entry_->setField(QStringLiteral("platform"), i18nc("PlayStation Portable", "PSP"));
    } else if(platform == QStringLiteral("Wii")) {
      entry_->setField(QStringLiteral("platform"), i18n("Nintendo Wii"));
    } else if(platform == QStringLiteral("Nintendo GameCube")) {
      entry_->setField(QStringLiteral("platform"), i18n("GameCube"));
    } else if(platform == QStringLiteral("PC (Microsoft Windows)")) {
      entry_->setField(QStringLiteral("platform"), i18nc("Windows Platform", "Windows"));
    } else if(platform == QStringLiteral("Mac")) {
      entry_->setField(QStringLiteral("platform"), i18n("Mac OS"));
    } else {
      // TODO all the other platform translations
      // also make the assumption that if the platform name isn't already in the allowed list, it should be added
      Data::FieldPtr f = entry_->collection()->fieldByName(QStringLiteral("platform"));
      if(f && !f->allowed().contains(platform)) {
        f->setAllowed(QStringList(f->allowed()) << platform);
      }
      entry_->setField(QStringLiteral("platform"), platform);
    }
  }

  const QVariantList ageRatingList = resultMap_.value(QStringLiteral("age_ratings")).toList();
  foreach(const QVariant& ageRating, ageRatingList) {
    const QVariantMap ratingMap = ageRating.toMap();
    // per Age Rating Enums, ESRB==1, PEGI==2
    const int category = ratingMap.value(QStringLiteral("category")).toInt();
    const int rating = ratingMap.value(QStringLiteral("rating")).toInt();
    if(category == 1) {
      entry_->setField(QStringLiteral("certification"), m_esrbHash.value(rating));
    } else if(category == 2 && optionalFields().contains(QStringLiteral("pegi"))) {
      entry_->setField(QStringLiteral("pegi"), m_pegiHash.value(rating));
    }
  }

  QStringList pubs, devs;
  const QVariantList companyList = resultMap_.value(QStringLiteral("involved_companies")).toList();
  foreach(const QVariant& company, companyList) {
    const QVariantMap companyMap = company.toMap();
    if(companyMap.value(QStringLiteral("publisher")).toBool()) {
      pubs += companyMap.value(QStringLiteral("company")).toString();
    } else if(companyMap.value(QStringLiteral("developer")).toBool()) {
      devs += companyMap.value(QStringLiteral("company")).toString();
    }
  }
  entry_->setField(QStringLiteral("pub-id"), pubs.join(FieldFormat::delimiterString()));
  entry_->setField(QStringLiteral("dev-id"), devs.join(FieldFormat::delimiterString()));

  if(optionalFields().contains(QStringLiteral("igdb"))) {
    entry_->setField(QStringLiteral("igdb"), mapValue(resultMap_, "url"));
  }
}

QString IGDBFetcher::companyName(const QString& companyId_) const {
  if(m_companyHash.contains(companyId_)) {
    return m_companyHash.value(companyId_);
  }

  QUrl u(QString::fromLatin1(IGDB_API_URL));
  u.setPath(u.path() + QStringLiteral("/companies/"));

  QStringList clauseList;
  clauseList += QStringLiteral("fields name;");
  clauseList += QString(QStringLiteral("where id = %1;")).arg(companyId_);
//  myDebug() << u << clauseList.join(QStringLiteral(" "));

  QPointer<KIO::StoredTransferJob> job = igdbJob(u, m_apiKey, clauseList.join(QStringLiteral(" ")));
  if(!job->exec()) {
    myDebug() << job->errorString() << u;
    return QString();
  }
  const QByteArray data = job->data();
  if(data.isEmpty()) {
    myDebug() << "no data for" << u;
    return QString();
  }
#if 0
  myWarning() << "Remove company debug from igdbfetcher.cpp";
  QFile file(QStringLiteral("/tmp/igdb-company.json"));
  if(file.open(QIODevice::WriteOnly)) {
    QTextStream t(&file);
    t.setCodec("UTF-8");
    t << data;
  }
  file.close();
#endif

  QJsonDocument doc = QJsonDocument::fromJson(data);
  QVariantList companyList = doc.array().toVariantList();
  if(companyList.isEmpty()) {
    return QString();
  }
  const QString company = mapValue(companyList.at(0).toMap(), "name");
  m_companyHash.insert(companyId_, company);
  return company;
}

Tellico::Fetch::ConfigWidget* IGDBFetcher::configWidget(QWidget* parent_) const {
  return new IGDBFetcher::ConfigWidget(parent_, this);
}

// Use member hash for certain field names for now.
// Don't expect IGDB values to change. This avoids exponentially multiplying the number of API calls
void IGDBFetcher::populateHashes() {
  m_genreHash.insert(2,  QStringLiteral("Point-and-click"));
  m_genreHash.insert(4,  QStringLiteral("Fighting"));
  m_genreHash.insert(5,  QStringLiteral("Shooter"));
  m_genreHash.insert(7,  QStringLiteral("Music"));
  m_genreHash.insert(8,  QStringLiteral("Platform"));
  m_genreHash.insert(9,  QStringLiteral("Puzzle"));
  m_genreHash.insert(10, QStringLiteral("Racing"));
  m_genreHash.insert(11, QStringLiteral("Real Time Strategy (RTS)"));
  m_genreHash.insert(12, QStringLiteral("Role-playing (RPG)"));
  m_genreHash.insert(13, QStringLiteral("Simulator"));
  m_genreHash.insert(14, QStringLiteral("Sport"));
  m_genreHash.insert(15, QStringLiteral("Strategy"));
  m_genreHash.insert(16, QStringLiteral("Turn-based strategy (TBS)"));
  m_genreHash.insert(24, QStringLiteral("Tactical"));
  m_genreHash.insert(25, QStringLiteral("Hack and slash/Beat 'em up"));
  m_genreHash.insert(26, QStringLiteral("Quiz/Trivia"));
  m_genreHash.insert(30, QStringLiteral("Pinball"));
  m_genreHash.insert(31, QStringLiteral("Adventure"));
  m_genreHash.insert(32, QStringLiteral("Indie"));
  m_genreHash.insert(33, QStringLiteral("Arcade"));

  m_platformHash.insert(3, QStringLiteral("Linux"));
  m_platformHash.insert(4, QStringLiteral("Nintendo 64"));
  m_platformHash.insert(5, QStringLiteral("Wii"));
  m_platformHash.insert(6, QStringLiteral("PC (Microsoft Windows)"));
  m_platformHash.insert(7, QStringLiteral("PlayStation"));
  m_platformHash.insert(8, QStringLiteral("PlayStation 2"));
  m_platformHash.insert(9, QStringLiteral("PlayStation 3"));
  m_platformHash.insert(11, QStringLiteral("Xbox"));
  m_platformHash.insert(12, QStringLiteral("Xbox 360"));
  m_platformHash.insert(13, QStringLiteral("PC DOS"));
  m_platformHash.insert(14, QStringLiteral("Mac"));
  m_platformHash.insert(15, QStringLiteral("Commodore C64/128"));
  m_platformHash.insert(16, QStringLiteral("Amiga"));
  m_platformHash.insert(18, QStringLiteral("Nintendo Entertainment System (NES)"));
  m_platformHash.insert(19, QStringLiteral("Super Nintendo Entertainment System (SNES)"));
  m_platformHash.insert(20, QStringLiteral("Nintendo DS"));
  m_platformHash.insert(21, QStringLiteral("Nintendo GameCube"));
  m_platformHash.insert(22, QStringLiteral("Game Boy Color"));
  m_platformHash.insert(23, QStringLiteral("Dreamcast"));
  m_platformHash.insert(24, QStringLiteral("Game Boy Advance"));
  m_platformHash.insert(25, QStringLiteral("Amstrad CPC"));
  m_platformHash.insert(26, QStringLiteral("ZX Spectrum"));
  m_platformHash.insert(27, QStringLiteral("MSX"));
  m_platformHash.insert(29, QStringLiteral("Sega Mega Drive/Genesis"));
  m_platformHash.insert(30, QStringLiteral("Sega 32X"));
  m_platformHash.insert(32, QStringLiteral("Sega Saturn"));
  m_platformHash.insert(33, QStringLiteral("Game Boy"));
  m_platformHash.insert(34, QStringLiteral("Android"));
  m_platformHash.insert(35, QStringLiteral("Sega Game Gear"));
  m_platformHash.insert(36, QStringLiteral("Xbox Live Arcade"));
  m_platformHash.insert(37, QStringLiteral("Nintendo 3DS"));
  m_platformHash.insert(38, QStringLiteral("PlayStation Portable"));
  m_platformHash.insert(39, QStringLiteral("iOS"));
  m_platformHash.insert(41, QStringLiteral("Wii U"));
  m_platformHash.insert(42, QStringLiteral("N-Gage"));
  m_platformHash.insert(44, QStringLiteral("Tapwave Zodiac"));
  m_platformHash.insert(45, QStringLiteral("PlayStation Network"));
  m_platformHash.insert(46, QStringLiteral("PlayStation Vita"));
  m_platformHash.insert(47, QStringLiteral("Virtual Console (Nintendo)"));
  m_platformHash.insert(48, QStringLiteral("PlayStation 4"));
  m_platformHash.insert(49, QStringLiteral("Xbox One"));
  m_platformHash.insert(50, QStringLiteral("3DO Interactive Multiplayer"));
  m_platformHash.insert(51, QStringLiteral("Family Computer Disk System"));
  m_platformHash.insert(52, QStringLiteral("Arcade"));
  m_platformHash.insert(53, QStringLiteral("MSX2"));
  m_platformHash.insert(55, QStringLiteral("Mobile"));
  m_platformHash.insert(56, QStringLiteral("WiiWare"));
  m_platformHash.insert(57, QStringLiteral("WonderSwan"));
  m_platformHash.insert(58, QStringLiteral("Super Famicom"));
  m_platformHash.insert(59, QStringLiteral("Atari 2600"));
  m_platformHash.insert(60, QStringLiteral("Atari 7800"));
  m_platformHash.insert(61, QStringLiteral("Atari Lynx"));
  m_platformHash.insert(62, QStringLiteral("Atari Jaguar"));
  m_platformHash.insert(63, QStringLiteral("Atari ST/STE"));
  m_platformHash.insert(64, QStringLiteral("Sega Master System"));
  m_platformHash.insert(65, QStringLiteral("Atari 8-bit"));
  m_platformHash.insert(66, QStringLiteral("Atari 5200"));
  m_platformHash.insert(67, QStringLiteral("Intellivision"));
  m_platformHash.insert(68, QStringLiteral("ColecoVision"));
  m_platformHash.insert(69, QStringLiteral("BBC Microcomputer System"));
  m_platformHash.insert(70, QStringLiteral("Vectrex"));
  m_platformHash.insert(71, QStringLiteral("Commodore VIC-20"));
  m_platformHash.insert(72, QStringLiteral("Ouya"));
  m_platformHash.insert(73, QStringLiteral("BlackBerry OS"));
  m_platformHash.insert(74, QStringLiteral("Windows Phone"));
  m_platformHash.insert(75, QStringLiteral("Apple II"));
  m_platformHash.insert(77, QStringLiteral("Sharp X1"));
  m_platformHash.insert(78, QStringLiteral("Sega CD"));
  m_platformHash.insert(79, QStringLiteral("Neo Geo MVS"));
  m_platformHash.insert(80, QStringLiteral("Neo Geo AES"));
  m_platformHash.insert(82, QStringLiteral("Web browser"));
  m_platformHash.insert(84, QStringLiteral("SG-1000"));
  m_platformHash.insert(85, QStringLiteral("Donner Model 30"));
  m_platformHash.insert(86, QStringLiteral("TurboGrafx-16/PC Engine"));
  m_platformHash.insert(87, QStringLiteral("Virtual Boy"));
  m_platformHash.insert(88, QStringLiteral("Odyssey"));
  m_platformHash.insert(89, QStringLiteral("Microvision"));
  m_platformHash.insert(90, QStringLiteral("Commodore PET"));
  m_platformHash.insert(91, QStringLiteral("Bally Astrocade"));
  m_platformHash.insert(92, QStringLiteral("SteamOS"));
  m_platformHash.insert(93, QStringLiteral("Commodore 16"));
  m_platformHash.insert(94, QStringLiteral("Commodore Plus/4"));
  m_platformHash.insert(95, QStringLiteral("PDP-1"));
  m_platformHash.insert(96, QStringLiteral("PDP-10"));
  m_platformHash.insert(97, QStringLiteral("PDP-8"));
  m_platformHash.insert(98, QStringLiteral("DEC GT40"));
  m_platformHash.insert(99, QStringLiteral("Family Computer"));
  m_platformHash.insert(100, QStringLiteral("Analogue electronics"));
  m_platformHash.insert(101, QStringLiteral("Ferranti Nimrod Computer"));
  m_platformHash.insert(102, QStringLiteral("EDSAC"));
  m_platformHash.insert(103, QStringLiteral("PDP-7"));
  m_platformHash.insert(104, QStringLiteral("HP 2100"));
  m_platformHash.insert(105, QStringLiteral("HP 3000"));
  m_platformHash.insert(106, QStringLiteral("SDS Sigma 7"));
  m_platformHash.insert(107, QStringLiteral("Call-A-Computer time-shared mainframe computer system"));
  m_platformHash.insert(108, QStringLiteral("PDP-11"));
  m_platformHash.insert(109, QStringLiteral("CDC Cyber 70"));
  m_platformHash.insert(110, QStringLiteral("PLATO"));
  m_platformHash.insert(111, QStringLiteral("Imlac PDS-1"));
  m_platformHash.insert(112, QStringLiteral("Microcomputer"));
  m_platformHash.insert(113, QStringLiteral("OnLive Game System"));
  m_platformHash.insert(114, QStringLiteral("Amiga CD32"));
  m_platformHash.insert(115, QStringLiteral("Apple IIGS"));
  m_platformHash.insert(116, QStringLiteral("Acorn Archimedes"));
  m_platformHash.insert(117, QStringLiteral("Philips CD-i"));
  m_platformHash.insert(118, QStringLiteral("FM Towns"));
  m_platformHash.insert(119, QStringLiteral("Neo Geo Pocket"));
  m_platformHash.insert(120, QStringLiteral("Neo Geo Pocket Color"));
  m_platformHash.insert(121, QStringLiteral("Sharp X68000"));
  m_platformHash.insert(122, QStringLiteral("Nuon"));
  m_platformHash.insert(123, QStringLiteral("WonderSwan Color"));
  m_platformHash.insert(124, QStringLiteral("SwanCrystal"));
  m_platformHash.insert(125, QStringLiteral("PC-8801"));
  m_platformHash.insert(126, QStringLiteral("TRS-80"));
  m_platformHash.insert(127, QStringLiteral("Fairchild Channel F"));
  m_platformHash.insert(128, QStringLiteral("PC Engine SuperGrafx"));
  m_platformHash.insert(129, QStringLiteral("Texas Instruments TI-99"));
  m_platformHash.insert(130, QStringLiteral("Nintendo Switch"));
  m_platformHash.insert(131, QStringLiteral("Nintendo PlayStation"));
  m_platformHash.insert(132, QStringLiteral("Amazon Fire TV"));
  m_platformHash.insert(133, QStringLiteral("Philips Videopac G7000"));
  m_platformHash.insert(134, QStringLiteral("Acorn Electron"));
  m_platformHash.insert(135, QStringLiteral("Hyper Neo Geo 64"));
  m_platformHash.insert(136, QStringLiteral("Neo Geo CD"));

  // cheat by grabbing i18n values from default collection
  Data::CollPtr c(new Data::GameCollection(true));
  QStringList esrb = c->fieldByName(QStringLiteral("certification"))->allowed();
  Q_ASSERT(esrb.size() == 8);
  while(esrb.size() < 8) {
    esrb << QString();
  }
  m_esrbHash.insert(1, esrb.at(7));
  m_esrbHash.insert(2, esrb.at(6));
  m_esrbHash.insert(3, esrb.at(5));
  m_esrbHash.insert(4, esrb.at(4));
  m_esrbHash.insert(5, esrb.at(3));
  m_esrbHash.insert(6, esrb.at(2));
  m_esrbHash.insert(7, esrb.at(1));

  m_pegiHash.insert(1, QStringLiteral("PEGI 3"));
  m_pegiHash.insert(2, QStringLiteral("PEGI 7"));
  m_pegiHash.insert(3, QStringLiteral("PEGI 12"));
  m_pegiHash.insert(4, QStringLiteral("PEGI 16"));
  m_pegiHash.insert(5, QStringLiteral("PEGI 18"));
}

QString IGDBFetcher::defaultName() {
  return i18n("Internet Game Database (IGDB.com)");
}

QString IGDBFetcher::defaultIcon() {
  return favIcon("http://www.igdb.com");
}

Tellico::StringHash IGDBFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("pegi")] = i18n("PEGI Rating");
  hash[QStringLiteral("igdb")] = i18n("IGDB Link");
  return hash;
}

QPointer<KIO::StoredTransferJob> IGDBFetcher::igdbJob(const QUrl& url_, const QString& apiKey_, const QString& query_) {
  QPointer<KIO::StoredTransferJob> job = KIO::storedHttpPost(query_.toUtf8(), url_, KIO::HideProgressInfo);
  job->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("user-key: ") + apiKey_);
  job->addMetaData(QStringLiteral("accept"), QStringLiteral("application/json"));
  KJobWidgets::setWindow(job, GUI::Proxy::widget());
  return job;
}

IGDBFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const IGDBFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QGridLayout* l = new QGridLayout(optionsWidget());
  l->setSpacing(4);
  l->setColumnStretch(1, 10);

  int row = -1;

  QLabel* al = new QLabel(i18n("Registration is required for accessing the %1 data source. "
                               "If you agree to the terms and conditions, <a href='%2'>sign "
                               "up for an account</a>, and enter your information below.",
                                IGDBFetcher::defaultName(),
                                QStringLiteral("https://api.igdb.com/signup")),
                          optionsWidget());
  al->setOpenExternalLinks(true);
  al->setWordWrap(true);
  ++row;
  l->addWidget(al, row, 0, 1, 2);
  // richtext gets weird with size
  al->setMinimumWidth(al->sizeHint().width());

  QLabel* label = new QLabel(i18n("Access key: "), optionsWidget());
  l->addWidget(label, ++row, 0);

  m_apiKeyEdit = new QLineEdit(optionsWidget());
  connect(m_apiKeyEdit, SIGNAL(textChanged(const QString&)), SLOT(slotSetModified()));
  l->addWidget(m_apiKeyEdit, row, 1);
  label->setBuddy(m_apiKeyEdit);

  l->setRowStretch(++row, 10);

  // now add additional fields widget
  addFieldsWidget(IGDBFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());

  // don't show the default API key
  if(fetcher_ && fetcher_->m_apiKey != Tellico::reverseObfuscate(IGDB_MAGIC_TOKEN)) {
    m_apiKeyEdit->setText(fetcher_->m_apiKey);
  }
}

void IGDBFetcher::ConfigWidget::saveConfigHook(KConfigGroup& config_) {
  const QString apiKey = m_apiKeyEdit->text().trimmed();
  if(!apiKey.isEmpty()) {
    config_.writeEntry("APIv3 Key", apiKey);
  }
}

QString IGDBFetcher::ConfigWidget::preferredName() const {
  return IGDBFetcher::defaultName();
}
