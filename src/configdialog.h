/****************************************************************************
    copyright            : (C) 2001-2004 by Robby Stephenson
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
class KComboBox;
class KIntSpinBox;

class QCheckBox;

#include <kdialogbase.h>
#include <kcombobox.h>

#include <qstringlist.h>
#include <qintdict.h>

namespace Bookcase {

/**
 * The configuration dialog class allows the user to change the global application
 * preferences.
 *
 * @author Robby Stephenson
 * @version $Id: configdialog.h 601 2004-04-10 21:14:02Z robby $
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
   * Sets-up the page for template options.
   */
  void setupTemplatePage();
  /**
   * Sets-up the page for bibtex options.
   */
  void setupBibliographyPage();

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
  /**
   * Preview an entry template.
   */
//  void slotPreview();
  /**
   * Update the help link for a page.
   *
   * @param w The page
   */
  void slotUpdateHelpLink(QWidget* w);

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
  KIntSpinBox* m_imageWidthBox;
  KIntSpinBox* m_imageHeightBox;

  QIntDict<KComboBox> m_cbTemplates;

  KLineEdit* m_leLyxpipe;
  KComboBox* m_cbBibtexStyle;
};

} // end namespace
#endif
