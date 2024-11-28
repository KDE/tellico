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

#include <config.h>

#include "fetch/fetcherinfolistitem.h"

#include <KPageDialog>
#ifdef ENABLE_KNEWSTUFF3
#include <knewstuff_version.h>
#if KNEWSTUFF_VERSION < QT_VERSION_CHECK(5, 91, 0)
#include <KNS3/Button>
#else
#include <KNSWidgets/Button>
#endif
#endif

#include <QListWidget>

class KConfig;
class KIntNumInput;
class KColorCombo;
class KMessageWidget;

class QSpinBox;
class QPushButton;
class QLineEdit;
class QFontComboBox;
class QCheckBox;
class QRadioButton;
class QFrame;

namespace Tellico {
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

Q_SIGNALS:
  /**
   * Emitted whenever the Ok or Apply button is clicked.
   */
  void signalConfigChanged();

private Q_SLOTS:
  /**
   * Called when anything gets changed
   */
  void slotModified();
  /**
   * Called when the Ok button is clicked.
   */
  virtual void accept() Q_DECL_OVERRIDE;
  /**
   * Called when the Apply button is clicked.
   */
  void slotApply();
  /**
   * Called when the Default button is clicked.
   */
  void slotDefault();
  void slotHelp();
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
  void slotShowTemplatePreview();
  void slotInstallTemplate();
#ifdef ENABLE_KNEWSTUFF3
#if KNEWSTUFF_VERSION < QT_VERSION_CHECK(5, 91, 0)
  void slotUpdateTemplates(const QList<KNS3::Entry>& list);
#else
  void slotUpdateTemplates(const QList<KNSCore::Entry>& list);
#endif
#endif
  void slotDeleteTemplate();
  void slotCreateConfigWidgets();
  void slotUpdateImageLocationLabel();

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

  FetcherInfoListItem* findItem(const QString& path) const;
  void loadTemplateList();

  bool m_modifying;
  bool m_okClicked;

  QRadioButton* m_rbImageInFile;
  QRadioButton* m_rbImageInAppDir;
  QRadioButton* m_rbImageInLocalDir;
  KMessageWidget* m_mwImageLocation;
  QTimer* m_infoTimer;
  QCheckBox* m_cbOpenLastFile;
  QCheckBox* m_cbQuickFilterRegExp;
  QCheckBox* m_cbEnableWebcam;
  QCheckBox* m_cbCapitalize;
  QCheckBox* m_cbFormat;
  QLineEdit* m_leCapitals;
  QLineEdit* m_leArticles;
  QLineEdit* m_leSuffixes;
  QLineEdit* m_lePrefixes;

  QCheckBox* m_cbPrintHeaders;
  QCheckBox* m_cbPrintFormatted;
  QCheckBox* m_cbPrintGrouped;
  QSpinBox* m_imageWidthBox;
  QSpinBox* m_imageHeightBox;

  GUI::ComboBox* m_templateCombo;
  QPushButton* m_previewButton;
  QFontComboBox* m_fontCombo;
  QSpinBox* m_fontSizeInput;
  KColorCombo* m_baseColorCombo;
  KColorCombo* m_textColorCombo;
  KColorCombo* m_highBaseColorCombo;
  KColorCombo* m_highTextColorCombo;
  KColorCombo* m_linkColorCombo;

  QListWidget* m_sourceListWidget;
  QMap<FetcherInfoListItem*, Fetch::ConfigWidget*> m_configWidgets;
  QList<Fetch::ConfigWidget*> m_newStuffConfigWidgets;
  QList<Fetch::ConfigWidget*> m_removedConfigWidgets;
  QPushButton* m_modifySourceBtn;
  QPushButton* m_moveUpSourceBtn;
  QPushButton* m_moveDownSourceBtn;
  QPushButton* m_removeSourceBtn;
  QCheckBox* m_cbFilterSource;
  GUI::CollectionTypeCombo* m_sourceTypeCombo;
};

} // end namespace
#endif
