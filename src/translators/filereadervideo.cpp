/***************************************************************************
    Copyright (C) 2024 Robby Stephenson <robby@periapsis.org>
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

#include "filereadervideo.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../core/netaccess.h"
#include "../core/filehandler.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KFileItem>

#include <QPixmap>
#include <QIcon>
#include <QFileInfo>
#include <QDomDocument>

namespace {
  static const int FILE_PREVIEW_SIZE = 128;
}

using Tellico::FileReaderVideo;

class FileReaderVideo::Private {
public:
  Private() {}

  // cache the icon image ids to avoid repeated creation of Data::Image objects
  QHash<QString, QString> iconImageId;
};

FileReaderVideo::FileReaderVideo(const QUrl& url_) : FileReaderMetaData(url_), d(new Private) {
}

FileReaderVideo::~FileReaderVideo() = default;

bool FileReaderVideo::populate(Data::EntryPtr entry, const KFileItem& item) {
  // reads video files
  if(!item.mimetype().startsWith(QLatin1String("video"))) {
    return false;
  }
#ifndef HAVE_KFILEMETADATA
  return false;
#else
  bool isEmpty = true;
  QStringList genres, keywords;
  const auto props = properties(item);
  for(auto it = props.constBegin(); it != props.constEnd(); ++it) {
    const QString value = it.value().toString();
    if(value.isEmpty()) continue;
    switch(it.key()) {
      case KFileMetaData::Property::Title:
        isEmpty = false; // require a title or author
        entry->setField(QStringLiteral("title"), value);
        break;

      case KFileMetaData::Property::Subject:
        keywords += value;
        break;

      case KFileMetaData::Property::Genre:
        genres += value;
        break;

      case KFileMetaData::Property::ReleaseYear:
        entry->setField(QStringLiteral("year"), value);
        break;

      case KFileMetaData::Property::AspectRatio:
        entry->setField(QStringLiteral("aspect-ratio"), value);
        break;

      case KFileMetaData::Property::Duration:
        entry->setField(QStringLiteral("running-time"), QString::number(value.toInt()/60));
        break;

      case KFileMetaData::Property::Description:
        entry->setField(QStringLiteral("plot"), value);
        break;

      default:
        if(!value.isEmpty()) {
          myDebug() << "skipping" << it.key() << it.value();
        }
        break;
    }
  }

  if(!genres.isEmpty()) {
    entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }
  if(!keywords.isEmpty()) {
    entry->setField(QStringLiteral("keyword"), keywords.join(FieldFormat::delimiterString()));
  }

  // look for an NFO file
  QFileInfo info(item.localPath());
  const QString nfoFile = info.path() + QLatin1Char('/') + info.completeBaseName() + QLatin1String(".nfo");
  if(QFileInfo::exists(nfoFile)) {
    myLog() << "Reading" << nfoFile;
    isEmpty = !populateNfo(entry, nfoFile);
  }

  if(isEmpty) return false;

  const QString url = QStringLiteral("url");
  if(!entry->collection()->hasField(url)) {
    Data::FieldPtr f(new Data::Field(url, i18n("URL"), Data::Field::URL));
    f->setCategory(i18n("Personal"));
    entry->collection()->addField(f);
  }
  entry->setField(url, item.url().url());

  const QString cover = QStringLiteral("cover");
  const QString posterFile = info.path() + QLatin1Char('/') + info.completeBaseName() + QLatin1String("-poster.jpg");
  if(QFileInfo::exists(posterFile)) {
    const QString id = ImageFactory::addImage(QUrl::fromLocalFile(posterFile), true /* quiet */);
    entry->setField(cover, id);
  } else {
    QPixmap pixmap;
    if(useFilePreview()) {
      pixmap = Tellico::NetAccess::filePreview(item, FILE_PREVIEW_SIZE);
    }
    if(pixmap.isNull()) {
      if(d->iconImageId.contains(item.iconName())) {
        entry->setField(cover, d->iconImageId.value(item.iconName()));
      } else {
        pixmap = QIcon::fromTheme(item.iconName()).pixmap(QSize(FILE_PREVIEW_SIZE, FILE_PREVIEW_SIZE));
        const QString id = ImageFactory::addImage(pixmap, QStringLiteral("PNG"));
        if(!id.isEmpty()) {
          entry->setField(cover, id);
          d->iconImageId.insert(item.iconName(), id);
        }
      }
    } else {
      const QString id = ImageFactory::addImage(pixmap, QStringLiteral("PNG"));
      if(!id.isEmpty()) {
        entry->setField(cover, id);
      }
    }
  }

#endif
  return true;
}

bool FileReaderVideo::populateNfo(Data::EntryPtr entry_, const QString& nfoFile_) {
  const QUrl nfoUrl(QUrl::fromLocalFile(nfoFile_));
  QDomDocument dom;
  if(!dom.setContent(FileHandler::readDataFile(nfoUrl))) {
    myDebug() << "Failed to read contents of" << nfoFile_;
    return false;
  }

  auto root = dom.documentElement();
  if(root.nodeName() == QLatin1String("xml")) {
    root = root.firstChildElement(QLatin1String("movie"));
  }
  if(root.nodeName() != QLatin1String("movie")) {
    myDebug() << "Bad content of" << nfoFile_;
    return false;
  }

  bool isEmpty = true;
  QStringList genres, keywords, studios, writers, directors, actors;
  auto childList = root.childNodes();
  for(int i = 0; i < childList.count(); ++i) {
    auto elem = childList.at(i).toElement();
    if(elem.isNull()) continue;
    if(elem.tagName() == QLatin1String("title")) {
      entry_->setField(QStringLiteral("title"), elem.text());
      isEmpty = false;
    } else if(elem.tagName() == QLatin1String("originaltitle")) {
      const QString orig(QStringLiteral("origtitle"));
      if(!entry_->collection()->hasField(orig)) {
        Data::FieldPtr f(new Data::Field(orig, i18n("Original Title")));
        entry_->collection()->addField(f);
      }
      entry_->setField(orig, elem.text());
    } else if(elem.tagName() == QLatin1String("country")) {
      entry_->setField(QStringLiteral("nationality"), elem.text());
    } else if(elem.tagName() == QLatin1String("runtime")) {
      entry_->setField(QStringLiteral("running-time"), elem.text());
    } else if(elem.tagName() == QLatin1String("year")) {
      entry_->setField(QStringLiteral("year"), elem.text());
    } else if(elem.tagName() == QLatin1String("genre")) {
      genres += elem.text();
    } else if(elem.tagName() == QLatin1String("tag")) {
      keywords += elem.text();
    } else if(elem.tagName() == QLatin1String("studio")) {
      studios += elem.text();
    } else if(elem.tagName() == QLatin1String("credits")) {
      writers += elem.text();
    } else if(elem.tagName() == QLatin1String("director")) {
      directors += elem.text();
    } else if(elem.tagName() == QLatin1String("actor")) {
      auto nameElem = elem.firstChildElement(QLatin1String("name"));
      if(!nameElem.isNull()) {
        QString actor = nameElem.toElement().text();
        if(actor.isEmpty()) continue;
        auto roleElem = elem.firstChildElement(QLatin1String("role"));
        if(!roleElem.isNull()) {
          actor += FieldFormat::columnDelimiterString() + roleElem.text();
        }
        auto orderElem = elem.firstChildElement(QLatin1String("order"));
        if(orderElem.isNull()) {
          actors += actor;
        } else {
          const int pos = orderElem.text().toInt();
          while(pos >= actors.size()) actors += QString();
          actors[pos] = actor;
        }
      }
    } else if(elem.tagName() == QLatin1String("mpaa")) {
      static const QRegularExpression mpaaRx(QLatin1String("^(?:Rated)?\\s*(\\S+:)?(\\S+)"));
      auto match = mpaaRx.match(elem.text());
      if(match.hasMatch()) {
        QString certCountry = match.captured(1);
        if(certCountry.endsWith(QLatin1Char(':'))) certCountry.chop(1);
        if(certCountry.isEmpty() || certCountry == QLatin1String("US")) certCountry = QLatin1String("USA");
        const QString cert = QStringLiteral("%1 (%2)").arg(match.capturedRef(2), certCountry);
        entry_->setField(QStringLiteral("certification"), cert);
      }
    } else if(elem.tagName() == QLatin1String("plot")) {
      entry_->setField(QStringLiteral("plot"), elem.text());
    } else if(elem.tagName() == QLatin1String("uniqueid")) {
      const QString imdb(QStringLiteral("imdb"));
      const QString tmdb(QStringLiteral("tmdb"));
      if(elem.attribute(QLatin1String("type")) == imdb) {
        if(!entry_->collection()->hasField(imdb)) {
          entry_->collection()->addField(Data::Field::createDefaultField(Data::Field::ImdbField));
        }
        entry_->setField(imdb, QLatin1String("https://www.imdb.com/title/") + elem.text());
      } else if(elem.attribute(QLatin1String("type")) == tmdb) {
        if(!entry_->collection()->hasField(tmdb)) {
          Data::FieldPtr f(new Data::Field(QStringLiteral("tmdb"), i18n("TMDb Link"), Data::Field::URL));
          f->setCategory(i18n("General"));
          entry_->collection()->addField(f);
        }
        entry_->setField(tmdb, QLatin1String("https://www.themoviedb.org/movie/") + elem.text());
      }
    }
  }

  if(!genres.isEmpty()) {
    entry_->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }
  if(!keywords.isEmpty()) {
    entry_->setField(QStringLiteral("keyword"), keywords.join(FieldFormat::delimiterString()));
  }
  if(!studios.isEmpty()) {
    entry_->setField(QStringLiteral("studio"), studios.join(FieldFormat::delimiterString()));
  }
  if(!writers.isEmpty()) {
    entry_->setField(QStringLiteral("writer"), writers.join(FieldFormat::delimiterString()));
  }
  if(!directors.isEmpty()) {
    entry_->setField(QStringLiteral("director"), directors.join(FieldFormat::delimiterString()));
  }
  if(!actors.isEmpty()) {
    // could have empty values if the order value was out of whack
    actors.removeAll(QString());
    entry_->setField(QStringLiteral("cast"), actors.join(FieldFormat::rowDelimiterString()));
  }

  return !isEmpty;
}
