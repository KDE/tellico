/*  *************************************************************************
                              configdialogh.h
                             -------------------
    begin                : Wed Dec 5 2001
    copyright            : (C) 2001 by Robby Stephenson
    email                : robby@radiojodi.com
 * *************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 * *************************************************************************/

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

class KConfig;
class QCheckBox;
class KLineEdit;

#include <kdialogbase.h>

/**
 * The configuration dialog class allows the user to change the global application preferences.
 *
 * @author Robby Stephenson
 * @version $Id: configdialog.h,v 1.3 2002/02/09 07:11:25 robby Exp $
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
   */
  ~ConfigDialog();

  /**
   * Reads the current configuration. Only the options which are not saved somewhere else
   * are read at this point.
   *
   * @param config A pointer to the KConfig object
   */
  void readConfiguration(KConfig* config);

  /**
   * Saves the configuration. @ref KConfigBase::sync is called. This method is called from
   * the main Bookcase object.
   *
   * @param config A pointer to the KConfig object
   */
  void saveConfiguration(KConfig* config);

protected:
  /**
   * Sets-up the page for the general options.
   */
  void setupGeneralPage();
  /**
   * Sets-up the page for the book collection options.
   */
  void setupBookPage();
  /**
   * Sets-up the page for the audio collection options.
   */
  void setupAudioPage();
  /**
   * Sets-up the page for the video collection options.
   */
  void setupVideoPage();

protected slots:
  /**
   * Called when the Ok button is clicked.
   */
  void slotOk();
  /**
   * Called when the Apply button is clicked.
   */
  void slotApply();

signals:
  /**
   * Emitted whenever the Ok or Apply button is clicked.
   */
  void signalConfigChanged();

private:
  QCheckBox* m_cbOpenLastFile;
  QCheckBox* m_cbCapitalize;
  KLineEdit* m_leArticles;
  KLineEdit* m_leSuffixes;
};

#endif
