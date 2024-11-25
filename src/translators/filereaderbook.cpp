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

#include "filereaderbook.h"
#include "../fieldformat.h"
#include "../images/imagefactory.h"
#include "../core/netaccess.h"
#include "tellico_xml.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KFileItem>
#include <KZip>

#include <QPixmap>
#include <QIcon>
#include <QDomDocument>
#include <QImageReader>

namespace {
  static const int FILE_PREVIEW_SIZE = 128;
}

using Tellico::FileReaderBook;

class FileReaderBook::Private {
public:
  Private() = default;

  // cache the icon image ids to avoid repeated creation of Data::Image objects
  QHash<QString, QString> iconImageId;
};

FileReaderBook::FileReaderBook(const QUrl& url_) : FileReaderMetaData(url_), d(new Private) {
}

FileReaderBook::~FileReaderBook() = default;

bool FileReaderBook::populate(Data::EntryPtr entry, const KFileItem& item) {
  bool goodRead = false;
  // reads pdf and ebooks
  // special case for epub since the epubextractor in KFileMetaData doesn't read ISBN values
  if(item.mimetype() == QLatin1String("application/epub+zip")) {
    myLog() << "Reading" << item.url().toLocalFile();
    goodRead = readEpub(entry, item);
  } else if(item.mimetype() == QLatin1String("application/pdf") ||
            item.mimetype() == QLatin1String("application/fb2+zip") ||
            item.mimetype() == QLatin1String("application/fb2+xml") ||
            item.mimetype() == QLatin1String("application/x-mobipocket-ebook")) {
    goodRead = readMeta(entry, item);
  } else {
    return false;
  }
  if(!goodRead) return false;

  const QString url = QStringLiteral("url");
  if(!entry->collection()->hasField(url)) {
    Data::FieldPtr f(new Data::Field(url, i18n("URL"), Data::Field::URL));
    f->setCategory(i18n("Personal"));
    entry->collection()->addField(f);
  }
  entry->setField(url, item.url().url());
  entry->setField(QStringLiteral("binding"), i18n("E-Book"));

  // does it have a cover yet?
  if(entry->field(QStringLiteral("cover")).isEmpty()) {
    setCover(entry, item);
  }
  return true;
}

bool FileReaderBook::readEpub(Data::EntryPtr entry, const KFileItem& item) {
  KZip zip(item.url().toLocalFile());
  if(!zip.open(QIODevice::ReadOnly)) {
    myDebug() << "can't open zip";
    return false;
  }

  const KArchiveDirectory* topDir = zip.directory();
  const KArchiveEntry* container = topDir->entry(QStringLiteral("META-INF/container.xml"));
  if(!container || !container->isFile()) {
    myDebug() << "no container node";
    return false;
  }

  const QByteArray containerData = static_cast<const KArchiveFile*>(container)->data();
  QDomDocument dom;
#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0))
  if(!dom.setContent(containerData, false /* namespace processing */)) {
#else
  if(!dom.setContent(containerData, QDomDocument::ParseOption::Default)) {
#endif
    return false;
  }
  QDomNode n = dom.documentElement().namedItem(QLatin1String("rootfiles"))
                                    .namedItem(QLatin1String("rootfile"));
  const auto rootPath = n.toElement().attribute(QLatin1String("full-path"));
  const KArchiveEntry* rootFile = topDir->entry(rootPath);
  if(!rootFile || !rootFile->isFile()) {
    myDebug() << "no root file";
    return false;
  }
  const QByteArray rootData = static_cast<const KArchiveFile*>(rootFile)->data();
#if (QT_VERSION < QT_VERSION_CHECK(6, 5, 0))
  if(!dom.setContent(rootData, true /* namespace processing */)) {
#else
  if(!dom.setContent(rootData, QDomDocument::ParseOption::UseNamespaceProcessing)) {
#endif
    myDebug() << "bad root data";
    return false;
  }

  auto metaNode = dom.documentElement().namedItem(QLatin1String("metadata")).toElement();
  if(metaNode.isNull() || metaNode.namespaceURI() != XML::nsOpenPackageFormat) {
    myDebug() << "bad namespace:" << metaNode.namespaceURI();
    return false;
  }

  // index from document position into author ID
  QMap<int, QString> authorPositions;
  //  index from author ID to author name. Could have multiple empty ids so use MultiHash
  QHash<QString, QString> authorNames;
  QString coverRef;
  QStringList publishers, genres;
  auto childs = metaNode.childNodes();
  for(int i = 0; i < childs.count(); ++i) {
    auto child = childs.at(i);
    if(!child.isElement()) continue;
    if(child.namespaceURI() != XML::nsDublinCore &&
       child.namespaceURI() != XML::nsOpenPackageFormat) continue;
    const auto elemText = child.toElement().text();
    if(child.localName() == QLatin1String("title")) {
      entry->setField(QStringLiteral("title"), elemText);
    } else if(child.localName() == QLatin1String("creator")) {
      auto elem = child.toElement();
      auto opfRole = elem.attributeNS(XML::nsOpenPackageFormat, QLatin1String("role"));
      if(!opfRole.isEmpty() && opfRole != QLatin1String("aut")) {
        continue;
      }
      auto id = elem.attribute(QLatin1String("id"));
      if(id.isEmpty()) id = QLatin1Char('_') + QString::number(authorPositions.size());
      authorPositions.insert(authorPositions.size(), id);
      authorNames.insert(id, elemText);
    } else if(child.localName() == QLatin1String("publisher")) {
      publishers += elemText;
    } else if(child.localName() == QLatin1String("subject")) {
      // subjects as genre instead of keywords
      genres += elemText;
    } else if(child.localName() == QLatin1String("date")) {
      entry->setField(QStringLiteral("pub_year"), elemText.left(4));
    } else if(child.localName() == QLatin1String("description")) {
      entry->setField(QStringLiteral("plot"), elemText);
    } else if(child.localName() == QLatin1String("identifier")) {
      QString isbn;
      if(elemText.startsWith(QLatin1String("urn:isbn:"), Qt::CaseInsensitive)) {
        isbn = elemText.mid(9);
      } else {
        auto elem = child.toElement();
        if(elem.attributeNS(XML::nsOpenPackageFormat, QLatin1String("scheme")) == QLatin1String("ISBN") ||
           elem.attributeNS(XML::nsOpenPackageFormat, QLatin1String("id")) == QLatin1String("ISBN")) {
          isbn = elemText;
        }
      }
      if(!isbn.isEmpty()) {
        entry->setField(QStringLiteral("isbn"), isbn);
      }
    } else if(child.localName() == QLatin1String("meta")) {
      auto elem = child.toElement();
      auto refines = elem.attribute(QLatin1String("refines"));
      if(refines.startsWith(QLatin1Char('#'))) refines = refines.mid(1);
      if(!refines.isEmpty() && authorNames.contains(refines)) {
        // remove creators who are not authors
        if(elem.attribute(QLatin1String("property")) == QLatin1String("role") &&
           elem.text() != QLatin1String("aut")) {
          authorNames.remove(refines);
        }
      } else if(elem.attribute(QLatin1String("name")) == QLatin1String("cover")) {
        coverRef = elem.attribute(QLatin1String("content"));
      }
    }
  }

  if(!authorNames.isEmpty()) {
    // there's probably a better way to do an "ordered hash" where I have a pos, key, and value
    // and need to lookup by key but sort by pos. This brute force works well enough
    QStringList authors;
    for(auto i = authorPositions.cbegin(), end = authorPositions.cend(); i != end; ++i) {
      const auto& authorId = authorPositions[i.key()];
      if(authorNames.contains(authorId)) {
         authors << authorNames[authorId];  // only the valid author names
       }
    }
    entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
  }
  if(!publishers.isEmpty()) {
    entry->setField(QStringLiteral("publisher"), publishers.join(FieldFormat::delimiterString()));
  }
  if(!genres.isEmpty()) {
    entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }

  if(!coverRef.isEmpty()) {
    auto manifestNode = dom.documentElement().namedItem(QLatin1String("manifest")).toElement();
    if(manifestNode.isElement() && manifestNode.namespaceURI() == XML::nsOpenPackageFormat) {
      auto items = manifestNode.toElement().elementsByTagNameNS(XML::nsOpenPackageFormat,
                                                                QLatin1String("item"));
      for(int i = 0; i < items.count(); ++i) {
        auto item = items.at(i).toElement();
        if(item.attribute(QLatin1String("id")) == coverRef) {
          auto href = item.attribute(QLatin1String("href"));
          const auto mediaType = item.attribute(QLatin1String("media-type"));
          auto formats = QImageReader::imageFormatsForMimeType(mediaType.toLatin1());
          if(formats.isEmpty()) {
            myDebug() << "No image reader for" << mediaType;
          } else {
            // href is relative to rootPath
            if(rootPath.contains(QLatin1Char('/'))) {
              href = rootPath.section(QLatin1Char('/'), 0, -2) + QLatin1Char('/') + href;
            }
            const KArchiveEntry* coverEntry = topDir->entry(href);
            if(coverEntry && coverEntry->isFile()) {
              const QString id = ImageFactory::addImage(static_cast<const KArchiveFile*>(coverEntry)->data(),
                                                        QString::fromLatin1(formats.first()));
              entry->setField(QStringLiteral("cover"), id);
            }
          }
          break;
        }
      }
    }
  }

  return true;
}

bool FileReaderBook::readMeta(Data::EntryPtr entry, const KFileItem& item) {
#ifndef HAVE_KFILEMETADATA
  return false;
#else
  bool isEmpty = true;
  QStringList authors, publishers, genres, keywords;
  const auto props = properties(item);
  for(auto it = props.constBegin(); it != props.constEnd(); ++it) {
    const QString value = it.value().toString();
    if(value.isEmpty()) continue;
    switch(it.key()) {
      case KFileMetaData::Property::Title:
        isEmpty = false; // require a title or author
        entry->setField(QStringLiteral("title"), value);
        break;

      case KFileMetaData::Property::Author:
        isEmpty = false; // require a title or author
        authors += value;
        break;

      case KFileMetaData::Property::Publisher:
        publishers += value;
        break;

      case KFileMetaData::Property::Subject:
        keywords += value;
        break;

      case KFileMetaData::Property::Genre:
        genres += value;
        break;

      case KFileMetaData::Property::ReleaseYear:
        entry->setField(QStringLiteral("pub_year"), value);
        break;

      // is description usually the plot or just comments?
      case KFileMetaData::Property::Description:
        entry->setField(QStringLiteral("plot"), value);
        break;

      case KFileMetaData::Property::PageCount:
        entry->setField(QStringLiteral("pages"), value);
        break;

      default:
        if(!value.isEmpty()) {
//          myDebug() << "skipping" << it.key() << it.value();
        }
        break;
    }
  }

  if(isEmpty) return false;

  if(!authors.isEmpty()) {
    entry->setField(QStringLiteral("author"), authors.join(FieldFormat::delimiterString()));
  }
  if(!publishers.isEmpty()) {
    entry->setField(QStringLiteral("publisher"), publishers.join(FieldFormat::delimiterString()));
  }
  if(!genres.isEmpty()) {
    entry->setField(QStringLiteral("genre"), genres.join(FieldFormat::delimiterString()));
  }
  if(!keywords.isEmpty()) {
    entry->setField(QStringLiteral("keyword"), keywords.join(FieldFormat::delimiterString()));
  }
  return true;
#endif
}

void FileReaderBook::setCover(Data::EntryPtr entry, const KFileItem& item) {
  const QString cover = QStringLiteral("cover");
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
