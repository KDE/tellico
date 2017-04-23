/***************************************************************************
    Copyright (C) 2001-2009 Robby Stephenson <robby@periapsis.org>
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

#ifndef TELLICO_DOCUMENT_H
#define TELLICO_DOCUMENT_H

#include "datavectors.h"
#include "filter.h"

#include <QObject>
#include <QPointer>
#include <QUrl>

namespace Tellico {
  namespace Import {
    class TellicoImporter;
    class TellicoSaxImporter;
  }

class MergeConflictResolver {
public:
  enum Result { KeepFirst, KeepSecond, CancelMerge };

  MergeConflictResolver() {}
  virtual ~MergeConflictResolver() {}
  virtual Result resolve(Data::EntryPtr entry1, Data::EntryPtr entry2, Data::FieldPtr field,
                         const QString& value1 = QString(), const QString& value2 = QString()) = 0;
};

  namespace Data {

/**
 * The Document contains everything needed to deal with the contents, thus separated from
 * the viewer, the Tellico object. It can take of opening and saving documents, and contains
 * a list of the collections in the document.
 *
 * @author Robby Stephenson
 */
class Document : public QObject {
Q_OBJECT

public:
  static Document* self() { if(!s_self) s_self = new Document(); return s_self; }

  /**
   * Sets the URL associated with the document.
   *
   * @param url The URL
   */
  void setURL(const QUrl& url);
  /**
   * Checks the modified flag, which indicates if the document has changed since the
   * last save.
   *
   * @return A boolean indicating the modified status
   */
  bool isModified() const { return m_isModified; }
  /**
   * Sets whether all images are loaded from file or not
   */
  void setLoadAllImages(bool loadAll) { m_loadAllImages = loadAll; }
  /**
   * Returns the current url associated with the document
   *
   * @return The url
   */
  const QUrl& URL() const { return m_url; }
  /**
   * Initializes a new document. The signalNewDoc() signal is emitted. The return
   * value is currently always true, but should indicate whether or not a new document
   * was correctly initialized.
   *
   * @param type The type of collection to add
   * @return A boolean indicating success
   */
  bool newDocument(int type);
  /**
   * Open a document given a specified location. If, for whatever reason, the file
   * cannot be opened, a proper message box is shown, indicating the problem. The
   * signalNewDoc() signal is made once the file contents have been confirmed.
   *
   * @param url The location to open
   * @return A boolean indicating success
   */
  bool openDocument(const QUrl& url);
  /**
   * Saves the document contents to a file.
   *
   * @param url The location to save the file
   * @param force Boolean indicating the file should be overwritten if necessary
   * @return A boolean indicating success
   */
  bool saveDocument(const QUrl& url, bool force = false);
  /**
   * Closes the document, deleting the contents. The return value is presently always true.
   *
   * @return A boolean indicating success
   */
  bool closeDocument();
  /**
   * Deletes the contents of the document. A signalCollectionDeleted() will be sent for every
   * collection in the document.
   */
  void deleteContents();
  /**
   * Returns a pointer to the document collection
   *
   * @return The collection
   */
  CollPtr collection() const;
  /**
   * Returns true if there are no entries. A doc with an empty collection is still empty.
   */
  bool isEmpty() const;
  /**
   * Appends the contents of another collection to the current one. The collections must be the
   * same type. Fields which are in the current collection are left alone. Fields
   * in the appended collection not in the current one are added. Entries in the appended collection
   * are added to the current one.
   *
   * @param coll A pointer to the appended collection.
   */
  void appendCollection(CollPtr coll);
  static void appendCollection(CollPtr targetColl, CollPtr sourceColl);
  /**
   * Merges another collection into this one. The collections must be the same type. Fields in the
   * current collection are left alone. Fields not in the current are added. The merging is slow
   * since each entry in @p coll must be compared to every entry in the current collection.
   *
   * @param coll A pointer to the collection to be merged.
   * @return A QPair of the merged entries, see note in datavectors.h
   */
  MergePair mergeCollection(CollPtr coll);
  static MergePair mergeCollection(CollPtr targetColl, CollPtr sourceColl);
  /**
   * Replace the current collection with a new one. Effectively, this is equivalent to opening
   * a new file containing this collection.
   *
   * @param coll A Pointer to the new collection, the document takes ownership.
   */
  void replaceCollection(CollPtr coll);
  void unAppendCollection(CollPtr coll, FieldList origFields);
  void unMergeCollection(CollPtr coll, FieldList origFields_, MergePair entryPair);
  bool loadAllImagesNow() const;
  bool allImagesOnDisk() const { return m_allImagesOnDisk; }
  int imageCount() const;
  EntryList filteredEntries(FilterPtr filter) const;

  void renameCollection(const QString& newTitle);

  void checkInEntry(EntryPtr entry);
  void checkOutEntry(EntryPtr entry);

  /**
   * The second entry vector contains entries with images which should not be removed
   * in addition to those already in the collection
   */
  void removeImagesNotInCollection(EntryList entries, EntryList entriesToKeep);
  void cancelImageWriting() { m_cancelImageWriting = true; }

  static bool mergeEntry(EntryPtr entry1, EntryPtr entry2, MergeConflictResolver* resolver=nullptr);
  // adds new fields into collection if any values in entries are not empty
  // first object is modified fields, second is new fields
  static QPair<Data::FieldList, Data::FieldList> mergeFields(Data::CollPtr coll,
                                                             Data::FieldList fields,
                                                             Data::EntryList entries);

public Q_SLOTS:
  /**
   * Sets the modified flag. If it is true, the signalModified signal is made.
   *
   * @param m A boolean indicating the current modified status
   */
  void slotSetModified(bool m=true);
  void slotSetClean(bool clean);

Q_SIGNALS:
  /**
   * Signals that the document has been modified.
   */
  void signalModified(bool modified);
  /**
   * Signals that a status message should be shown.
   *
   * @param str The message
   */
  void signalStatusMsg(const QString& str);
  /**
   * Signals that all images in the loaded file have been loaded
   * into memory or onto the disk
   */
  void signalCollectionImagesLoaded(Tellico::Data::CollPtr coll);
  void signalCollectionAdded(Tellico::Data::CollPtr coll);
  void signalCollectionDeleted(Tellico::Data::CollPtr coll);

private Q_SLOTS:
  /**
   * Does an initial loading of all images, used for writing
   * images to temp dir initially
   */
  void slotLoadAllImages();

private:
  static Document* s_self;

  /**
   * Writes all images in the current collection to the cache directory
   * if cacheDir = LocalDir, then url will be used and must not be empty
   */
  void writeAllImages(int cacheDir, const QUrl& url=QUrl());
  bool pruneImages();

  // make all constructors private
  Document();
  Document(const Document& doc);
  Document& operator=(const Document&);
  ~Document();

  CollPtr m_coll;
  bool m_isModified;
  bool m_loadAllImages;
  QUrl m_url;
  bool m_validFile;
  QPointer<Import::TellicoImporter> m_importer;
  bool m_cancelImageWriting;
  int m_fileFormat;
  bool m_allImagesOnDisk;
};

  } // end namespace
} // end namespace
#endif
