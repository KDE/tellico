/***************************************************************************
                                bookcasedoc.h
                             -------------------
    begin                : Sun Sep 9 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#ifndef BOOKCASEDOC_H
#define BOOKCASEDOC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

class QFile;

#include "bccollection.h"

#include <kurl.h>

#include <qptrlist.h>
#include <qdom.h>
#include <qobject.h>

/**
 * The BookcaseDoc contains everything needed to deal with the contents, thus separated from
 * the viewer, the Bookcase object. It can take of opening and saving documents, and contains
 * a list of the collections in the document.
 *
 * @author Robby Stephenson
 * @version $Id: bookcasedoc.h,v 1.43 2003/03/10 02:13:49 robby Exp $
 */
class BookcaseDoc : public QObject  {
Q_OBJECT

public:
  /**
   * The constructor initializes the document. A new collection is created and added
   * to the document.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  BookcaseDoc(QWidget* parent, const char* name=0);

  /**
   * Sets the modified flag. If it is true, the signalModified signal is made.
   *
   * @param m A boolean indicating the current modified status
   */
  void setModified(bool m=true);
  /**
   * Sets the URL associated with the document.
   *
   * @param url The URL
   */
  void setURL(const KURL& url);
  /**
   * Checks the modified flag, which indicates if the document has changed since the
   * last save.
   *
   * @return A boolean indicating the modified status
   */
  bool isModified() const;
  /**
   * Returns the current url associated with the document
   *
   * @return The url
   */
  const KURL& URL() const;
  /**
   * Initializes a new document. The signalNewDoc() signal is emitted. The return
   * value is currently always true, but should indicate whether or not a new document
   * was correctly initialized.
   *
   * @return A boolean indicating success
   */
  bool newDocument();
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
   * Loads a DomDocument into a BookcaseDoc
   */
  bool loadDomDocument(const KURL& url, const QDomDocument& dom);
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
   * Writes the contents of a string to a url. If the file already exists, a "~" is appended
   * and the existing file is moved. The the file is remote, a temporary file is written and
   * then uploaded.
   *
   * @param url The url
   * @param text The text
   * @return A boolean indicating success
   */
  bool writeURL(const KURL& url, const QString& text) const;
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
   * Returns the number of collections in the document.
   *
   * @return The number of collections
   */
  unsigned collectionCount() const;
  /**
   * Locates a collection by its ID number. If none is found, a NULL pointer is returned.
   *
   * @param id The collection ID
   * @return A pointer to the collection
   */
  BCCollection* collectionById(int id);
  /**
   * Returns a list of all the collections in the document.
   *
   * @return The collection list
   */
  const BCCollectionList& collectionList() const;
  /**
   * Inserts the data in the document into a DOM ordered form by iterating over
   * the units in the whole document.
   *
   * @param format Whether to format the attributes
   * @return The QDomDocument object
   */
  QDomDocument exportXML(bool format=false);
  /**
   * Inserts the data in the document into a DOM ordered form by iterating through
   * a certain attribute group
   *
   * @param dictName The attribute name
   * @param format Whether to format the attributes
   * @return The QDomDocument object
   */
  QDomDocument exportXML(const QString& dictName, bool format);
  /**
   * Returns true if there are no units. A doc with an empty collection is still empty.
   */
  bool isEmpty() const; 
  /**
   * Loads the contents of a file into a QDomDocument.
   *
   * @param url The url of the file
   * @return A pointer to the QDomDocument, it must be deleted!
   */
  QDomDocument* readDocument(const KURL& url) const;

  static QDomDocument* importBibtex(const KURL& url);
  /**
   * Flags used for searching The options should be bit-wise OR'd together.
   * @li AllAttributes - Search through all attributes
   * @li AsRegExp - Use the text as the pattern for a regexp search
   * @li FindBackwards - search backwards
   * @li CaseSensitive - Case sensitive search
   * @li FromBeginning - Search from beginning
   */
  enum SearchOptions {
    AllAttributes = 1 << 0,
    AsRegExp      = 1 << 1,
    FindBackwards = 1 << 2,
    CaseSensitive = 1 << 3,
    FromBeginning = 1 << 4
  };
  /**
   * @param text Text to search for
   * @param attTitle Title of attribute to search, or empty for all
   * @param options The options, bit-wise OR'd together
   */
  void search(const QString& text, const QString& attTitle, int options);
  
  BCAttributeList uniqueAttributes(int type = 0) const;
  
public slots:
  /**
   * Adds a collection to the document. The signalCollectionAdded() signal is made.
   *
   * @param coll A pointer to the collection
   */
  void slotAddCollection(BCCollection* coll);
  /**
   * Removes a collection from the document. The signalCollectionDeleted() signal is made.
   *
   * @param coll A pointer to the collection
   */
  void slotDeleteCollection(BCCollection* coll);
  /**
   * Renames a collection, i.e. changes the collection title.
   *
   * @param id The ID of the collection to change
   * @param newName The new name
   */
  void slotRenameCollection(int id, const QString& newName);
  /**
   * Saves a unit. If the unit is already in a collection, the slotAddUnit() method is called;
   * otherwise, the signalUnitModified() signal is made.
   *
   * @param unit A pointer to the unit
   */
  void slotSaveUnit(BCUnit* unit);
  /**
   * Adds a unit to the document. It already contains a pointer to its proper collection
   * parent. The signalUnitAdded() signal is made.
   *
   * @param unit A pointer to the unit
   */
  void slotAddUnit(BCUnit* unit);
  /**
   * Removes a unit from the document. The signalUnitDeleted() signal is made.
   *
   * @param unit A pointer to the unit
   */
  void slotDeleteUnit(BCUnit* unit);

protected:
  /**
   * Writes the contents of a string to a file.
   *
   * @param file The file object
   * @param text The string
   * @return A boolean indicating success
   */
  bool writeFile(QFile& file, const QString& text) const;

signals:
  /**
   * Signals that the document has been modified.
   */
  void signalModified();
  /**
   * Signals that a status message should be shown.
   *
   * @param str The message
   */
  void signalStatusMsg(const QString& str);
  /**
   * Signals that a new collection has been added.
   *
   * @param coll A pointer to the collection
   */
  void signalCollectionAdded(BCCollection* coll);
  /**
   * Signals that a collection has been removed.
   *
   * @param coll A pointer to the collection
   */
  void signalCollectionDeleted(BCCollection* coll);
  /**
   * Signals that a new unit has been added.
   *
   * @param unit A pointer to the unit
   */
  void signalUnitAdded(BCUnit* unit);
  /**
   * Signals that a unit has been modified.
   *
   * @param unit A pointer to the unit
   */
  void signalUnitModified(BCUnit* unit);
  /**
   * Signals that a unit has been removed.
   *
   * @param unit A pointer to the unit
   */
  void signalUnitDeleted(BCUnit* unit);
  /**
   * Signals that a fraction of an operation has been completed.
   *
   * @param f The fraction, 0 =< f >= 1
   */
  void signalFractionDone(float f);
  void signalUnitSelected(BCUnit* unit);

private:
  BCCollectionList m_collList;
  bool m_isModified;
  KURL m_url;
};

#endif
