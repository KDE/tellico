/****************************************************************************
                              configdialogh.h
                             -------------------
    begin                : Wed Dec 5 2001
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

class BookcaseDoc;

class KConfig;
class KLineEdit;
class KListBox;
class KComboBox;

#include <kdialogbase.h>

#include <qdict.h>
#include <qcheckbox.h>
#include <qstringlist.h>

/**
 * The configuration dialog class allows the user to change the global application
 * preferences.
 *
 * @author Robby Stephenson
 * @version $Id: configdialog.h,v 1.13 2003/03/08 18:24:47 robby Exp $
 */
class ConfigDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * The constructor sets up the Tabbed dialog pages.
   *
   * @param doc A pointer to the BookcaseDoc, used for filling out fields
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  ConfigDialog(BookcaseDoc* doc, QWidget* parent, const char* name=0);

  /**
   * Reads the current configuration. Only the options which are not saved somewhere
   * else are read at this point.
   *
   * @param config A pointer to the KConfig object
   */
  void readConfiguration(KConfig* config);
  /**
   * Saves the configuration. @ref KConfigBase::sync is called. This method is called
   * from the main Bookcase object.
   *
   * @param config A pointer to the KConfig object
   */
  void saveConfiguration(KConfig* config);
  bool configValue(const QString& key);

protected:
  /**
   * Sets-up the page for the general options.
   */
  void setupGeneralPage();
  /**
   * Sets-up the page for printing options.
   */
  void setupPrintingPage();
  /**
   * Sets-up the page for the book collection options.
   */
//  void setupBookPage();
  /**
   * Sets-up the page for the audio collection options.
   */
//  void setupAudioPage();
  /**
   * Sets-up the page for the video collection options.
   */
//  void setupVideoPage();

protected slots:
  /**
   * Called when the Ok button is clicked.
   */
  void slotOk();
  /**
   * Called when the Apply button is clicked.
   */
  void slotApply();
  /**
   * Called when the Default button is clicked.
   */
  void slotDefault();
  void slotFieldLeft();
  void slotFieldRight();
  void slotFieldUp();
  void slotFieldDown();
  void slotTogglePrintGrouped(bool checked);

signals:
  /**
   * Emitted whenever the Ok or Apply button is clicked.
   */
  void signalConfigChanged();

private:
  BookcaseDoc* m_doc;
  QCheckBox* m_cbOpenLastFile;
  QCheckBox* m_cbCapitalize;
  QCheckBox* m_cbFormat;
  QCheckBox* m_cbShowCount;
  KLineEdit* m_leArticles;
  KLineEdit* m_leSuffixes;

  QCheckBox* m_cbPrintHeaders;
  QCheckBox* m_cbPrintFormatted;
  QCheckBox* m_cbPrintGrouped;
  QStringList m_groupAttributes;
  KComboBox* m_cbPrintGroupAttribute;
  KListBox* m_lbAvailableFields;
  KListBox* m_lbSelectedFields;

  QDict<QCheckBox> m_cbDict;
};

#endif
