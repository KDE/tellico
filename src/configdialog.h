/****************************************************************************
                              configdialogh.h
                             -------------------
    begin                : Wed Dec 5 2001
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

class KConfig;
class KLineEdit;

#include <kdialogbase.h>

#include <qdict.h>
#include <qcheckbox.h>
#include <qstringlist.h>

/**
 * The configuration dialog class allows the user to change the global application
 * preferences.
 *
 * @author Robby Stephenson
 * @version $Id: configdialog.h 178 2003-10-11 03:36:05Z robby $
 */
class ConfigDialog : public KDialogBase {
Q_OBJECT

public:
  /**
   * The constructor sets up the Tabbed dialog pages.
   *
   * @param parent A pointer to the parent widget
   * @param name The widget name
   */
  ConfigDialog(QWidget* parent, const char* name=0);

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
  /**
   * Enable the checkboxes for formatting if formatting is enabled.
   *
   * @param checked The formatting checkbox is checkeed
   */
  void slotToggleFormatted(bool checked);

signals:
  /**
   * Emitted whenever the Ok or Apply button is clicked.
   */
  void signalConfigChanged();

private:
  QCheckBox* m_cbOpenLastFile;
  QCheckBox* m_cbShowTipDay;
  QCheckBox* m_cbCapitalize;
  QCheckBox* m_cbFormat;
  QCheckBox* m_cbShowCount;
  KLineEdit* m_leArticles;
  KLineEdit* m_leSuffixes;
  KLineEdit* m_lePrefixes;

  QCheckBox* m_cbPrintHeaders;
  QCheckBox* m_cbPrintFormatted;
  QCheckBox* m_cbPrintGrouped;
  QStringList m_groupAttributes;

  QDict<QCheckBox> m_cbDict;
};

#endif
