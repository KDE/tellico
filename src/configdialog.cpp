/***************************************************************************
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

#include <config.h>

#include "configdialog.h"
#include "field.h"
#include "collection.h"
#include "collectionfactory.h"
#include "fetch/execexternalfetcher.h"
#include "fetch/fetchmanager.h"
#include "fetch/configwidget.h"
#include "controller.h"
#include "fetcherconfigdialog.h"
#include "tellico_kernel.h"
#include "tellico_utils.h"
#include "core/tellico_config.h"
#include "images/imagefactory.h"
#include "gui/combobox.h"
#include "gui/previewdialog.h"
#include "newstuff/manager.h"
#include "fieldformat.h"
#include "tellico_debug.h"

#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <knuminput.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <kacceleratormanager.h>
#include <khtmlview.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kcolorcombo.h>
#include <kapplication.h>
#include <kvbox.h>
#include <khbox.h>
#include <KNS/Engine>

#include <QSize>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPixmap>
#include <QRegExp>
#include <QPushButton>
#include <QFileInfo>
#include <QRadioButton>
#include <QFrame>
#include <QFontComboBox>
#include <QGroupBox>
#include <QButtonGroup>

namespace {
  static const int CONFIG_MIN_WIDTH = 640;
  static const int CONFIG_MIN_HEIGHT = 420;
}

using Tellico::SourceListItem;
using Tellico::ConfigDialog;

SourceListItem::SourceListItem(const Tellico::GeneralFetcherInfo& info_, const QString& groupName_)
    : QListWidgetItem(), m_info(info_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()), m_fetcher(0) {
  setData(Qt::DisplayRole, info_.name);
  QPixmap pix = Fetch::Manager::fetcherIcon(info_.type);
  if(!pix.isNull()) {
    setData(Qt::DecorationRole, pix);
  }
}

SourceListItem::SourceListItem(KListWidget* parent_, const Tellico::GeneralFetcherInfo& info_, const QString& groupName_)
    : QListWidgetItem(parent_), m_info(info_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()), m_fetcher(0) {
  setData(Qt::DisplayRole, info_.name);
  QPixmap pix = Fetch::Manager::fetcherIcon(info_.type);
  if(!pix.isNull()) {
    setData(Qt::DecorationRole, pix);
  }
}

void SourceListItem::setFetcher(Tellico::Fetch::Fetcher::Ptr fetcher) {
  m_fetcher = fetcher;
  QPixmap pix = Fetch::Manager::fetcherIcon(fetcher);
  if(!pix.isNull()) {
    setData(Qt::DecorationRole, pix);
  }
}

ConfigDialog::ConfigDialog(QWidget* parent_)
    : KPageDialog(parent_)
    , m_initializedPages(0)
    , m_modifying(false)
    , m_okClicked(false) {
  setFaceType(List);
  setModal(true);
  setCaption(i18n("Configure Tellico"));
  setButtons(Help|Ok|Apply|Cancel|Default);

  setupGeneralPage();
  setupPrintingPage();
  setupTemplatePage();
  setupFetchPage();

  updateGeometry();
  QSize s = sizeHint();
  resize(qMax(s.width(), CONFIG_MIN_WIDTH), qMax(s.height(), CONFIG_MIN_HEIGHT));

  connect(this, SIGNAL(okClicked()), SLOT(slotOk()));
  connect(this, SIGNAL(applyClicked()), SLOT(slotApply()));
  connect(this, SIGNAL(defaultClicked()), SLOT(slotDefault()));

  enableButtonOk(false);
  enableButtonApply(false);

  setHelp(QLatin1String("general-options"));
  connect(this, SIGNAL(currentPageChanged(KPageWidgetItem*, KPageWidgetItem*)), SLOT(slotUpdateHelpLink(KPageWidgetItem*)));
  connect(this, SIGNAL(currentPageChanged(KPageWidgetItem*, KPageWidgetItem*)), SLOT(slotInitPage(KPageWidgetItem*)));
}

ConfigDialog::~ConfigDialog() {
  foreach(Fetch::ConfigWidget* widget, m_newStuffConfigWidgets) {
    widget->removed();
  }
}

void ConfigDialog::slotUpdateHelpLink(KPageWidgetItem* item_) {
  const QString name = item_->name();
  // thes ename must be kept in sync with the page names
  if(name == i18n("General")) {
    setHelp(QLatin1String("general-options"));
  } else if(name == i18n("Printing")) {
    setHelp(QLatin1String("printing-options"));
  } else if(name == i18n("Templates")) {
    setHelp(QLatin1String("template-options"));
  } else if(name == i18n("Data Sources")) {
    setHelp(QLatin1String("internet-sources-options"));
  }
}

void ConfigDialog::slotInitPage(KPageWidgetItem* item_) {
  Q_ASSERT(item_);
  // every page item has a frame
  // if the frame has no layout, then we need to initialize the itme
  QFrame* frame = ::qobject_cast<QFrame*>(item_->widget());
  Q_ASSERT(frame);
  if(frame->layout()) {
    return;
  }

  const QString name = item_->name();
  // thes ename must be kept in sync with the page names
  if(name == i18n("General")) {
    initGeneralPage(frame);
  } else if(name == i18n("Printing")) {
    initPrintingPage(frame);
  } else if(name == i18n("Templates")) {
    initTemplatePage(frame);
  } else if(name == i18n("Data Sources")) {
    initFetchPage(frame);
  }
}

void ConfigDialog::slotOk() {
  m_okClicked = true;
  slotApply();
  accept();
  m_okClicked = false;
}

void ConfigDialog::slotApply() {
  emit signalConfigChanged();
  enableButtonApply(false);
}

void ConfigDialog::slotDefault() {
  // only change the defaults on the active page
  Config::self()->useDefaults(true);
  QString name = currentPage()->name();
  if(name == i18n("General")) {
    readGeneralConfig();
  } else if(name == i18n("Printing")) {
    readPrintingConfig();
  } else if(name == i18n("Templates")) {
    readTemplateConfig();
  }
  Config::self()->useDefaults(false);
  slotModified();
}

bool ConfigDialog::isPageInitialized(Page page_) const {
  return m_initializedPages & page_;
}

void ConfigDialog::setupGeneralPage() {
  QPixmap pix = DesktopIcon(QLatin1String("tellico"), KIconLoader::SizeMedium);
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("General"));
  page->setHeader(i18n("General Options"));
  page->setIcon(KIcon(pix));
  addPage(page);

  // since this is the first page, go ahead and lay it out
  initGeneralPage(frame);
}

void ConfigDialog::initGeneralPage(QFrame* frame) {
  QVBoxLayout* l = new QVBoxLayout(frame);

  m_cbOpenLastFile = new QCheckBox(i18n("&Reopen file at startup"), frame);
  m_cbOpenLastFile->setWhatsThis(i18n("If checked, the file that was last open "
                                      "will be re-opened at program start-up."));
  l->addWidget(m_cbOpenLastFile);
  connect(m_cbOpenLastFile, SIGNAL(clicked()), SLOT(slotModified()));

  m_cbShowTipDay = new QCheckBox(i18n("&Show \"Tip of the Day\" at startup"), frame);
  m_cbShowTipDay->setWhatsThis(i18n("If checked, the \"Tip of the Day\" will be "
                                    "shown at program start-up."));
  l->addWidget(m_cbShowTipDay);
  connect(m_cbShowTipDay, SIGNAL(clicked()), SLOT(slotModified()));

  m_cbEnableWebcam = new QCheckBox(i18n("&Enable webcam for barcode scanning"), frame);
  m_cbEnableWebcam->setWhatsThis(i18n("If checked, the input from a webcam will be used "
                                      "to scan barcodes for searching."));
  l->addWidget(m_cbEnableWebcam);
  connect(m_cbEnableWebcam, SIGNAL(clicked()), SLOT(slotModified()));

  QGroupBox* imageGroupBox = new QGroupBox(i18n("Image Storage Options"), frame);
  l->addWidget(imageGroupBox);
  m_rbImageInFile = new QRadioButton(i18n("Store images in data file"), imageGroupBox);
  m_rbImageInAppDir = new QRadioButton(i18n("Store images in common application directory"), imageGroupBox);
  m_rbImageInLocalDir = new QRadioButton(i18n("Store images in directory relative to data file"), imageGroupBox);
  imageGroupBox->setWhatsThis(i18n("Images may be saved in the data file itself, which can "
                                   "cause Tellico to run slowly, stored in the Tellico "
                                   "application directory, or stored in a directory in the "
                                   "same location as the data file."));
  QVBoxLayout* imageGroupLayout = new QVBoxLayout(imageGroupBox);
  imageGroupLayout->addWidget(m_rbImageInFile);
  imageGroupLayout->addWidget(m_rbImageInAppDir);
  imageGroupLayout->addWidget(m_rbImageInLocalDir);
  imageGroupBox->setLayout(imageGroupLayout);

  QButtonGroup* imageGroup = new QButtonGroup(frame);
  imageGroup->addButton(m_rbImageInFile);
  imageGroup->addButton(m_rbImageInAppDir);
  imageGroup->addButton(m_rbImageInLocalDir);
  connect(imageGroup, SIGNAL(buttonClicked(int)), SLOT(slotModified()));

  QGroupBox* formatGroup = new QGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(formatGroup);
  QVBoxLayout* formatGroupLayout = new QVBoxLayout(formatGroup);
  formatGroup->setLayout(formatGroupLayout);

  m_cbCapitalize = new QCheckBox(i18n("Auto capitalize &titles and names"), formatGroup);
  m_cbCapitalize->setWhatsThis(i18n("If checked, titles and names will "
                                    "be automatically capitalized."));
  connect(m_cbCapitalize, SIGNAL(clicked()), SLOT(slotModified()));
  formatGroupLayout->addWidget(m_cbCapitalize);

  m_cbFormat = new QCheckBox(i18n("Auto &format titles and names"), formatGroup);
  m_cbFormat->setWhatsThis(i18n("If checked, titles and names will "
                                "be automatically formatted."));
  connect(m_cbFormat, SIGNAL(clicked()), SLOT(slotModified()));
  formatGroupLayout->addWidget(m_cbFormat);

  QWidget* g1 = new QWidget(formatGroup);
  QGridLayout* g1Layout = new QGridLayout(g1);
  g1->setLayout(g1Layout);
  formatGroupLayout->addWidget(g1);

  QLabel* lab = new QLabel(i18n("No capitali&zation:"), g1);
  g1Layout->addWidget(lab, 0, 0);
  m_leCapitals = new KLineEdit(g1);
  g1Layout->addWidget(m_leCapitals, 0, 1);
  lab->setBuddy(m_leCapitals);
  QString whats = i18n("<qt>A list of words which should not be capitalized. Multiple values "
                       "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_leCapitals->setWhatsThis(whats);
  connect(m_leCapitals, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("Artic&les:"), g1);
  g1Layout->addWidget(lab, 1, 0);
  m_leArticles = new KLineEdit(g1);
  g1Layout->addWidget(m_leArticles, 1, 1);
  lab->setBuddy(m_leArticles);
  whats = i18n("<qt>A list of words which should be considered as articles "
               "if they are the first word in a title. Multiple values "
               "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_leArticles->setWhatsThis(whats);
  connect(m_leArticles, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("Personal suffi&xes:"), g1);
  g1Layout->addWidget(lab, 2, 0);
  m_leSuffixes = new KLineEdit(g1);
  g1Layout->addWidget(m_leSuffixes, 2, 1);
  lab->setBuddy(m_leSuffixes);
  whats = i18n("<qt>A list of suffixes which might be used in personal names. Multiple values "
               "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_leSuffixes->setWhatsThis(whats);
  connect(m_leSuffixes, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("Surname &prefixes:"), g1);
  g1Layout->addWidget(lab, 3, 0);
  m_lePrefixes = new KLineEdit(g1);
  g1Layout->addWidget(m_lePrefixes, 3, 1);
  lab->setBuddy(m_lePrefixes);
  whats = i18n("<qt>A list of prefixes which might be used in surnames. Multiple values "
               "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_lePrefixes->setWhatsThis(whats);
  connect(m_lePrefixes, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  // stretch to fill lower area
  l->addStretch(1);
  m_initializedPages |= General;
  readGeneralConfig();
}

void ConfigDialog::setupPrintingPage() {
  QPixmap pix = DesktopIcon(QLatin1String("printer"), KIconLoader::SizeMedium);
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("Printing"));
  page->setHeader(i18n("Printing Options"));
  page->setIcon(KIcon(pix));
  addPage(page);
}

void ConfigDialog::initPrintingPage(QFrame* frame) {
  QVBoxLayout* l = new QVBoxLayout(frame);

  QGroupBox* formatOptions = new QGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(formatOptions);
  QVBoxLayout* formatLayout = new QVBoxLayout(formatOptions);
  formatOptions->setLayout(formatLayout);

  m_cbPrintFormatted = new QCheckBox(i18n("&Format titles and names"), formatOptions);
  m_cbPrintFormatted->setWhatsThis(i18n("If checked, titles and names will be automatically formatted."));
  connect(m_cbPrintFormatted, SIGNAL(clicked()), SLOT(slotModified()));
  formatLayout->addWidget(m_cbPrintFormatted);

  m_cbPrintHeaders = new QCheckBox(i18n("&Print field headers"), formatOptions);
  m_cbPrintHeaders->setWhatsThis(i18n("If checked, the field names will be printed as table headers."));
  connect(m_cbPrintHeaders, SIGNAL(clicked()), SLOT(slotModified()));
  formatLayout->addWidget(m_cbPrintHeaders);

  QGroupBox* groupOptions = new QGroupBox(i18n("Grouping Options"), frame);
  l->addWidget(groupOptions);
  QVBoxLayout* groupLayout = new QVBoxLayout(groupOptions);
  groupOptions->setLayout(groupLayout);

  m_cbPrintGrouped = new QCheckBox(i18n("&Group the entries"), groupOptions);
  m_cbPrintGrouped->setWhatsThis(i18n("If checked, the entries will be grouped by the selected field."));
  connect(m_cbPrintGrouped, SIGNAL(clicked()), SLOT(slotModified()));
  groupLayout->addWidget(m_cbPrintGrouped);

  QGroupBox* imageOptions = new QGroupBox(i18n("Image Options"), frame);
  l->addWidget(imageOptions);

  QGridLayout* gridLayout = new QGridLayout(imageOptions);
  imageOptions->setLayout(gridLayout);

  QLabel* lab = new QLabel(i18n("Maximum image &width:"), imageOptions);
  gridLayout->addWidget(lab, 0, 0);
  m_imageWidthBox = new KIntSpinBox(0, 999, 1, 50, imageOptions);
  gridLayout->addWidget(m_imageWidthBox, 0, 1);
  m_imageWidthBox->setSuffix(QLatin1String(" px"));
  lab->setBuddy(m_imageWidthBox);
  QString whats = i18n("The maximum width of the images in the printout. The aspect ratio is preserved.");
  lab->setWhatsThis(whats);
  m_imageWidthBox->setWhatsThis(whats);
  connect(m_imageWidthBox, SIGNAL(valueChanged(int)), SLOT(slotModified()));
  // QSpinBox doesn't emit valueChanged if you edit the value with
  // the lineEdit until you change the keyboard focus
//  connect(m_imageWidthBox->child("qt_spinbox_edit"), SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("&Maximum image height:"), imageOptions);
  gridLayout->addWidget(lab, 1, 0);
  m_imageHeightBox = new KIntSpinBox(0, 999, 1, 50, imageOptions);
  gridLayout->addWidget(m_imageHeightBox, 1, 1);
  m_imageHeightBox->setSuffix(QLatin1String(" px"));
  lab->setBuddy(m_imageHeightBox);
  whats = i18n("The maximum height of the images in the printout. The aspect ratio is preserved.");
  lab->setWhatsThis(whats);
  m_imageHeightBox->setWhatsThis(whats);
  connect(m_imageHeightBox, SIGNAL(valueChanged(int)), SLOT(slotModified()));
  // QSpinBox doesn't emit valueChanged if you edit the value with
  // the lineEdit until you change the keyboard focus
//  connect(m_imageHeightBox->child("qt_spinbox_edit"), SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  // stretch to fill lower area
  l->addStretch(1);
  m_initializedPages |= Printing;
  readPrintingConfig();
}

void ConfigDialog::setupTemplatePage() {
  // odd icon, I know, matches KMail, though...
  QPixmap pix = DesktopIcon(QLatin1String("preferences-desktop-theme"), KIconLoader::SizeMedium);
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("Templates"));
  page->setHeader(i18n("Template Options"));
  page->setIcon(KIcon(pix));
  addPage(page);
}

void ConfigDialog::initTemplatePage(QFrame* frame) {
  QVBoxLayout* l = new QVBoxLayout(frame);

  QGridLayout* gridLayout = new QGridLayout();
  l->addLayout(gridLayout);

  int row = -1;
  // so I can reuse an i18n string, a plain label can't have an '&'
  QString s = Tellico::removeAcceleratorMarker(i18n("Collection &type:"));
  QLabel* lab = new QLabel(s, frame);
  gridLayout->addWidget(lab, ++row, 0);
  const int collType = Kernel::self()->collectionType();
  lab = new QLabel(CollectionFactory::nameHash()[collType], frame);
  gridLayout->addWidget(lab, row, 1, 1, 2);

  lab = new QLabel(i18n("Template:"), frame);
  m_templateCombo = new GUI::ComboBox(frame);
  connect(m_templateCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_templateCombo);
  QString whats = i18n("Select the template to use for the current type of collections. "
                       "Not all templates will use the font and color settings.");
  lab->setWhatsThis(whats);
  m_templateCombo->setWhatsThis(whats);
  gridLayout->addWidget(lab, ++row, 0);
  gridLayout->addWidget(m_templateCombo, row, 1);

  KPushButton* btn = new KPushButton(i18n("&Preview..."), frame);
  btn->setWhatsThis(i18n("Show a preview of the template"));
  btn->setIcon(KIcon(QLatin1String("zoom-original")));
  gridLayout->addWidget(btn, row, 2);
  connect(btn, SIGNAL(clicked()), SLOT(slotShowTemplatePreview()));

  // so the button is squeezed small
  gridLayout->setColumnStretch(0, 10);
  gridLayout->setColumnStretch(1, 10);

  loadTemplateList();

//  QLabel* l1 = new QLabel(i18n("The options below will be passed to the template, but not "
//                               "all templates will use them. Some fonts and colors may be "
//                               "specified directly in the template."), frame);
//  l1->setTextFormat(Qt::RichText);
//  l->addWidget(l1);

  QGroupBox* fontGroup = new QGroupBox(i18n("Font Options"), frame);
  l->addWidget(fontGroup);

  row = -1;
  QGridLayout* fontLayout = new QGridLayout();
  fontGroup->setLayout(fontLayout);

  lab = new QLabel(i18n("Font:"), fontGroup);
  fontLayout->addWidget(lab, ++row, 0);
  m_fontCombo = new QFontComboBox(fontGroup);
  fontLayout->addWidget(m_fontCombo, row, 1);
  connect(m_fontCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_fontCombo);
  whats = i18n("This font is passed to the template used in the Entry View.");
  lab->setWhatsThis(whats);
  m_fontCombo->setWhatsThis(whats);

  fontLayout->addWidget(new QLabel(i18n("Size:"), fontGroup), ++row, 0);
  m_fontSizeInput = new KIntNumInput(fontGroup);
  m_fontSizeInput->setRange(5, 30); // 30 is same max as konq config
  m_fontSizeInput->setSuffix(QLatin1String("pt"));
  fontLayout->addWidget(m_fontSizeInput, row, 1);
  connect(m_fontSizeInput, SIGNAL(valueChanged(int)), SLOT(slotModified()));
  lab->setBuddy(m_fontSizeInput);
  lab->setWhatsThis(whats);
  m_fontSizeInput->setWhatsThis(whats);

  QGroupBox* colGroup = new QGroupBox(i18n("Color Options"), frame);
  l->addWidget(colGroup);

  row = -1;
  QGridLayout* colLayout = new QGridLayout();
  colGroup->setLayout(colLayout);

  lab = new QLabel(i18n("Background color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_baseColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_baseColorCombo, row, 1);
  connect(m_baseColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_baseColorCombo);
  whats = i18n("This color is passed to the template used in the Entry View.");
  lab->setWhatsThis(whats);
  m_baseColorCombo->setWhatsThis(whats);

  lab = new QLabel(i18n("Text color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_textColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_textColorCombo, row, 1);
  connect(m_textColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_textColorCombo);
  lab->setWhatsThis(whats);
  m_textColorCombo->setWhatsThis(whats);

  lab = new QLabel(i18n("Highlight color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_highBaseColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_highBaseColorCombo, row, 1);
  connect(m_highBaseColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_highBaseColorCombo);
  lab->setWhatsThis(whats);
  m_highBaseColorCombo->setWhatsThis(whats);

  lab = new QLabel(i18n("Highlighted text color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_highTextColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_highTextColorCombo, row, 1);
  connect(m_highTextColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_highTextColorCombo);
  lab->setWhatsThis(whats);
  m_highTextColorCombo->setWhatsThis(whats);

  QGroupBox* groupBox = new QGroupBox(i18n("Manage Templates"), frame);
  l->addWidget(groupBox);
  QVBoxLayout* vlay = new QVBoxLayout(groupBox);
  groupBox->setLayout(vlay);

  KHBox* box1 = new KHBox(groupBox);
  vlay->addWidget(box1);
  box1->setSpacing(spacingHint());

  KPushButton* b1 = new KPushButton(i18n("Install..."), box1);
  b1->setIcon(KIcon(QLatin1String("list-add")));
  connect(b1, SIGNAL(clicked()), SLOT(slotInstallTemplate()));
  whats = i18n("Click to install a new template directly.");
  b1->setWhatsThis(whats);

  KPushButton* b2 = new KPushButton(i18n("Download..."), box1);
  b2->setIcon(KIcon(QLatin1String("get-hot-new-stuff")));
  connect(b2, SIGNAL(clicked()), SLOT(slotDownloadTemplate()));
  whats = i18n("Click to download additional templates.");
  b2->setWhatsThis(whats);

  KPushButton* b3 = new KPushButton(i18n("Delete..."), box1);
  b3->setIcon(KIcon(QLatin1String("list-remove")));
  connect(b3, SIGNAL(clicked()), SLOT(slotDeleteTemplate()));
  whats = i18n("Click to select and remove installed templates.");
  b3->setWhatsThis(whats);

  // stretch to fill lower area
  l->addStretch(1);

  // purely for asthetics make all widgets line up
  QList<QWidget*> widgets;
  widgets.append(m_fontCombo);
  widgets.append(m_fontSizeInput);
  widgets.append(m_baseColorCombo);
  widgets.append(m_textColorCombo);
  widgets.append(m_highBaseColorCombo);
  widgets.append(m_highTextColorCombo);
  int w = 0;
  foreach(QWidget* widget, widgets) {
    widget->ensurePolished();
    w = qMax(w, widget->sizeHint().width());
  }
  foreach(QWidget* widget, widgets) {
    widget->setMinimumWidth(w);
  }

  KAcceleratorManager::manage(frame);
  m_initializedPages |= Template;
  readTemplateConfig();
}

void ConfigDialog::setupFetchPage() {
  QPixmap pix = DesktopIcon(QLatin1String("network-wired"), KIconLoader::SizeMedium);
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("Data Sources"));
  page->setHeader(i18n("Data Sources Options"));
  page->setIcon(KIcon(pix));
  addPage(page);
}

void ConfigDialog::initFetchPage(QFrame* frame) {
  QHBoxLayout* l = new QHBoxLayout(frame);

  QVBoxLayout* leftLayout = new QVBoxLayout();
  l->addLayout(leftLayout);
  m_sourceListWidget = new KListWidget(frame);
  m_sourceListWidget->setSortingEnabled(false); // no sorting
  m_sourceListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  leftLayout->addWidget(m_sourceListWidget, 1);
  connect(m_sourceListWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(slotSelectedSourceChanged(QListWidgetItem*)));
  connect(m_sourceListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(slotModifySourceClicked()));

  KHBox* hb = new KHBox(frame);
  leftLayout->addWidget(hb);
  m_moveUpSourceBtn = new KPushButton(i18n("Move &Up"), hb);
  m_moveUpSourceBtn->setIcon(KIcon(QLatin1String("go-up")));
  m_moveUpSourceBtn->setWhatsThis(i18n("The order of the data sources sets the order "
                                       "that Tellico uses when entries are automatically updated."));
  m_moveDownSourceBtn = new KPushButton(i18n("Move &Down"), hb);
  m_moveDownSourceBtn->setIcon(KIcon(QLatin1String("go-down")));
  m_moveDownSourceBtn->setWhatsThis(i18n("The order of the data sources sets the order "
                                         "that Tellico uses when entries are automatically updated."));

  // these icons are rather arbitrary, but seem to vaguely fit
  QVBoxLayout* vlay = new QVBoxLayout();
  l->addLayout(vlay);
  KPushButton* newSourceBtn = new KPushButton(i18n("&New..."), frame);
  newSourceBtn->setIcon(KIcon(QLatin1String("document-new")));
  newSourceBtn->setWhatsThis(i18n("Click to add a new data source."));
  m_modifySourceBtn = new KPushButton(i18n("&Modify..."), frame);
  m_modifySourceBtn->setIcon(KIcon(QLatin1String("network-wired")));
  m_modifySourceBtn->setWhatsThis(i18n("Click to modify the selected data source."));
  m_removeSourceBtn = new KPushButton(i18n("&Delete"), frame);
  m_removeSourceBtn->setIcon(KIcon(QLatin1String("list-remove")));
  m_removeSourceBtn->setWhatsThis(i18n("Click to delete the selected data source."));
  m_newStuffBtn = new KPushButton(i18n("Download..."), frame);
  m_newStuffBtn->setIcon(KIcon(QLatin1String("get-hot-new-stuff")));
  m_newStuffBtn->setWhatsThis(i18n("Click to download additional data sources."));
  // checksum and signature checking are no longer possible with knewstuff2
  // disable button for now
  m_newStuffBtn->setEnabled(false);

  vlay->addWidget(newSourceBtn);
  vlay->addWidget(m_modifySourceBtn);
  vlay->addWidget(m_removeSourceBtn);
  // separate newstuff button from the rest
  vlay->addSpacing(2 * KDialog::spacingHint());
  vlay->addWidget(m_newStuffBtn);
  vlay->addStretch(1);

  connect(newSourceBtn, SIGNAL(clicked()), SLOT(slotNewSourceClicked()));
  connect(m_modifySourceBtn, SIGNAL(clicked()), SLOT(slotModifySourceClicked()));
  connect(m_moveUpSourceBtn, SIGNAL(clicked()), SLOT(slotMoveUpSourceClicked()));
  connect(m_moveDownSourceBtn, SIGNAL(clicked()), SLOT(slotMoveDownSourceClicked()));
  connect(m_removeSourceBtn, SIGNAL(clicked()), SLOT(slotRemoveSourceClicked()));
  connect(m_newStuffBtn, SIGNAL(clicked()), SLOT(slotNewStuffClicked()));

  KAcceleratorManager::manage(frame);
  m_initializedPages |= Fetch;
  readFetchConfig();
}

void ConfigDialog::readGeneralConfig() {
  m_modifying = true;

  m_cbShowTipDay->setChecked(Config::showTipOfDay());
  m_cbOpenLastFile->setChecked(Config::reopenLastFile());
#ifdef ENABLE_WEBCAM
  m_cbEnableWebcam->setChecked(Config::enableWebcam());
#else
  m_cbEnableWebcam->setChecked(false);
  m_cbEnableWebcam->setEnabled(false);
#endif

  switch(Config::imageLocation()) {
    case Config::ImagesInFile: m_rbImageInFile->setChecked(true); break;
    case Config::ImagesInAppDir: m_rbImageInAppDir->setChecked(true); break;
    case Config::ImagesInLocalDir: m_rbImageInLocalDir->setChecked(true); break;
  }

  bool autoCapitals = Config::autoCapitalization();
  m_cbCapitalize->setChecked(autoCapitals);

  bool autoFormat = Config::autoFormat();
  m_cbFormat->setChecked(autoFormat);

  const QRegExp comma(QLatin1String("\\s*,\\s*"));

  m_leCapitals->setText(Config::noCapitalizationString().replace(comma, FieldFormat::delimiterString()));
  m_leArticles->setText(Config::articlesString().replace(comma, FieldFormat::delimiterString()));
  m_leSuffixes->setText(Config::nameSuffixesString().replace(comma, FieldFormat::delimiterString()));
  m_lePrefixes->setText(Config::surnamePrefixesString().replace(comma, FieldFormat::delimiterString()));

  m_modifying = false;
}

void ConfigDialog::readPrintingConfig() {
  m_modifying = true;

  m_cbPrintHeaders->setChecked(Config::printFieldHeaders());
  m_cbPrintFormatted->setChecked(Config::printFormatted());
  m_cbPrintGrouped->setChecked(Config::printGrouped());
  m_imageWidthBox->setValue(Config::maxImageWidth());
  m_imageHeightBox->setValue(Config::maxImageHeight());

  m_modifying = false;
}

void ConfigDialog::readTemplateConfig() {
  m_modifying = true;

  // entry template selection
  const int collType = Kernel::self()->collectionType();
  QString file = Config::templateName(collType);
  file.replace(QLatin1Char('_'), QLatin1Char(' '));
  QString fileContext = file + QLatin1String(" XSL Template");
  m_templateCombo->setCurrentItem(i18nc(fileContext.toUtf8(), file.toUtf8()));

  m_fontCombo->setCurrentFont(QFont(Config::templateFont(collType).family()));
  m_fontSizeInput->setValue(Config::templateFont(collType).pointSize());
  m_baseColorCombo->setColor(Config::templateBaseColor(collType));
  m_textColorCombo->setColor(Config::templateTextColor(collType));
  m_highBaseColorCombo->setColor(Config::templateHighlightedBaseColor(collType));
  m_highTextColorCombo->setColor(Config::templateHighlightedTextColor(collType));

  m_modifying = false;
}

void ConfigDialog::readFetchConfig() {
  m_modifying = true;

  m_sourceListWidget->clear();
  m_configWidgets.clear();

  m_sourceListWidget->setUpdatesEnabled(false);
  Fetch::FetcherVec fetchers = Fetch::Manager::self()->fetchers();
  foreach(Fetch::Fetcher::Ptr fetcher, fetchers) {
    GeneralFetcherInfo info(fetcher->type(), fetcher->source(),
                            fetcher->updateOverwrite(), fetcher->uuid());
    SourceListItem* item = new SourceListItem(m_sourceListWidget, info);
    item->setFetcher(fetcher);
    // grab the config widget, taking ownership
    Fetch::ConfigWidget* cw = fetcher->configWidget(this);
    if(cw) { // might return 0 when no widget available for fetcher type
      m_configWidgets.insert(item, cw);
      // there's weird layout bug if it's not hidden
      cw->hide();
    }
    kapp->processEvents();
  }
  m_sourceListWidget->setUpdatesEnabled(true);

  if(m_sourceListWidget->count() == 0) {
    m_modifySourceBtn->setEnabled(false);
    m_removeSourceBtn->setEnabled(false);
  } else {
    // go ahead and select the first one
    m_sourceListWidget->setCurrentItem(m_sourceListWidget->item(0));
  }

  m_modifying = false;
}

void ConfigDialog::saveConfiguration() {
  if(isPageInitialized(General)) saveGeneralConfig();
  if(isPageInitialized(Printing)) savePrintingConfig();
  if(isPageInitialized(Template)) saveTemplateConfig();
  if(isPageInitialized(Fetch)) saveFetchConfig();
}

void ConfigDialog::saveGeneralConfig() {
  Config::setShowTipOfDay(m_cbShowTipDay->isChecked());
  Config::setEnableWebcam(m_cbEnableWebcam->isChecked());

  int imageLocation;
  if(m_rbImageInFile->isChecked()) {
    imageLocation = Config::ImagesInFile;
  } else if(m_rbImageInAppDir->isChecked()) {
    imageLocation = Config::ImagesInAppDir;
  } else {
    imageLocation = Config::ImagesInLocalDir;
  }
  Config::setImageLocation(imageLocation);
  Config::setReopenLastFile(m_cbOpenLastFile->isChecked());

  Config::setAutoCapitalization(m_cbCapitalize->isChecked());
  Config::setAutoFormat(m_cbFormat->isChecked());

  const QRegExp semicolon(QLatin1String("\\s*;\\s*"));
  const QChar comma = QLatin1Char(',');

  Config::setNoCapitalizationString(m_leCapitals->text().replace(semicolon, comma));
  Config::setArticlesString(m_leArticles->text().replace(semicolon, comma));
  Config::setNameSuffixesString(m_leSuffixes->text().replace(semicolon, comma));
  Config::setSurnamePrefixesString(m_lePrefixes->text().replace(semicolon, comma));
}

void ConfigDialog::savePrintingConfig() {
  Config::setPrintFieldHeaders(m_cbPrintHeaders->isChecked());
  Config::setPrintFormatted(m_cbPrintFormatted->isChecked());
  Config::setPrintGrouped(m_cbPrintGrouped->isChecked());
  Config::setMaxImageWidth(m_imageWidthBox->value());
  Config::setMaxImageHeight(m_imageHeightBox->value());
}

void ConfigDialog::saveTemplateConfig() {
  const int collType = Kernel::self()->collectionType();
  Config::setTemplateName(collType, m_templateCombo->currentData().toString());
  QFont font(m_fontCombo->currentFont().family(), m_fontSizeInput->value());
  Config::setTemplateFont(collType, font);
  Config::setTemplateBaseColor(collType, m_baseColorCombo->color());
  Config::setTemplateTextColor(collType, m_textColorCombo->color());
  Config::setTemplateHighlightedBaseColor(collType, m_highBaseColorCombo->color());
  Config::setTemplateHighlightedTextColor(collType, m_highTextColorCombo->color());
}

void ConfigDialog::saveFetchConfig() {
  // first, tell config widgets they got deleted
  foreach(Fetch::ConfigWidget* widget, m_removedConfigWidgets) {
    widget->removed();
  }
  m_removedConfigWidgets.clear();

  bool reloadFetchers = false;
  int count = 0; // start group numbering at 0
  for( ; count < m_sourceListWidget->count(); ++count) {
    SourceListItem* item = static_cast<SourceListItem*>(m_sourceListWidget->item(count));
    Fetch::ConfigWidget* cw = m_configWidgets[item];
    if(!cw || (!cw->shouldSave() && !item->isNewSource())) {
      continue;
    }
    m_newStuffConfigWidgets.removeAll(cw);
    QString group = QString::fromLatin1("Data Source %1").arg(count);
    // in case we later change the order, clear the group now
    KGlobal::config()->deleteGroup(group);
    KConfigGroup configGroup(KGlobal::config(), group);
    configGroup.writeEntry("Name", item->data(Qt::DisplayRole).toString());
    configGroup.writeEntry("Type", int(item->fetchType()));
    configGroup.writeEntry("UpdateOverwrite", item->updateOverwrite());
    configGroup.writeEntry("Uuid", item->uuid());
    cw->saveConfig(configGroup);
    item->setNewSource(false);
    // in case the ordering changed
    item->setConfigGroup(group);
    reloadFetchers = true;
  }
  // now update total number of sources
  KConfigGroup sourceGroup(KGlobal::config(), "Data Sources");
  sourceGroup.writeEntry("Sources Count", count);
  // and purge old config groups
  QString group = QString::fromLatin1("Data Source %1").arg(count);
  while(KGlobal::config()->hasGroup(group)) {
    KGlobal::config()->deleteGroup(group);
    ++count;
    group = QString::fromLatin1("Data Source %1").arg(count);
  }

  Config::self()->writeConfig();

  if(reloadFetchers) {
    Fetch::Manager::self()->loadFetchers();
    Controller::self()->updatedFetchers();
    // reload fetcher items if OK was not clicked
    // meaning apply was clicked
    if(!m_okClicked) {
      QString currentSource;
      if(m_sourceListWidget->currentItem()) {
        currentSource = m_sourceListWidget->currentItem()->data(Qt::DisplayRole).toString();
      }
      readFetchConfig();
      if(!currentSource.isEmpty()) {
        QList<QListWidgetItem*> items = m_sourceListWidget->findItems(currentSource, Qt::MatchExactly);
        if(!items.isEmpty()) {
          m_sourceListWidget->setCurrentItem(items.first());
          m_sourceListWidget->scrollToItem(items.first());
        }
      }
    }
  }
}

void ConfigDialog::slotModified() {
  if(m_modifying) {
    return;
  }
  enableButtonOk(true);
  enableButtonApply(true);
}

void ConfigDialog::slotNewSourceClicked() {
  FetcherConfigDialog dlg(this);
  if(dlg.exec() != QDialog::Accepted) {
    return;
  }

  Fetch::Type type = dlg.sourceType();
  if(type == Fetch::Unknown) {
    return;
  }

  GeneralFetcherInfo info(type, dlg.sourceName(), dlg.updateOverwrite());
  SourceListItem* item = new SourceListItem(m_sourceListWidget, info);
  m_sourceListWidget->scrollToItem(item);
  m_sourceListWidget->setCurrentItem(item);
  Fetch::ConfigWidget* cw = dlg.configWidget();
  if(cw) {
    cw->setAccepted(true);
    cw->slotSetModified();
    cw->setParent(this); // keep the config widget around
    m_configWidgets.insert(item, cw);
  }
  m_modifySourceBtn->setEnabled(true);
  m_removeSourceBtn->setEnabled(true);
  slotModified(); // toggle apply button
}

void ConfigDialog::slotModifySourceClicked() {
  SourceListItem* item = static_cast<SourceListItem*>(m_sourceListWidget->currentItem());
  if(!item) {
    return;
  }

  Fetch::ConfigWidget* cw = 0;
  if(m_configWidgets.contains(item)) {
    cw = m_configWidgets[item];
  }
  if(!cw) {
    // no config widget for this one
    // might be because support was compiled out
    myDebug() << "no config widget for source" << item->data(Qt::DisplayRole);
    return;
  }
  FetcherConfigDialog dlg(item->data(Qt::DisplayRole).toString(), item->fetchType(), item->updateOverwrite(), cw, this);

  if(dlg.exec() == QDialog::Accepted) {
    cw->setAccepted(true); // mark to save
    QString newName = dlg.sourceName();
    if(newName != item->data(Qt::DisplayRole).toString()) {
      item->setData(Qt::DisplayRole, newName);
      cw->slotSetModified();
    }
    item->setUpdateOverwrite(dlg.updateOverwrite());
    slotModified(); // toggle apply button
  }
  cw->setParent(this); // keep the config widget around
}

void ConfigDialog::slotRemoveSourceClicked() {
  SourceListItem* item = static_cast<SourceListItem*>(m_sourceListWidget->currentItem());
  if(!item) {
    return;
  }

  Tellico::NewStuff::Manager::self()->removeScriptByName(item->text());

  Fetch::ConfigWidget* cw = m_configWidgets[item];
  if(cw) {
    m_removedConfigWidgets.append(cw);
    // it gets deleted by the parent
  }
  m_configWidgets.remove(item);
  delete item;
//  m_sourceListWidget->setCurrentItem(m_sourceListWidget->currentItem());
  slotModified(); // toggle apply button
}

void ConfigDialog::slotMoveUpSourceClicked() {
  int row = m_sourceListWidget->currentRow();
  if(row < 1) {
    return;
  }
  QListWidgetItem* item = m_sourceListWidget->takeItem(row);
  m_sourceListWidget->insertItem(row-1, item);
  m_sourceListWidget->setCurrentItem(item);
  slotModified(); // toggle apply button
}

void ConfigDialog::slotMoveDownSourceClicked() {
  int row = m_sourceListWidget->currentRow();
  if(row > m_sourceListWidget->count()-2) {
    return;
  }
  QListWidgetItem* item = m_sourceListWidget->takeItem(row);
  m_sourceListWidget->insertItem(row+1, item);
  m_sourceListWidget->setCurrentItem(item);
  slotModified(); // toggle apply button
}

void ConfigDialog::slotSelectedSourceChanged(QListWidgetItem* item_) {
  int row = m_sourceListWidget->row(item_);
  m_moveUpSourceBtn->setEnabled(row > 0);
  m_moveDownSourceBtn->setEnabled(row < m_sourceListWidget->count()-1);
}

void ConfigDialog::slotNewStuffClicked() {
  KNS::Engine engine(this);
  if(engine.init(QLatin1String("tellico-script.knsrc"))) {
    KNS::Entry::List entries = engine.downloadDialogModal(this);
    if(!entries.isEmpty()) {
      Fetch::Manager::self()->loadFetchers();
      readFetchConfig();
    }
  }
}

Tellico::SourceListItem* ConfigDialog::findItem(const QString& path_) const {
  if(path_.isEmpty()) {
    myDebug() << "empty path";
    return 0;
  }

  // this is a bit ugly, loop over all items, find the execexternal one
  // that matches the path
  for(int i = 0; i < m_sourceListWidget->count(); ++i) {
    SourceListItem* item = static_cast<SourceListItem*>(m_sourceListWidget->item(i));
    if(item->fetchType() != Fetch::ExecExternal) {
      continue;
    }
    Fetch::ExecExternalFetcher* f = dynamic_cast<Fetch::ExecExternalFetcher*>(item->fetcher().data());
    if(f && f->execPath() == path_) {
      return item;
    }
  }
  myDebug() << "no matching item found";
  return 0;
}

void ConfigDialog::slotShowTemplatePreview() {
  GUI::PreviewDialog* dlg = new GUI::PreviewDialog(this);

  const QString templateName = m_templateCombo->currentData().toString();
  dlg->setXSLTFile(templateName + QLatin1String(".xsl"));

  StyleOptions options;
  options.fontFamily = m_fontCombo->currentFont().family();
  options.fontSize   = m_fontSizeInput->value();
  options.baseColor  = m_baseColorCombo->color();
  options.textColor  = m_textColorCombo->color();
  options.highlightedTextColor = m_highTextColorCombo->color();
  options.highlightedBaseColor = m_highBaseColorCombo->color();
  dlg->setXSLTOptions(Kernel::self()->collectionType(), options);

  Data::CollPtr c = CollectionFactory::collection(Kernel::self()->collectionType(), true);
  Data::EntryPtr e(new Data::Entry(c));
  foreach(Data::FieldPtr f, c->fields()) {
    if(f->name() == QLatin1String("title")) {
      e->setField(f->name(), m_templateCombo->currentText());
    } else if(f->type() == Data::Field::Image) {
      continue;
    } else if(f->type() == Data::Field::Choice) {
      e->setField(f->name(), f->allowed().front());
    } else if(f->type() == Data::Field::Number) {
      e->setField(f->name(), QLatin1String("1"));
    } else if(f->type() == Data::Field::Bool) {
      e->setField(f->name(), QLatin1String("true"));
    } else if(f->type() == Data::Field::Rating) {
      e->setField(f->name(), QLatin1String("5"));
    } else {
      e->setField(f->name(), f->title());
    }
  }

  dlg->showEntry(e);
  dlg->show();
  // dlg gets deleted by itself
  // the finished() signal is connected in its constructor to delayedDestruct
}

void ConfigDialog::loadTemplateList() {
  QStringList files = KGlobal::dirs()->findAllResources("appdata", QLatin1String("entry-templates/*.xsl"),
                                                        KStandardDirs::NoDuplicates);
  QMap<QString, QString> templates; // a QMap will have them values sorted by key
  foreach(const QString& file, files) {
    QFileInfo fi(file);
    QString lfile = fi.fileName().section(QLatin1Char('.'), 0, -2);
    QString name = lfile;
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    QString title = i18nc((name + QLatin1String(" XSL Template")).toUtf8(), name.toUtf8());
    templates.insert(title, lfile);
  }

  QString s = m_templateCombo->currentText();
  m_templateCombo->clear();
  for(QMap<QString, QString>::ConstIterator it2 = templates.constBegin(); it2 != templates.constEnd(); ++it2) {
    m_templateCombo->addItem(it2.key(), it2.value());
  }
  m_templateCombo->setCurrentItem(s);
}

void ConfigDialog::slotInstallTemplate() {
  QString filter = i18n("*.xsl|XSL Files (*.xsl)") + QLatin1Char('\n');
  filter += i18n("*.tar.gz *.tgz|Template Packages (*.tar.gz)") + QLatin1Char('\n');
  filter += i18n("*|All Files");

  QString f = KFileDialog::getOpenFileName(KUrl(), filter, this);
  if(f.isEmpty()) {
    return;
  }

  if(Tellico::NewStuff::Manager::self()->installTemplate(f)) {
    loadTemplateList();
  }
}

void ConfigDialog::slotDownloadTemplate() {
  KNS::Engine engine(this);
  if(engine.init(QLatin1String("tellico-template.knsrc"))) {
    KNS::Entry::List entries = engine.downloadDialogModal(this);
    if(!entries.isEmpty()) {
      loadTemplateList();
    }
  }
}

void ConfigDialog::slotDeleteTemplate() {
  bool ok;
  QString name = KInputDialog::getItem(i18n("Delete Template"),
                                       i18n("Select template to delete:"),
                                       Tellico::NewStuff::Manager::self()->userTemplates().keys(),
                                       0, false, &ok, this);
  if(ok && !name.isEmpty()) {
    Tellico::NewStuff::Manager::self()->removeTemplateByName(name);
    loadTemplateList();
  }
}

#include "configdialog.moc"
