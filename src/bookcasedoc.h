/***************************************************************************
                                bookcasedoc.h
                             -------------------
    begin                : Sun Sep 9 2001
    copyright            : (C) 2001, 2002, 2003 by Robby Stephenson
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

class Bookcase;
class BCUnit;
class BCCollection;

class QFile;
class QDomDocument;

#include <kurl.h>

#include <qptrlist.h>
#include <qobject.h>

/**
 * The BookcaseDoc contains everything needed to deal with the contents, thus separated from
 * the viewer, the Bookcase object. It can take of opening and saving documents, and contains
 * a list of the collections in the document.
 *
 * @author Robby Stephenson
 * @version $Id: bookcasedoc.h 260 2003-11-05 06:06:18Z robby $
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
  BookcaseDoc(Bookcase* parent, const char* name=0);
  /**
   * Destructor
   */
  ~BookcaseDoc();
  
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
  BCCollection* collection();
  /**
   * Returns true if there are no units. A doc with an empty collection is still empty.
   */
  bool isEmpty() const;
  /**
   * Replace the current collection with a new one. Effectively, this is equivalent to opening
   * a new file containg this collection.
   *
   * @param coll A Pointer to the new collection. @ref BookcaseDoc takes ownership.
   */
  void replaceCollection(BCCollection* coll);
  /**
   * Appends the contents of another collection to the current one. The collections must be the
   * same type. BCAttributes which are in the current collection are left alone. BCAttributes
   * in the appended collection not in the current one are added. BCUnits in the appended collection
   * are added to the current one.
   *
   * @param coll A Pointer to the appended collection.
   */
  void appendCollection(BCCollection* coll);
  /**
   * Adds a collection to the document. The signalCollectionAdded() signal is made.
   *
   * @param coll A pointer to the collection
   */
  void addCollection(BCCollection* coll);
  /**
   * Removes a collection from the document. The signalCollectionDeleted() signal is made.
   *
   * @param coll A pointer to the collection
   */
  void deleteCollection(BCCollection* coll);

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
 
public slots:
  /**
   * Renames a collection, i.e. changes the collection title. A dialog box
   * opens up for the user to input the new name.
   */
  void slotRenameCollection();
  /**
   * Converts the document's collection to a BibtexCollection.
   */
  void slotConvertToBibtex();
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
  /**
   * Sets the modified flag. If it is true, the signalModified signal is made.
   *
   * @param m A boolean indicating the current modified status
   */
  void slotSetModified(bool m=true);

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
   * Signals that the collection has been renamed.
   *
   * @param name The new collection name
   */
  void signalCollectionRenamed(const QString& name);
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
   * Signals that a unit should be selected, with an optional highlight string
   *
   * @param unit The unit to be selected
   * @param highlight The string in the unit which should be highlighted
   */
  void signalUnitSelected(BCUnit* unit, const QString& highlight);

private:
  BCCollection* m_coll;
  bool m_isModified;
  KURL m_url;
};

#endif
