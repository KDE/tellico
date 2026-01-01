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

#include "vgcollectfetcher.h"
#include "../utils/guiproxy.h"
#include "../utils/string_utils.h"
#include "../collections/gamecollection.h"
#include "../entry.h"
#include "../core/filehandler.h"
#include "../images/imagefactory.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfig>
#include <KIO/StoredTransferJob>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>

#include <QRegularExpression>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
  static const char* VGCOLLECT_BASE_URL = "https://vgcollect.com/search/advanced";
}

using namespace Tellico;
using namespace Qt::StringLiterals;
using Tellico::Fetch::VGCollectFetcher;

VGCollectFetcher::VGCollectFetcher(QObject* parent_)
    : Fetcher(parent_), m_started(false) {
}

VGCollectFetcher::~VGCollectFetcher() {
}

QString VGCollectFetcher::source() const {
  return m_name.isEmpty() ? defaultName() : m_name;
}

bool VGCollectFetcher::canFetch(int type) const {
  return type == Data::Collection::Game;
}

void VGCollectFetcher::readConfigHook(const KConfigGroup& config_) {
  Q_UNUSED(config_);
}

void VGCollectFetcher::search() {
  m_started = true;
  m_matches.clear();

  QUrl u(QString::fromLatin1(VGCOLLECT_BASE_URL));
  QString urlPath(QStringLiteral("/no-filter/%1/no-filter/0/ALL/ALL/ALL/ALL/no-filter/%2/%3"));

  static const QRegularExpression yearRX(QStringLiteral("\\s*[12][0-9]{3}\\s*"));
  switch(request().key()) {
    case Keyword:
      {
        QString value = request().value();
        QString yearStart, yearEnd;
        // pull out year, keep the regexp a little loose
        auto match = yearRX.match(value);
        if(match.hasMatch()) {
          // fragile, but the form uses a year index
          yearStart = match.captured(0).trimmed() + QLatin1String("-01-01");
          yearEnd = match.captured(0).trimmed() + QLatin1String("-12-31");
          value = value.remove(yearRX);
        } else {
          yearStart = QStringLiteral("no-filter");
          yearEnd = yearStart;
        }
        urlPath = urlPath.arg(value, yearStart, yearEnd);
      }
      break;

    default:
      myWarning() << source() << "- key not recognized:" << request().key();
      stop();
      return;
  }
  u.setPath(u.path() + urlPath);
//  myDebug() << "url:" << u;

  m_job = KIO::storedGet(u, KIO::NoReload, KIO::HideProgressInfo);
  m_job->addMetaData(QStringLiteral("referrer"), QStringLiteral("https://vgcollect.com/search"));
  KJobWidgets::setWindow(m_job, GUI::Proxy::widget());
  connect(m_job.data(), &KJob::result,
          this, &VGCollectFetcher::slotComplete);
}

void VGCollectFetcher::stop() {
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

void VGCollectFetcher::slotComplete(KJob*) {
  if(m_job->error()) {
    m_job->uiDelegate()->showErrorMessage();
    stop();
    return;
  }

  const QByteArray data = m_job->data();
  if(data.isEmpty()) {
    myDebug() << "no data";
    stop();
    return;
  }

  // since the fetch is done, don't worry about holding the job pointer
  m_job = nullptr;

  const QString s = Tellico::decodeHTML(data);
#if 0
  myWarning() << "Remove debug from vgcollectfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << s;
  }
  f.close();
#endif

  static const QRegularExpression rowRx(QStringLiteral("<div class=\"span10\">(.+?)<div class=\"item-notes\""),
                                        QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression itemRx(QStringLiteral("<a href\\s*=\\s*\"(https://vgcollect.com/item/\\d+)\">(.+?)</a"));
  static const QRegularExpression platformRx(QStringLiteral("<a href=\"https://vgcollect.com/browse/[0-9a-z]+\">(.+?)</a"));
  static const QRegularExpression tagRx(QStringLiteral("<.*?>"));

  QRegularExpressionMatchIterator i = rowRx.globalMatch(s);
  while(i.hasNext()) {
    auto rowMatch = i.next();
    auto itemMatch = itemRx.match(rowMatch.captured(0));
    if(!itemMatch.hasMatch()) {
      continue;
    }
    auto u = itemMatch.captured(1);
    auto title = itemMatch.captured(2);
    QString platform;
    auto platformMatch = platformRx.match(rowMatch.captured(0));
    if(platformMatch.hasMatch()) {
      platform = platformMatch.captured(1);
      platform = platform.remove(tagRx).trimmed();
    }
    // skip some non-game "platforms"
    if(platform == "Toys"_L1 ||
       platform == "Clothing"_L1 ||
       platform == "Merchandise"_L1 ||
       platform == "Soundtrack"_L1 ||
       platform == "Books"_L1 ||
       platform == "Comics"_L1 ||
       platform == "GOG.com"_L1 ||
       platform.startsWith("Amiibo Figures"_L1) ||
       platform.contains("Video"_L1) ||
       platform == "Steam"_L1 ||
       platform == "Consoles"_L1 ||
       platform == "Accessory"_L1) {
        continue;
    }
//    myDebug() << title << platform << u;
    FetchResult* r = new FetchResult(this, title, platform);
    QUrl url = QUrl(QString::fromLatin1(VGCOLLECT_BASE_URL)).resolved(QUrl(u));
    m_matches.insert(r->uid, url);
    // don't emit signal until after putting url in matches hash
    Q_EMIT signalResultFound(r);
  }

  stop();
}

Tellico::Data::EntryPtr VGCollectFetcher::fetchEntryHook(uint uid_) {
  Data::EntryPtr entry = m_entries.value(uid_);
  if(entry || !m_matches.contains(uid_)) {
    return entry;
  }

  auto url = m_matches[uid_];
  QString results = Tellico::decodeHTML(FileHandler::readTextFile(url, true, true));
  if(results.isEmpty()) {
    myDebug() << "no text results from" << m_matches[uid_];
    return entry;
  }

#if 0
  myWarning() << "Remove debug2 from vgcollectfetcher.cpp";
  QFile f(QStringLiteral("/tmp/test2.html"));
  if(f.open(QIODevice::WriteOnly)) {
    QTextStream t(&f);
    t << results;
  }
  f.close();
#endif

  Data::CollPtr coll(new Data::GameCollection(true));
  entry = new Data::Entry(coll);
  parseEntry(entry, results);
  m_entries.insert(uid_, entry);

  const QString vgcollect(QStringLiteral("vgcollect"));
  if(optionalFields().contains(vgcollect)) {
    Data::FieldPtr field(new Data::Field(vgcollect, i18n("VGCollect Link"), Data::Field::URL));
    field->setCategory(i18n("General"));
    coll->addField(field);
    entry->setField(vgcollect, url.url());
  }

  // remove url to indicate the entry is fully populated
  m_matches.remove(uid_);
  return entry;
}

void VGCollectFetcher::parseEntry(Data::EntryPtr entry_, const QString& str_) {
  static const QRegularExpression divRx(QStringLiteral("<div class=\"tab-pane active\" id=\"info\">(.+?)</div"),
                                        QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression trRx(QStringLiteral("<tr>(.+?)</tr"),
                                       QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression tdRx(QStringLiteral("<td[^>]*>(.+?)</td"),
                                       QRegularExpression::DotMatchesEverythingOption);

  auto divMatch = divRx.match(str_);
  if(divMatch.hasMatch()) {
    auto i = trRx.globalMatch(divMatch.captured(1));
    while(i.hasNext()) {
      auto rowMatch = i.next();
      auto headerMatch = tdRx.match(rowMatch.captured(1));
      if(headerMatch.hasMatch()) {
        auto valueMatch = tdRx.match(rowMatch.captured(1), headerMatch.capturedEnd());
        if(valueMatch.hasMatch()) {
          populateValue(entry_, headerMatch.captured(1), valueMatch.captured(1));
        }
      }
    }
  }

  static const QRegularExpression titleRx(QStringLiteral("<meta property=\"og:title\" content=\"([^\"]+?) \\|"));
  auto titleMatch = titleRx.match(str_);
  if(titleMatch.hasMatch()) {
    entry_->setField(QStringLiteral("title"), titleMatch.captured(1));
  }

  static const QRegularExpression coverRx(QStringLiteral("<meta property=\"og:image\" content=\"(.+?)\">"));
  auto coverMatch = coverRx.match(str_);
  if(coverMatch.hasMatch()) {
    const QString u = coverMatch.captured(1);
    const QUrl coverUrl = QUrl(QString::fromLatin1(VGCOLLECT_BASE_URL)).resolved(QUrl(u));

    const QString id = ImageFactory::addImage(coverUrl, true /* quiet */);
    if(id.isEmpty()) {
      myDebug() << "Could not load" << coverUrl;
      message(i18n("The cover image could not be loaded."), MessageHandler::Warning);
    }
    // empty image ID is ok
    entry_->setField(QStringLiteral("cover"), id);
  }
}

void VGCollectFetcher::populateValue(Data::EntryPtr entry_, const QString& header_, const QString& value_) const {
  static const QRegularExpression tagRx(QStringLiteral("<.*?>"));
  auto header = header_;
  header = header.remove(tagRx).simplified();
  auto value = value_.simplified();
  if(header_.isEmpty() || value_.isEmpty() || value == "NA"_L1) {
    return;
  }

  if(header.startsWith("Publisher"_L1)) {
    entry_->setField(QStringLiteral("publisher"), value);
  } else if(header.startsWith("Developer"_L1)) {
    entry_->setField(QStringLiteral("developer"), value);
  } else if(header.startsWith("Platform"_L1)) {
    const QString platform = Data::GameCollection::normalizePlatform(value.remove(tagRx).trimmed());
    entry_->setField(QStringLiteral("platform"), platform);
  } else if(header.startsWith("Genre"_L1)) {
    entry_->setField(QStringLiteral("genre"), value);
  } else if(header.startsWith("Rating"_L1)) {
    QString pegi;
    Data::GameCollection::EsrbRating esrb = Data::GameCollection::UnknownEsrb;
    if(value.contains("u.png"_L1))         esrb = Data::GameCollection::Unrated;
    else if(value.contains("esrb-t"_L1))   esrb = Data::GameCollection::Teen;
    else if(value.contains("esrb-e"_L1))   esrb = Data::GameCollection::Everyone;
    else if(value.contains("esrb-ka"_L1))  esrb = Data::GameCollection::Everyone;
    else if(value.contains("esrb-e10"_L1)) esrb = Data::GameCollection::Everyone10;
    else if(value.contains("esrb-ec"_L1))  esrb = Data::GameCollection::EarlyChildhood;
    else if(value.contains("esrb-m"_L1))   esrb = Data::GameCollection::Mature;
    else if(value.contains("esrb-ao"_L1))  esrb = Data::GameCollection::Adults;
    else if(value.contains("pegi-3"_L1))   pegi = QStringLiteral("PEGI 3");
    else if(value.contains("pegi-7"_L1))   pegi = QStringLiteral("PEGI 7");
    else if(value.contains("pegi-12"_L1))  pegi = QStringLiteral("PEGI 12");
    else if(value.contains("pegi-16"_L1))  pegi = QStringLiteral("PEGI 16");
    else if(value.contains("pegi-18"_L1))  pegi = QStringLiteral("PEGI 18");
    if(esrb != Data::GameCollection::UnknownEsrb) {
      entry_->setField(QStringLiteral("certification"), Data::GameCollection::esrbRating(esrb));
    }
    if(!pegi.isEmpty() && optionalFields().contains(QStringLiteral("pegi"))) {
      entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::PegiField));
      entry_->setField(QStringLiteral("pegi"), pegi);
    }
  } else if(header.startsWith("Release Date"_L1)) {
    entry_->setField(QStringLiteral("year"), value.right(4));
  } else if(header.startsWith("Box Text"_L1)) {
    entry_->setField(QStringLiteral("description"), value);
  } else if(header.startsWith("Barcode"_L1) &&
            optionalFields().contains(QStringLiteral("barcode"))) {
    Data::FieldPtr field(new Data::Field(QStringLiteral("barcode"), i18n("Barcode")));
    field->setCategory(i18n("General"));
    entry_->collection()->addField(field);
    entry_->setField(QStringLiteral("barcode"), value);
  }
}

Tellico::Fetch::FetchRequest VGCollectFetcher::updateRequest(Data::EntryPtr entry_) {
  QString t = entry_->field(QStringLiteral("title"));
  if(!t.isEmpty()) {
    return FetchRequest(Fetch::Keyword, t);
  }
  return FetchRequest();
}

Tellico::Fetch::ConfigWidget* VGCollectFetcher::configWidget(QWidget* parent_) const {
  return new VGCollectFetcher::ConfigWidget(parent_, this);
}

QString VGCollectFetcher::defaultName() {
  return QStringLiteral("VGCollect");
}

QString VGCollectFetcher::defaultIcon() {
  return favIcon("https://vgcollect.com/assets/favicon.ico");
}

//static
Tellico::StringHash VGCollectFetcher::allOptionalFields() {
  StringHash hash;
  hash[QStringLiteral("vgcollect")] = i18n("VGCollect Link");
  hash[QStringLiteral("pegi")]      = i18n("PEGI Rating");
  hash[QStringLiteral("barcode")]   = i18n("Barcode");
  return hash;
}

VGCollectFetcher::ConfigWidget::ConfigWidget(QWidget* parent_, const VGCollectFetcher* fetcher_)
    : Fetch::ConfigWidget(parent_) {
  QVBoxLayout* l = new QVBoxLayout(optionsWidget());
  l->addWidget(new QLabel(i18n("This source has no options."), optionsWidget()));
  l->addStretch();

  // now add additional fields widget
  addFieldsWidget(VGCollectFetcher::allOptionalFields(), fetcher_ ? fetcher_->optionalFields() : QStringList());
}

QString VGCollectFetcher::ConfigWidget::preferredName() const {
  return VGCollectFetcher::defaultName();
}
