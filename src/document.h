/***************************************************************************
    copyright            : (C) 2001-2005 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef TELLICO_DOCUMENT_H
#define TELLICO_DOCUMENT_H

#include <config.h>

#include "datavectors.h"

#include <kurl.h>
#include <ksharedptr.h>

#include <qptrlist.h>
#include <qobject.h>

namespace Tellico {
  namespace Data {
    class Collection;

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
  void setURL(const KURL& url) { m_url = url; }
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
  const KURL& URL() const { return m_url; }
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
   * signalNewDoc() signal is made once the file contents have been confirmed. The
   * signalFractionDone() signal is made periodically, indicating progress.
   *
   * @param url The location to open
   * @return A boolean indicating success
   */
  bool openDocument(const KURL& url);
  /**
   * Checks to see if the document has been modified before deleting the contents.
   * If it has, then a message box asks the user if the document should be saved,
   * and then acts on the result.
   *
   * @return A boolean indicating success
   */
  bool saveModified();
  /**
   * Saves the document contents to a file.
   *
   * @param url The location to save the file
   * @return A boolean indicating success
   */
  bool saveDocument(const KURL& url);
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
  Collection* collection() const { return m_coll; }
  /**
   * Returns true if there are no entries. A doc with an empty collection is still empty.
   */
  bool isEmpty() const;
  /**
   * Appends the contents of another collection to the current one. The collections must be the
   * same type. Fields which are in the current collection are left alone. Fields
   * in the appended collection not in the current one are added. Entrys in the appended collection
   * are added to the current one.
   *
   * @param coll A pointer to the appended collection.
   */
  void appendCollection(Collection* coll);
  /**
   * Merges another collection into this one. The collections must be the same type. Fields in the
   * current collection are left alone. Fields not in the current are added. The merging is slow
   * since each entry in @p coll must be compared to ever entry in the current collection.
   *
   * @param coll A pointer to the collection to be merged.
   * @return A QPair of the merged entries, see note in datavectors.h
   */
  MergePair mergeCollection(Collection* coll);
  /**
   * Replace the current collection with a new one. Effectively, this is equivalent to opening
   * a new file containg this collection.
   *
   * @param coll A Pointer to the new collection, the document takes ownership.
   */
  void replaceCollection(Collection* coll);
  void unAppendCollection(Collection* coll, FieldVec origFields);
  void unMergeCollection(Collection* coll, FieldVec origFields_, MergePair entryPair);
  /**
   * Attempts to load an image from the document file
   */
  bool loadImage(const QString& id);
  EntryVec filteredEntries(const Filter* filter) const;

  void renameCollection(const QString& newTitle);

  void checkInEntry(Data::Entry* entry);
  void checkOutEntry(Data::Entry* entry);

public slots:
  /**
   * Sets the modified flag. If it is true, the signalModified signal is made.
   *
   * @param m A boolean indicating the current modified status
   */
  void slotSetModified(bool m=true);
  void slotDocumentRestored();

signals:
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
   * Signals that a fraction of an operation has been completed.
   *
   * @param f The fraction, 0 =< f >= 1
   */
  void signalFractionDone(float f);


private:
  static Document* s_self;

  /**
   * Saves a list of entries. If the entry is already in a collection, the slotAddEntry() method is called;
   * otherwise, the signalEntryModified() signal is made.
   *
   * @param entry A pointer to the entry
   */
  void saveEntry(Tellico::Data::Entry* entry);
  // make all constructors private
  Document();
  Document(const Document& doc);
  Document& operator=(const Document&);

  KSharedPtr<Collection> m_coll;
  bool m_isModified;
  bool m_loadAllImages;
  KURL m_url;
  bool m_validFile;
};

  } // end namespace
} // end namespace
#endif
