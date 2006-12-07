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

#include "fetch/fetcher.h"
#include "gui/combobox.h"

#include <kdialogbase.h>
#include <klistview.h>

#include <qstringlist.h>
#include <qintdict.h>

class KConfig;
class KLineEdit;
class KIntSpinBox;
class KPushButton;
class KIntNumInput;
class KFontCombo;
class KColorCombo;

class QCheckBox;

namespace Tellico {
  class SourceListViewItem;
  namespace Fetch {
    class ConfigWidget;
  }

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
  virtual ~ConfigDialog();

  /**
   * Reads the current configuration. Only the options which are not saved somewhere
   * else are read at this point.
   *
   * @param config A pointer to the KConfig object
   */
  void readConfiguration();
  /**
   * Saves the configuration. @ref KConfigBase::sync is called. This method is called
   * from the main Tellico object.
   *
   * @param config A pointer to the KConfig object
   */
  void saveConfiguration();

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
  void slotSelectedSourceChanged(QListViewItem* item);
  void slotMoveUpSourceClicked();
  void slotMoveDownSourceClicked();
  void slotNewStuffClicked();
  void slotShowTemplatePreview();
  void slotInstallTemplate();
  void slotDownloadTemplate();
  void slotDeleteTemplate();

private:
  /**
   * Sets-up the page for the general options.
   */
  void setupGeneralPage();
  void readGeneralConfig();
  /**
   * Sets-up the page for printing options.
   */
  void setupPrintingPage();
  void readPrintingConfig();
  /**
   * Sets-up the page for template options.
   */
  void setupTemplatePage();
  void readTemplateConfig();
  void setupFetchPage();
  /**
   * Load fetcher config
   */
  void readFetchConfig();

  SourceListViewItem* findItem(const QString& path) const;
  void loadTemplateList();

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

  GUI::ComboBox* m_templateCombo;
  KPushButton* m_previewButton;
  KFontCombo* m_fontCombo;
  KIntNumInput* m_fontSizeInput;
  KColorCombo* m_baseColorCombo;
  KColorCombo* m_textColorCombo;
  KColorCombo* m_highBaseColorCombo;
  KColorCombo* m_highTextColorCombo;

  KListView* m_sourceListView;
  QMap<SourceListViewItem*, Fetch::ConfigWidget*> m_configWidgets;
  QPtrList<Fetch::ConfigWidget> m_newStuffConfigWidgets;
  QPtrList<Fetch::ConfigWidget> m_removedConfigWidgets;
  KPushButton* m_modifySourceBtn;
  KPushButton* m_moveUpSourceBtn;
  KPushButton* m_moveDownSourceBtn;
  KPushButton* m_removeSourceBtn;
  KPushButton* m_newStuffBtn;
};

class GeneralFetcherInfo {
public:
  GeneralFetcherInfo(Fetch::Type t, const QString& n, bool o) : type(t), name(n), updateOverwrite(o) {}
  Fetch::Type type;
  QString name;
  bool updateOverwrite : 1;
};

class SourceListViewItem : public KListViewItem {
public:
  SourceListViewItem(KListView* parent, const GeneralFetcherInfo& info,
                     const QString& groupName = QString::null);

  SourceListViewItem(KListView* parent, QListViewItem* after,
                     const GeneralFetcherInfo& info, const QString& groupName = QString::null);

  void setConfigGroup(const QString& s) { m_configGroup = s; }
  const QString& configGroup() const { return m_configGroup; }
  const Fetch::Type& fetchType() const { return m_info.type; }
  void setUpdateOverwrite(bool b) { m_info.updateOverwrite = b; }
  bool updateOverwrite() const { return m_info.updateOverwrite; }
  void setNewSource(bool b) { m_newSource = b; }
  bool isNewSource() const { return m_newSource; }
  void setFetcher(Fetch::Fetcher::Ptr fetcher);
  Fetch::Fetcher::Ptr fetcher() const { return m_fetcher; }

private:
  GeneralFetcherInfo m_info;
  QString m_configGroup;
  bool m_newSource : 1;
  Fetch::Fetcher::Ptr m_fetcher;
};

} // end namespace
#endif
