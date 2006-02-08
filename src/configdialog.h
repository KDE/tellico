/****************************************************************************
    copyright            : (C) 2001-2006 by Robby Stephenson
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

namespace Tellico {
  class SourceListViewItem;
  namespace Fetch {
    class ConfigWidget;
  }
}

class KConfig;
class KLineEdit;
class KIntSpinBox;
class KPushButton;

class QCheckBox;

#include "fetch/fetch.h"
#include "gui/comboboxproxy.h"

#include <kdialogbase.h>
#include <klistview.h>

#include <qstringlist.h>
#include <qintdict.h>

namespace Tellico {

/**
 * The configuration dialog class allows the user to change the global application
 * preferences.
 *
 * @author Robby Stephenson
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
   * from the main Tellico object.
   *
   * @param config A pointer to the KConfig object
   */
  void saveConfiguration(KConfig* config);

signals:
  /**
   * Emitted whenever the Ok or Apply button is clicked.
   */
  void signalConfigChanged();

private slots:
  /**
   * Called when anything gets changed
   */
  void slotModified();
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
  void slotToggleCapitalized(bool checked);
  /**
   * Enable the checkboxes for formatting if formatting is enabled.
   *
   * @param checked The formatting checkbox is checkeed
   */
  void slotToggleFormatted(bool checked);
  /**
   * Update the help link for a page.
   *
   * @param w The page
   */
  void slotUpdateHelpLink(QWidget* w);
  /**
   * Create a new Internet source
   */
  void slotNewSourceClicked();
  /**
   * Modify a Internet source
   */
  void slotModifySourceClicked();
  /**
   * Remove a Internet source
   */
  void slotRemoveSourceClicked();
  /**
   * Load fetcher config
   */
  void loadFetcherConfig();
  void slotSelectedSourceChanged(QListViewItem* item);
  void slotMoveUpSourceClicked();
  void slotMoveDownSourceClicked();

private:
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
  void setupFetchPage();

  bool m_modifying;

  QCheckBox* m_cbWriteImagesInFile;
  QCheckBox* m_cbOpenLastFile;
  QCheckBox* m_cbShowTipDay;
  QCheckBox* m_cbCapitalize;
  QCheckBox* m_cbFormat;
  KLineEdit* m_leCapitals;
  KLineEdit* m_leArticles;
  KLineEdit* m_leSuffixes;
  KLineEdit* m_lePrefixes;

  QCheckBox* m_cbPrintHeaders;
  QCheckBox* m_cbPrintFormatted;
  QCheckBox* m_cbPrintGrouped;
  KIntSpinBox* m_imageWidthBox;
  KIntSpinBox* m_imageHeightBox;

  typedef Tellico::GUI::ComboBoxProxy<QString> CBProxy;
  QIntDict<CBProxy> m_cbTemplates;

  KListView* m_sourceListView;
  QMap<SourceListViewItem*, Fetch::ConfigWidget*> m_configWidgets;
  KPushButton* m_modifySourceBtn;
  KPushButton* m_moveUpSourceBtn;
  KPushButton* m_moveDownSourceBtn;
  KPushButton* m_removeSourceBtn;
};

class SourceListViewItem : public KListViewItem {
public:
  SourceListViewItem(KListView* parent, QListViewItem* after_, const QString& name_, Fetch::Type type_,
                     const QString& groupName_ = QString::null)
    : KListViewItem(parent, after_, name_), m_fetchType(type_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()) {}
  SourceListViewItem(KListView* parent, const QString& name_, Fetch::Type type_,
                     const QString& groupName_ = QString::null)
    : KListViewItem(parent, name_), m_fetchType(type_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()) {}

  void setConfigGroup(const QString& s) { m_configGroup = s; }
  const QString& configGroup() const { return m_configGroup; }
  const Fetch::Type& fetchType() const { return m_fetchType; }
  void setNewSource(bool b) { m_newSource = b; }
  bool isNewSource() const { return m_newSource; }

private:
  Fetch::Type m_fetchType;
  QString m_configGroup;
  bool m_newSource;
};

} // end namespace
#endif
