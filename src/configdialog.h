/****************************************************************************
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

#ifndef TELLICO_CONFIGDIALOG_H
#define TELLICO_CONFIGDIALOG_H

#include "fetch/fetcher.h"

#include <kpagedialog.h>
#include <klistwidget.h>

class KConfig;
class KLineEdit;
class KIntSpinBox;
class KPushButton;
class KIntNumInput;
class KColorCombo;

class QFontComboBox;
class QCheckBox;
class QRadioButton;
class QFrame;

namespace Tellico {
  class SourceListItem;
  namespace Fetch {
    class ConfigWidget;
  }
  namespace GUI {
    class ComboBox;
    class CollectionTypeCombo;
  }

/**
 * The configuration dialog class allows the user to change the global application
 * preferences.
 *
 * @author Robby Stephenson
 */
class ConfigDialog : public KPageDialog {
Q_OBJECT

public:
  /**
   * The constructor sets up the Tabbed dialog pages.
   *
   * @param parent A pointer to the parent widget
   */
  ConfigDialog(QWidget* parent);
  virtual ~ConfigDialog();

  /**
   * Saves the configuration
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
  void slotUpdateHelpLink(KPageWidgetItem* item);
  void slotInitPage(KPageWidgetItem* item);
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
  void slotSelectedSourceChanged(QListWidgetItem* item);
  void slotMoveUpSourceClicked();
  void slotMoveDownSourceClicked();
  void slotSourceFilterChanged();
  void slotNewStuffClicked();
  void slotShowTemplatePreview();
  void slotInstallTemplate();
  void slotDownloadTemplate();
  void slotDeleteTemplate();

private:
  enum Page {
    General  = 1 << 0,
    Printing = 1 << 1,
    Template = 1 << 2,
    Fetch    = 1 << 3
  };
  int m_initializedPages;
  bool isPageInitialized(Page page) const;

  /**
   * Sets-up the page for the general options.
   */
  void setupGeneralPage();
  void initGeneralPage(QFrame* frame);
  void readGeneralConfig();
  void saveGeneralConfig();
  /**
   * Sets-up the page for printing options.
   */
  void setupPrintingPage();
  void initPrintingPage(QFrame* frame);
  void readPrintingConfig();
  void savePrintingConfig();
  /**
   * Sets-up the page for template options.
   */
  void setupTemplatePage();
  void initTemplatePage(QFrame* frame);
  void readTemplateConfig();
  void saveTemplateConfig();
  /**
   */
  void setupFetchPage();
  void initFetchPage(QFrame* frame);
  void readFetchConfig();
  void saveFetchConfig();

  SourceListItem* findItem(const QString& path) const;
  void loadTemplateList();

  bool m_modifying;
  bool m_okClicked;

  QRadioButton* m_rbImageInFile;
  QRadioButton* m_rbImageInAppDir;
  QRadioButton* m_rbImageInLocalDir;
  QCheckBox* m_cbOpenLastFile;
  QCheckBox* m_cbShowTipDay;
  QCheckBox* m_cbEnableWebcam;
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
  QFontComboBox* m_fontCombo;
  KIntNumInput* m_fontSizeInput;
  KColorCombo* m_baseColorCombo;
  KColorCombo* m_textColorCombo;
  KColorCombo* m_highBaseColorCombo;
  KColorCombo* m_highTextColorCombo;

  KListWidget* m_sourceListWidget;
  QMap<SourceListItem*, Fetch::ConfigWidget*> m_configWidgets;
  QList<Fetch::ConfigWidget*> m_newStuffConfigWidgets;
  QList<Fetch::ConfigWidget*> m_removedConfigWidgets;
  KPushButton* m_modifySourceBtn;
  KPushButton* m_moveUpSourceBtn;
  KPushButton* m_moveDownSourceBtn;
  KPushButton* m_removeSourceBtn;
  KPushButton* m_newStuffBtn;
  QCheckBox* m_cbFilterSource;
  GUI::CollectionTypeCombo* m_sourceTypeCombo;
};

class GeneralFetcherInfo {
public:
  GeneralFetcherInfo(Fetch::Type t, const QString& n, bool o, QString u=QString()) : type(t), name(n), updateOverwrite(o), uuid(u) {}
  Fetch::Type type;
  QString name;
  bool updateOverwrite;
  QString uuid;
};

class SourceListItem : public QListWidgetItem {
public:
  explicit SourceListItem(const GeneralFetcherInfo& info,
                          const QString& groupName = QString());
  SourceListItem(KListWidget* parent, const GeneralFetcherInfo& info,
                 const QString& groupName = QString());

  void setConfigGroup(const QString& s) { m_configGroup = s; }
  const QString& configGroup() const { return m_configGroup; }
  const Fetch::Type& fetchType() const { return m_info.type; }
  void setUpdateOverwrite(bool b) { m_info.updateOverwrite = b; }
  bool updateOverwrite() const { return m_info.updateOverwrite; }
  void setNewSource(bool b) { m_newSource = b; }
  bool isNewSource() const { return m_newSource; }
  QString uuid() const { return m_info.uuid; }
  void setFetcher(Fetch::Fetcher::Ptr fetcher);
  Fetch::Fetcher::Ptr fetcher() const { return m_fetcher; }

private:
  GeneralFetcherInfo m_info;
  QString m_configGroup;
  bool m_newSource;
  Fetch::Fetcher::Ptr m_fetcher;
};

} // end namespace
#endif
