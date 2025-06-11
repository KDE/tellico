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
#include "utils/tellico_utils.h"
#include "utils/string_utils.h"
#include "config/tellico_config.h"
#include "core/tellico_strings.h"
#include "images/imagefactory.h"
#include "gui/combobox.h"
#include "gui/collectiontypecombo.h"
#include "gui/previewdialog.h"
#include "newstuff/manager.h"
#include "fieldformat.h"
#include "tellico_debug.h"

#include <KLocalizedString>
#include <KConfig>
#include <KAcceleratorManager>
#include <KColorCombo>
#include <KHelpClient>
#include <KRecentDirs>
#include <KMessageWidget>

#ifdef ENABLE_KNEWSTUFF3
#if KNEWSTUFF_VERSION < QT_VERSION_CHECK(5, 91, 0)
#include <KNS3/Button>
#else
#include <KNSWidgets/Button>
#endif
#endif

#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSize>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPixmap>
#include <QRegularExpression>
#include <QFileInfo>
#include <QRadioButton>
#include <QFrame>
#include <QFontComboBox>
#include <QGroupBox>
#include <QButtonGroup>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QTimer>
#include <QFileDialog>
#include <QSignalBlocker>

namespace {
  static const int CONFIG_MIN_WIDTH = 640;
  static const int CONFIG_MIN_HEIGHT = 420;
}

using Tellico::ConfigDialog;

ConfigDialog::ConfigDialog(QWidget* parent_)
    : KPageDialog(parent_)
    , m_initializedPages(0)
    , m_modifying(false)
    , m_okClicked(false) {
  setFaceType(List);
  setModal(true);
  setWindowTitle(i18n("Configure Tellico"));
  setStandardButtons(QDialogButtonBox::Help |
                     QDialogButtonBox::Ok |
                     QDialogButtonBox::Apply |
                     QDialogButtonBox::Cancel |
                     QDialogButtonBox::RestoreDefaults);

  setupGeneralPage();
  setupPrintingPage();
  setupTemplatePage();
  setupFetchPage();

  updateGeometry();
  QSize s = sizeHint();
  resize(qMax(s.width(), CONFIG_MIN_WIDTH), qMax(s.height(), CONFIG_MIN_HEIGHT));

  // OK button is connected to buttonBox accepted() signal which is already connected to accept() slot
  connect(button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &ConfigDialog::slotApply);
  connect(button(QDialogButtonBox::Help), &QAbstractButton::clicked, this, &ConfigDialog::slotHelp);
  connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, this, &ConfigDialog::slotDefault);

  button(QDialogButtonBox::Ok)->setEnabled(false);
  button(QDialogButtonBox::Apply)->setEnabled(false);
  button(QDialogButtonBox::Ok)->setDefault(true);
  button(QDialogButtonBox::Ok)->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));

  connect(this, &KPageDialog::currentPageChanged, this, &ConfigDialog::slotInitPage);
}

ConfigDialog::~ConfigDialog() {
  foreach(Fetch::ConfigWidget* widget, m_newStuffConfigWidgets) {
    widget->removed();
  }
}

void ConfigDialog::slotInitPage(KPageWidgetItem* item_) {
  Q_ASSERT(item_);
  // every page item has a frame
  // if the frame has no layout, then we need to initialize the item
  QFrame* frame = ::qobject_cast<QFrame*>(item_->widget());
  Q_ASSERT(frame);
  if(frame->layout()) {
    return;
  }

  const QString name = item_->name();
  // these names must be kept in sync with the page names
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

void ConfigDialog::accept() {
  m_okClicked = true;
  slotApply();
  KPageDialog::accept();
  m_okClicked = false;
}

void ConfigDialog::slotApply() {
  emit signalConfigChanged();
  button(QDialogButtonBox::Apply)->setEnabled(false);
}

void ConfigDialog::slotDefault() {
  // only change the defaults on the active page
  Config::self()->useDefaults(true);
  const QString name = currentPage()->name();
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

void ConfigDialog::slotHelp() {
  const QString name = currentPage()->name();
  // these names must be kept in sync with the page names
  if(name == i18n("General")) {
    KHelpClient::invokeHelp(QStringLiteral("configuration.html#general-options"));
  } else if(name == i18n("Printing")) {
    KHelpClient::invokeHelp(QStringLiteral("configuration.html#printing-options"));
  } else if(name == i18n("Templates")) {
    KHelpClient::invokeHelp(QStringLiteral("configuration.html#template-options"));
  } else if(name == i18n("Data Sources")) {
    KHelpClient::invokeHelp(QStringLiteral("configuration.html#data-sources-options"));
  }
}

bool ConfigDialog::isPageInitialized(Page page_) const {
  return m_initializedPages & page_;
}

void ConfigDialog::setupGeneralPage() {
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("General"));
  page->setHeader(i18n("General Options"));
  page->setIcon(QIcon::fromTheme(QStringLiteral("tellico"), QIcon(QLatin1String(":/icons/tellico"))));
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
  connect(m_cbOpenLastFile, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);

  m_cbQuickFilterRegExp = new QCheckBox(i18n("&Enable regular expressions in quick filter"), frame);
  m_cbQuickFilterRegExp->setWhatsThis(i18n("If checked, the quick filter will "
                                           "interpret text as a regular expression."));
  l->addWidget(m_cbQuickFilterRegExp);
  connect(m_cbQuickFilterRegExp, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);

  m_cbEnableWebcam = new QCheckBox(i18n("&Enable webcam for barcode scanning"), frame);
  m_cbEnableWebcam->setWhatsThis(i18n("If checked, the input from a webcam will be used "
                                      "to scan barcodes for searching."));
  l->addWidget(m_cbEnableWebcam);
  connect(m_cbEnableWebcam, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);

  QGroupBox* imageGroupBox = new QGroupBox(i18n("Image Storage Options"), frame);
  l->addWidget(imageGroupBox);
  m_rbImageInFile = new QRadioButton(i18n("Store images in data file"), imageGroupBox);
  m_rbImageInAppDir = new QRadioButton(i18n("Store images in common application directory"), imageGroupBox);
  m_rbImageInLocalDir = new QRadioButton(i18n("Store images in directory relative to data file"), imageGroupBox);
  imageGroupBox->setWhatsThis(i18n("Images may be saved in the data file itself, which can "
                                   "cause Tellico to run slowly, stored in the Tellico "
                                   "application directory, or stored in a directory in the "
                                   "same location as the data file."));
  connect(m_rbImageInFile, &QRadioButton::toggled, this, &ConfigDialog::slotUpdateImageLocationLabel);
  connect(m_rbImageInAppDir, &QRadioButton::toggled, this, &ConfigDialog::slotUpdateImageLocationLabel);
  connect(m_rbImageInLocalDir, &QRadioButton::toggled, this, &ConfigDialog::slotUpdateImageLocationLabel);
  m_mwImageLocation = new KMessageWidget(imageGroupBox);
  m_mwImageLocation->setMessageType(KMessageWidget::Information);
  m_mwImageLocation->hide();
  m_mwImageLocation->setWordWrap(true);
  m_infoTimer = new QTimer(imageGroupBox);
  m_infoTimer->setInterval(5000);
  m_infoTimer->callOnTimeout(m_mwImageLocation, &KMessageWidget::animatedHide);
  QVBoxLayout* imageGroupLayout = new QVBoxLayout(imageGroupBox);
  imageGroupLayout->addWidget(m_rbImageInFile);
  imageGroupLayout->addWidget(m_rbImageInAppDir);
  imageGroupLayout->addWidget(m_rbImageInLocalDir);
  imageGroupLayout->addWidget(m_mwImageLocation);
  imageGroupBox->setLayout(imageGroupLayout);

  QButtonGroup* imageGroup = new QButtonGroup(frame);
  imageGroup->addButton(m_rbImageInFile);
  imageGroup->addButton(m_rbImageInAppDir);
  imageGroup->addButton(m_rbImageInLocalDir);
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
  void (QButtonGroup::* buttonClicked)(int) = &QButtonGroup::buttonClicked;
  connect(imageGroup, buttonClicked, this, &ConfigDialog::slotModified);
#else
  connect(imageGroup, &QButtonGroup::idClicked, this, &ConfigDialog::slotModified);
#endif

  QGroupBox* formatGroup = new QGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(formatGroup);
  QVBoxLayout* formatGroupLayout = new QVBoxLayout(formatGroup);
  formatGroup->setLayout(formatGroupLayout);

  m_cbCapitalize = new QCheckBox(i18n("Auto capitalize &titles and names"), formatGroup);
  m_cbCapitalize->setWhatsThis(i18n("If checked, titles and names will "
                                    "be automatically capitalized."));
  connect(m_cbCapitalize, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);
  formatGroupLayout->addWidget(m_cbCapitalize);

  m_cbFormat = new QCheckBox(i18n("Auto &format titles and names"), formatGroup);
  m_cbFormat->setWhatsThis(i18n("If checked, titles and names will "
                                "be automatically formatted."));
  connect(m_cbFormat, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);
  formatGroupLayout->addWidget(m_cbFormat);

  QWidget* g1 = new QWidget(formatGroup);
  QGridLayout* g1Layout = new QGridLayout(g1);
  g1->setLayout(g1Layout);
  formatGroupLayout->addWidget(g1);

  QLabel* lab = new QLabel(i18n("No capitali&zation:"), g1);
  g1Layout->addWidget(lab, 0, 0);
  m_leCapitals = new QLineEdit(g1);
  g1Layout->addWidget(m_leCapitals, 0, 1);
  lab->setBuddy(m_leCapitals);
  QString whats = i18n("<qt>A list of words which should not be capitalized. Multiple values "
                       "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_leCapitals->setWhatsThis(whats);
  connect(m_leCapitals, &QLineEdit::textChanged, this, &ConfigDialog::slotModified);

  lab = new QLabel(i18n("Artic&les:"), g1);
  g1Layout->addWidget(lab, 1, 0);
  m_leArticles = new QLineEdit(g1);
  g1Layout->addWidget(m_leArticles, 1, 1);
  lab->setBuddy(m_leArticles);
  whats = i18n("<qt>A list of words which should be considered as articles "
               "if they are the first word in a title. Multiple values "
               "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_leArticles->setWhatsThis(whats);
  connect(m_leArticles, &QLineEdit::textChanged, this, &ConfigDialog::slotModified);

  lab = new QLabel(i18n("Personal suffi&xes:"), g1);
  g1Layout->addWidget(lab, 2, 0);
  m_leSuffixes = new QLineEdit(g1);
  g1Layout->addWidget(m_leSuffixes, 2, 1);
  lab->setBuddy(m_leSuffixes);
  whats = i18n("<qt>A list of suffixes which might be used in personal names. Multiple values "
               "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_leSuffixes->setWhatsThis(whats);
  connect(m_leSuffixes, &QLineEdit::textChanged, this, &ConfigDialog::slotModified);

  lab = new QLabel(i18n("Surname &prefixes:"), g1);
  g1Layout->addWidget(lab, 3, 0);
  m_lePrefixes = new QLineEdit(g1);
  g1Layout->addWidget(m_lePrefixes, 3, 1);
  lab->setBuddy(m_lePrefixes);
  whats = i18n("<qt>A list of prefixes which might be used in surnames. Multiple values "
               "should be separated by a semi-colon.</qt>");
  lab->setWhatsThis(whats);
  m_lePrefixes->setWhatsThis(whats);
  connect(m_lePrefixes, &QLineEdit::textChanged, this, &ConfigDialog::slotModified);

  // stretch to fill lower area
  l->addStretch(1);
  m_initializedPages |= General;
  readGeneralConfig();
}

void ConfigDialog::setupPrintingPage() {
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("Printing"));
  page->setHeader(i18n("Printing Options"));
  page->setIcon(QIcon::fromTheme(QStringLiteral("printer")));
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
  connect(m_cbPrintFormatted, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);
  formatLayout->addWidget(m_cbPrintFormatted);

  m_cbPrintHeaders = new QCheckBox(i18n("&Print field headers"), formatOptions);
  m_cbPrintHeaders->setWhatsThis(i18n("If checked, the field names will be printed as table headers."));
  connect(m_cbPrintHeaders, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);
  formatLayout->addWidget(m_cbPrintHeaders);

  QGroupBox* groupOptions = new QGroupBox(i18n("Grouping Options"), frame);
  l->addWidget(groupOptions);
  QVBoxLayout* groupLayout = new QVBoxLayout(groupOptions);
  groupOptions->setLayout(groupLayout);

  m_cbPrintGrouped = new QCheckBox(i18n("&Group the entries"), groupOptions);
  m_cbPrintGrouped->setWhatsThis(i18n("If checked, the entries will be grouped by the selected field."));
  connect(m_cbPrintGrouped, &QAbstractButton::clicked, this, &ConfigDialog::slotModified);
  groupLayout->addWidget(m_cbPrintGrouped);

  QGroupBox* imageOptions = new QGroupBox(i18n("Image Options"), frame);
  l->addWidget(imageOptions);

  QGridLayout* gridLayout = new QGridLayout(imageOptions);
  imageOptions->setLayout(gridLayout);

  QLabel* lab = new QLabel(i18n("Maximum image &width:"), imageOptions);
  gridLayout->addWidget(lab, 0, 0);
  m_imageWidthBox = new QSpinBox(imageOptions);
  m_imageWidthBox->setMaximum(999);
  m_imageWidthBox->setMinimum(0);
  m_imageWidthBox->setValue(50);
  gridLayout->addWidget(m_imageWidthBox, 0, 1);
  m_imageWidthBox->setSuffix(QStringLiteral(" px"));
  lab->setBuddy(m_imageWidthBox);
  QString whats = i18n("The maximum width of the images in the printout. The aspect ratio is preserved.");
  lab->setWhatsThis(whats);
  m_imageWidthBox->setWhatsThis(whats);
  void (QSpinBox::* valueChanged)(int) = &QSpinBox::valueChanged;
  connect(m_imageWidthBox, valueChanged, this, &ConfigDialog::slotModified);

  lab = new QLabel(i18n("&Maximum image height:"), imageOptions);
  gridLayout->addWidget(lab, 1, 0);
  m_imageHeightBox = new QSpinBox(imageOptions);
  m_imageHeightBox->setMaximum(999);
  m_imageHeightBox->setMinimum(0);
  m_imageHeightBox->setValue(50);
  gridLayout->addWidget(m_imageHeightBox, 1, 1);
  m_imageHeightBox->setSuffix(QStringLiteral(" px"));
  lab->setBuddy(m_imageHeightBox);
  whats = i18n("The maximum height of the images in the printout. The aspect ratio is preserved.");
  lab->setWhatsThis(whats);
  m_imageHeightBox->setWhatsThis(whats);
  connect(m_imageHeightBox, valueChanged, this, &ConfigDialog::slotModified);

  // stretch to fill lower area
  l->addStretch(1);
  m_initializedPages |= Printing;
  readPrintingConfig();
}

void ConfigDialog::setupTemplatePage() {
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("Templates"));
  page->setHeader(i18n("Template Options"));
  // odd icon, I know, matches KMail, though...
  page->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-theme")));
  addPage(page);
}

void ConfigDialog::initTemplatePage(QFrame* frame) {
  QVBoxLayout* l = new QVBoxLayout(frame);

  QGridLayout* gridLayout = new QGridLayout();
  l->addLayout(gridLayout);

  int row = -1;
  // so I can reuse an i18n string, a plain label can't have an '&'
  QString s = KLocalizedString::removeAcceleratorMarker(i18n("Collection &type:"));
  QLabel* lab = new QLabel(s, frame);
  gridLayout->addWidget(lab, ++row, 0);
  const int collType = Kernel::self()->collectionType();
  lab = new QLabel(CollectionFactory::nameHash().value(collType), frame);
  gridLayout->addWidget(lab, row, 1, 1, 2);

  lab = new QLabel(i18n("Template:"), frame);
  m_templateCombo = new GUI::ComboBox(frame);
  void (QComboBox::* activatedInt)(int) = &QComboBox::activated;
  connect(m_templateCombo, activatedInt, this, &ConfigDialog::slotModified);
  lab->setBuddy(m_templateCombo);
  QString whats = i18n("Select the template to use for the current type of collections. "
                       "Not all templates will use the font and color settings.");
  lab->setWhatsThis(whats);
  m_templateCombo->setWhatsThis(whats);
  gridLayout->addWidget(lab, ++row, 0);
  gridLayout->addWidget(m_templateCombo, row, 1);

  QPushButton* btn = new QPushButton(i18n("&Preview..."), frame);
  btn->setWhatsThis(i18n("Show a preview of the template"));
  btn->setIcon(QIcon::fromTheme(QStringLiteral("zoom-original")));
  gridLayout->addWidget(btn, row, 2);
  connect(btn, &QAbstractButton::clicked, this, &ConfigDialog::slotShowTemplatePreview);

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
  connect(m_fontCombo, activatedInt, this, &ConfigDialog::slotModified);
  lab->setBuddy(m_fontCombo);
  whats = i18n("This font is passed to the template used in the Entry View.");
  lab->setWhatsThis(whats);
  m_fontCombo->setWhatsThis(whats);

  fontLayout->addWidget(new QLabel(i18n("Size:"), fontGroup), ++row, 0);
  m_fontSizeInput = new QSpinBox(fontGroup);
  m_fontSizeInput->setMaximum(30); // 30 is same max as konq config
  m_fontSizeInput->setMinimum(5);
  m_fontSizeInput->setSuffix(QStringLiteral("pt"));
  fontLayout->addWidget(m_fontSizeInput, row, 1);
  void (QSpinBox::* valueChangedInt)(int) = &QSpinBox::valueChanged;
  connect(m_fontSizeInput, valueChangedInt, this, &ConfigDialog::slotModified);
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
  connect(m_baseColorCombo, activatedInt, this, &ConfigDialog::slotModified);
  lab->setBuddy(m_baseColorCombo);
  whats = i18n("This color is passed to the template used in the Entry View.");
  lab->setWhatsThis(whats);
  m_baseColorCombo->setWhatsThis(whats);

  lab = new QLabel(i18n("Text color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_textColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_textColorCombo, row, 1);
  connect(m_textColorCombo, activatedInt, this, &ConfigDialog::slotModified);
  lab->setBuddy(m_textColorCombo);
  lab->setWhatsThis(whats);
  m_textColorCombo->setWhatsThis(whats);

  lab = new QLabel(i18n("Highlight color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_highBaseColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_highBaseColorCombo, row, 1);
  connect(m_highBaseColorCombo, activatedInt, this, &ConfigDialog::slotModified);
  lab->setBuddy(m_highBaseColorCombo);
  lab->setWhatsThis(whats);
  m_highBaseColorCombo->setWhatsThis(whats);

  lab = new QLabel(i18n("Highlighted text color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_highTextColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_highTextColorCombo, row, 1);
  connect(m_highTextColorCombo, activatedInt, this, &ConfigDialog::slotModified);
  lab->setBuddy(m_highTextColorCombo);
  lab->setWhatsThis(whats);
  m_highTextColorCombo->setWhatsThis(whats);

  lab = new QLabel(i18n("Link color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_linkColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_linkColorCombo, row, 1);
  connect(m_linkColorCombo, activatedInt, this, &ConfigDialog::slotModified);
  lab->setBuddy(m_linkColorCombo);
  lab->setWhatsThis(whats);
  m_linkColorCombo->setWhatsThis(whats);

  QGroupBox* groupBox = new QGroupBox(i18n("Manage Templates"), frame);
  l->addWidget(groupBox);
  QVBoxLayout* vlay = new QVBoxLayout(groupBox);
  groupBox->setLayout(vlay);

  QWidget* box1 = new QWidget(groupBox);
  QHBoxLayout* box1HBoxLayout = new QHBoxLayout(box1);
  box1HBoxLayout->setContentsMargins(0, 0, 0, 0);
  vlay->addWidget(box1);

  QPushButton* b1 = new QPushButton(i18n("Install..."), box1);
  box1HBoxLayout->addWidget(b1);
  b1->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
  connect(b1, &QAbstractButton::clicked, this, &ConfigDialog::slotInstallTemplate);
  whats = i18n("Click to install a new template directly.");
  b1->setWhatsThis(whats);

#ifdef ENABLE_KNEWSTUFF3
#if KNEWSTUFF_VERSION < QT_VERSION_CHECK(5, 91, 0)
  auto b2 = new KNS3::Button(i18n("Download..."), QStringLiteral("tellico-template.knsrc"), box1);
  connect(b2, &KNS3::Button::dialogFinished, this, &ConfigDialog::slotUpdateTemplates);
#else
  auto b2 = new KNSWidgets::Button(i18n("Download..."), QStringLiteral("tellico-template.knsrc"), box1);
  connect(b2, &KNSWidgets::Button::dialogFinished, this, &ConfigDialog::slotUpdateTemplates);
#endif
#else
  QPushButton* b2 = new QPushButton(i18n("Download..."), box1);
  b2->setIcon(QIcon::fromTheme(QStringLiteral("get-hot-new-stuff")));
  b2->setEnabled(false);
#endif
  box1HBoxLayout->addWidget(b2);
  whats = i18n("Click to download additional templates.");
  b2->setWhatsThis(whats);

  QPushButton* b3 = new QPushButton(i18n("Delete..."), box1);
  box1HBoxLayout->addWidget(b3);
  b3->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
  connect(b3, &QAbstractButton::clicked, this, &ConfigDialog::slotDeleteTemplate);
  whats = i18n("Click to select and remove installed templates.");
  b3->setWhatsThis(whats);

  // stretch to fill lower area
  l->addStretch(1);

  // purely for aesthetics make all widgets line up
  QList<QWidget*> widgets;
  widgets.append(m_fontCombo);
  widgets.append(m_fontSizeInput);
  widgets.append(m_baseColorCombo);
  widgets.append(m_textColorCombo);
  widgets.append(m_highBaseColorCombo);
  widgets.append(m_highTextColorCombo);
  widgets.append(m_linkColorCombo);
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
  QFrame* frame = new QFrame(this);
  KPageWidgetItem* page = new KPageWidgetItem(frame, i18n("Data Sources"));
  page->setHeader(i18n("Data Sources Options"));
  page->setIcon(QIcon::fromTheme(QStringLiteral("network-wired")));
  addPage(page);
}

void ConfigDialog::initFetchPage(QFrame* frame) {
  QHBoxLayout* l = new QHBoxLayout(frame);

  QVBoxLayout* leftLayout = new QVBoxLayout();
  l->addLayout(leftLayout);
  m_sourceListWidget = new QListWidget(frame);
  m_sourceListWidget->setSortingEnabled(false); // no sorting
  m_sourceListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  leftLayout->addWidget(m_sourceListWidget, 1);
  connect(m_sourceListWidget, &QListWidget::currentItemChanged, this, &ConfigDialog::slotSelectedSourceChanged);
  connect(m_sourceListWidget, &QListWidget::itemDoubleClicked, this, &ConfigDialog::slotModifySourceClicked);

  QWidget* hb = new QWidget(frame);
  QHBoxLayout* hbHBoxLayout = new QHBoxLayout(hb);
  hbHBoxLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->addWidget(hb);
  m_moveUpSourceBtn = new QPushButton(i18n("Move &Up"), hb);
  hbHBoxLayout->addWidget(m_moveUpSourceBtn);
  m_moveUpSourceBtn->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
  const QString moveTip(i18n("The order of the data sources sets the order "
                             "that Tellico uses when entries are automatically updated."));
  m_moveUpSourceBtn->setWhatsThis(moveTip);
  m_moveDownSourceBtn = new QPushButton(i18n("Move &Down"), hb);
  hbHBoxLayout->addWidget(m_moveDownSourceBtn);
  m_moveDownSourceBtn->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
  m_moveDownSourceBtn->setWhatsThis(moveTip);

  QWidget* hb2 = new QWidget(frame);
  QHBoxLayout* hb2HBoxLayout = new QHBoxLayout(hb2);
  hb2HBoxLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->addWidget(hb2);
  m_cbFilterSource = new QCheckBox(i18n("Filter by type:"), hb2);
  hb2HBoxLayout->addWidget(m_cbFilterSource);
  connect(m_cbFilterSource, &QAbstractButton::clicked, this, &ConfigDialog::slotSourceFilterChanged);
  m_sourceTypeCombo = new GUI::CollectionTypeCombo(hb2);
  hb2HBoxLayout->addWidget(m_sourceTypeCombo);
  void (QComboBox::* currentIndexChanged)(int) = &QComboBox::currentIndexChanged;
  connect(m_sourceTypeCombo, currentIndexChanged, this, &ConfigDialog::slotSourceFilterChanged);
  // we want to remove the item for a custom collection
  int index = m_sourceTypeCombo->findData(Data::Collection::Base);
  if(index > -1) {
    m_sourceTypeCombo->removeItem(index);
  }
  // disable until check box is checked
  m_sourceTypeCombo->setEnabled(false);

  // these icons are rather arbitrary, but seem to vaguely fit
  QVBoxLayout* vlay = new QVBoxLayout();
  l->addLayout(vlay);
  QPushButton* newSourceBtn = new QPushButton(i18n("&New..."), frame);
  newSourceBtn->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
  newSourceBtn->setWhatsThis(i18n("Click to add a new data source."));
  m_modifySourceBtn = new QPushButton(i18n("&Modify..."), frame);
  m_modifySourceBtn->setIcon(QIcon::fromTheme(QStringLiteral("network-wired")));
  m_modifySourceBtn->setWhatsThis(i18n("Click to modify the selected data source."));
  m_removeSourceBtn = new QPushButton(i18n("&Delete"), frame);
  m_removeSourceBtn->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
  m_removeSourceBtn->setWhatsThis(i18n("Click to delete the selected data source."));

  vlay->addWidget(newSourceBtn);
  vlay->addWidget(m_modifySourceBtn);
  vlay->addWidget(m_removeSourceBtn);
  vlay->addStretch(1);

  connect(newSourceBtn, &QAbstractButton::clicked, this, &ConfigDialog::slotNewSourceClicked);
  connect(m_modifySourceBtn, &QAbstractButton::clicked, this, &ConfigDialog::slotModifySourceClicked);
  connect(m_moveUpSourceBtn, &QAbstractButton::clicked, this, &ConfigDialog::slotMoveUpSourceClicked);
  connect(m_moveDownSourceBtn, &QAbstractButton::clicked, this, &ConfigDialog::slotMoveDownSourceClicked);
  connect(m_removeSourceBtn, &QAbstractButton::clicked, this, &ConfigDialog::slotRemoveSourceClicked);

  KAcceleratorManager::manage(frame);
  m_initializedPages |= Fetch;
  readFetchConfig();
}

void ConfigDialog::readGeneralConfig() {
  m_modifying = true;

  m_cbQuickFilterRegExp->setChecked(Config::quickFilterRegExp());
  m_cbOpenLastFile->setChecked(Config::reopenLastFile());
#ifdef ENABLE_WEBCAM
  m_cbEnableWebcam->setChecked(Config::enableWebcam());
#else
  m_cbEnableWebcam->setChecked(false);
  m_cbEnableWebcam->setEnabled(false);
#endif

  // block signals temporarily so the image location label isn't shown initially
  const QSignalBlocker block1(m_rbImageInFile);
  const QSignalBlocker block2(m_rbImageInAppDir);
  const QSignalBlocker block3(m_rbImageInLocalDir);
  switch(Config::imageLocation()) {
    case Config::ImagesInFile: m_rbImageInFile->setChecked(true); break;
    case Config::ImagesInAppDir: m_rbImageInAppDir->setChecked(true); break;
    case Config::ImagesInLocalDir: m_rbImageInLocalDir->setChecked(true); break;
  }

  bool autoCapitals = Config::autoCapitalization();
  m_cbCapitalize->setChecked(autoCapitals);

  bool autoFormat = Config::autoFormat();
  m_cbFormat->setChecked(autoFormat);

  static const QRegularExpression comma(QLatin1String("\\s*,\\s*"));

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
  m_templateCombo->setCurrentItem(i18nc(fileContext.toUtf8().constData(), file.toUtf8().constData()));

  m_fontCombo->setCurrentFont(QFont(Config::templateFont(collType).family()));
  m_fontSizeInput->setValue(Config::templateFont(collType).pointSize());
  m_baseColorCombo->setColor(Config::templateBaseColor(collType));
  m_textColorCombo->setColor(Config::templateTextColor(collType));
  m_highBaseColorCombo->setColor(Config::templateHighlightedBaseColor(collType));
  m_highTextColorCombo->setColor(Config::templateHighlightedTextColor(collType));
  m_linkColorCombo->setColor(Config::templateLinkColor(collType));

  m_modifying = false;
}

void ConfigDialog::readFetchConfig() {
  m_modifying = true;

  m_sourceListWidget->clear();
  m_configWidgets.clear();

  m_sourceListWidget->setUpdatesEnabled(false);
  foreach(Fetch::Fetcher::Ptr fetcher, Fetch::Manager::self()->fetchers()) {
    Fetch::FetcherInfo info(fetcher->type(), fetcher->source(),
                            fetcher->updateOverwrite(), fetcher->uuid());
    FetcherInfoListItem* item = new FetcherInfoListItem(m_sourceListWidget, info);
    item->setFetcher(fetcher.data());
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
  QTimer::singleShot(500, this, &ConfigDialog::slotCreateConfigWidgets);
}

void ConfigDialog::saveConfiguration() {
  if(isPageInitialized(General)) saveGeneralConfig();
  if(isPageInitialized(Printing)) savePrintingConfig();
  if(isPageInitialized(Template)) saveTemplateConfig();
  if(isPageInitialized(Fetch)) saveFetchConfig();
}

void ConfigDialog::saveGeneralConfig() {
  Config::setQuickFilterRegExp(m_cbQuickFilterRegExp->isChecked());
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

  static const QRegularExpression semicolon(QLatin1String("\\s*;\\s*"));
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
  if(collType == Data::Collection::Base &&
     Kernel::self()->URL().fileName() != TC_I18N1(Tellico::untitledFilename)) {
    // use a nested config group for template specific to custom collections
    // using the filename alone as a keyEvents
    const QString configGroup = QStringLiteral("Options - %1").arg(CollectionFactory::typeName(collType));
    KConfigGroup group(KSharedConfig::openConfig(), configGroup);
    KConfigGroup subGroup(&group, Kernel::self()->URL().fileName());
    subGroup.writeEntry(QStringLiteral("Template Name"), m_templateCombo->currentData().toString());
  } else {
    Config::setTemplateName(collType, m_templateCombo->currentData().toString());
  }
  QFont font(m_fontCombo->currentFont().family(), m_fontSizeInput->value());
  Config::setTemplateFont(collType, font);
  Config::setTemplateBaseColor(collType, m_baseColorCombo->color());
  Config::setTemplateTextColor(collType, m_textColorCombo->color());
  Config::setTemplateHighlightedBaseColor(collType, m_highBaseColorCombo->color());
  Config::setTemplateHighlightedTextColor(collType, m_highTextColorCombo->color());
  Config::setTemplateLinkColor(collType, m_linkColorCombo->color());
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
    FetcherInfoListItem* item = static_cast<FetcherInfoListItem*>(m_sourceListWidget->item(count));
    Fetch::ConfigWidget* cw = m_configWidgets[item];
    if(!cw || (!cw->shouldSave() && !item->isNewSource())) {
      continue;
    }
    m_newStuffConfigWidgets.removeAll(cw);
    QString group = QStringLiteral("Data Source %1").arg(count);
    // in case we later change the order, clear the group now
    KSharedConfig::openConfig()->deleteGroup(group);
    KConfigGroup configGroup(KSharedConfig::openConfig(), group);
    configGroup.writeEntry("Name", item->data(Qt::DisplayRole).toString());
    configGroup.writeEntry("Type", int(item->fetchType()));
    configGroup.writeEntry("UpdateOverwrite", item->updateOverwrite());
    configGroup.writeEntry("Uuid", item->uuid());
    cw->saveConfig(configGroup);
    item->setNewSource(false);
    // in case the ordering changed
    item->setConfigGroup(configGroup);
    reloadFetchers = true;
  }
  // now update total number of sources
  KConfigGroup sourceGroup(KSharedConfig::openConfig(), QLatin1String("Data Sources"));
  sourceGroup.writeEntry("Sources Count", count);
  // and purge old config groups
  const QString dataSource1(QStringLiteral("Data Source %1"));
  QString group = dataSource1.arg(count);
  while(KSharedConfig::openConfig()->hasGroup(group)) {
    KSharedConfig::openConfig()->deleteGroup(group);
    ++count;
    group = dataSource1.arg(count);
  }

  Config::self()->save();

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
  button(QDialogButtonBox::Ok)->setEnabled(true);
  button(QDialogButtonBox::Apply)->setEnabled(true);
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

  Fetch::FetcherInfo info(type, dlg.sourceName(), dlg.updateOverwrite());
  FetcherInfoListItem* item = new FetcherInfoListItem(m_sourceListWidget, info);
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
  FetcherInfoListItem* item = static_cast<FetcherInfoListItem*>(m_sourceListWidget->currentItem());
  if(!item) {
    return;
  }

  Fetch::ConfigWidget* cw = nullptr;
  if(m_configWidgets.contains(item)) {
    cw = m_configWidgets[item];
  } else if(item->fetcher()) {
    // grab the config widget, taking ownership
    cw = item->fetcher()->configWidget(this);
    if(cw) { // might return null when no widget available for fetcher type
      m_configWidgets.insert(item, cw);
      // there's weird layout bug if it's not hidden
      cw->hide();
    }
  } else {
    myDebug() << "no config item fetcher!";
  }
  if(!cw) {
    // no config widget for this one
    // might be because support was compiled out
    myDebug() << "no config widget for source" << item->data(Qt::DisplayRole).toString();
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
  FetcherInfoListItem* item = static_cast<FetcherInfoListItem*>(m_sourceListWidget->currentItem());
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

void ConfigDialog::slotSourceFilterChanged() {
  m_sourceTypeCombo->setEnabled(m_cbFilterSource->isChecked());
  const bool showAll = !m_sourceTypeCombo->isEnabled();
  const int type = m_sourceTypeCombo->currentType();
  for(int count = 0; count < m_sourceListWidget->count(); ++count) {
    FetcherInfoListItem* item = static_cast<FetcherInfoListItem*>(m_sourceListWidget->item(count));
    item->setHidden(!showAll && item->fetcher() && !item->fetcher()->canFetch(type));
  }
}

void ConfigDialog::slotSelectedSourceChanged(QListWidgetItem* item_) {
  int row = m_sourceListWidget->row(item_);
  m_moveUpSourceBtn->setEnabled(row > 0);
  m_moveDownSourceBtn->setEnabled(row < m_sourceListWidget->count()-1);
}

Tellico::FetcherInfoListItem* ConfigDialog::findItem(const QString& path_) const {
  if(path_.isEmpty()) {
    myDebug() << "empty path";
    return nullptr;
  }

  // this is a bit ugly, loop over all items, find the execexternal one
  // that matches the path
  for(int i = 0; i < m_sourceListWidget->count(); ++i) {
    FetcherInfoListItem* item = static_cast<FetcherInfoListItem*>(m_sourceListWidget->item(i));
    if(item->fetchType() != Fetch::ExecExternal) {
      continue;
    }
    Fetch::ExecExternalFetcher* f = dynamic_cast<Fetch::ExecExternalFetcher*>(item->fetcher());
    if(f && f->execPath() == path_) {
      return item;
    }
  }
  myDebug() << "no matching item found";
  return nullptr;
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
  options.linkColor  = m_linkColorCombo->color();
  dlg->setXSLTOptions(Kernel::self()->collectionType(), options);

  // always want to include a url to show link color too
  bool hasLink = false;
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
      e->setField(f->name(), QStringLiteral("1"));
    } else if(f->type() == Data::Field::Bool) {
      e->setField(f->name(), QStringLiteral("true"));
    } else if(f->type() == Data::Field::Rating) {
      e->setField(f->name(), QStringLiteral("4"));
    } else if(f->type() == Data::Field::URL) {
      e->setField(f->name(), QStringLiteral("https://tellico-project.org"));
      hasLink = true;
    } else if(f->type() == Data::Field::Table) {
      QStringList values;
      bool ok;
      int ncols = Tellico::toUInt(f->property(QStringLiteral("columns")), &ok);
      ncols = qMax(ncols, 1);
      for(int ncol = 1; ncol <= ncols; ++ncol) {
        const auto prop = QStringLiteral("column%1").arg(ncol);
        const auto col = f->property(prop);
        values += col.isEmpty() ? prop : col;
      }
      e->setField(f->name(), values.join(FieldFormat::columnDelimiterString()));
    } else {
      e->setField(f->name(), f->title());
    }
  }
  if(!hasLink) {
    Data::FieldPtr f(new Data::Field(QStringLiteral("url"),
                                     QLatin1String("URL"),
                                     Data::Field::URL));
    f->setCategory(i18n("General"));
    c->addField(f);
    e->setField(f->name(), QStringLiteral("https://tellico-project.org"));
  }

  dlg->showEntry(e);
  dlg->show();
  // dlg gets deleted by itself
  // the finished() signal is connected in its constructor to delayedDestruct
}

void ConfigDialog::loadTemplateList() {
  QStringList files = Tellico::locateAllFiles(QStringLiteral("tellico/entry-templates/*.xsl"));
  QMap<QString, QString> templates; // a QMap will have them values sorted by key
  foreach(const QString& file, files) {
    QFileInfo fi(file);
    QString lfile = fi.fileName().section(QLatin1Char('.'), 0, -2);
    QString name = lfile;
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    QString title = i18nc((name + QLatin1String(" XSL Template")).toUtf8().constData(), name.toUtf8().constData());
    templates.insert(title, lfile);
  }

  QString s = m_templateCombo->currentText();
  m_templateCombo->clear();
  for(auto it2 = templates.constBegin(); it2 != templates.constEnd(); ++it2) {
    m_templateCombo->addItem(it2.key(), it2.value());
  }
  m_templateCombo->setCurrentItem(s);
}

void ConfigDialog::slotInstallTemplate() {
  QString filter = i18n("XSL Files") + QLatin1String(" (*.xsl)") + QLatin1String(";;");
  filter += i18n("Template Packages") + QLatin1String(" (*.tar.gz *.tgz)") + QLatin1String(";;");
  filter += i18n("All Files") + QLatin1String(" (*)");

  const QString fileClass(QStringLiteral(":InstallTemplate"));
  const QString f = QFileDialog::getOpenFileName(this, QString(), KRecentDirs::dir(fileClass), filter);
  if(f.isEmpty()) {
    return;
  }
  KRecentDirs::add(fileClass, QFileInfo(f).dir().canonicalPath());

  if(Tellico::NewStuff::Manager::self()->installTemplate(f)) {
    loadTemplateList();
  }
}

#ifdef ENABLE_KNEWSTUFF3
#if KNEWSTUFF_VERSION < QT_VERSION_CHECK(5, 91, 0)
void ConfigDialog::slotUpdateTemplates(const QList<KNS3::Entry>& list_) {
#else
void ConfigDialog::slotUpdateTemplates(const QList<KNSCore::Entry>& list_) {
#endif
  if(!list_.isEmpty()) {
    loadTemplateList();
  }
}
#endif

void ConfigDialog::slotDeleteTemplate() {
  bool ok;
  QString name = QInputDialog::getItem(this,
                                       i18n("Delete Template"),
                                       i18n("Select template to delete:"),
                                       Tellico::NewStuff::Manager::self()->userTemplates().keys(),
                                       0, false, &ok);
  if(ok && !name.isEmpty()) {
    Tellico::NewStuff::Manager::self()->removeTemplateByName(name);
    loadTemplateList();
  }
}

void ConfigDialog::slotCreateConfigWidgets() {
  for(int count = 0; count < m_sourceListWidget->count(); ++count) {
    FetcherInfoListItem* item = static_cast<FetcherInfoListItem*>(m_sourceListWidget->item(count));
    // only create a new config widget if we don't have one already
    if(!m_configWidgets.contains(item) && item->fetcher()) {
      Fetch::ConfigWidget* cw = item->fetcher()->configWidget(this);
      if(cw) { // might return 0 when no widget available for fetcher type
        m_configWidgets.insert(item, cw);
        // there's weird layout bug if it's not hidden
        cw->hide();
      }
    }
  }
}

void ConfigDialog::slotUpdateImageLocationLabel() {
  int newImageLocation;
  QString newImageDir;
  if(m_rbImageInFile->isChecked()) {
    newImageLocation = Config::ImagesInFile;
  } else if(m_rbImageInAppDir->isChecked()) {
    newImageLocation = Config::ImagesInAppDir;
    newImageDir = ImageFactory::dataDir().toString(QUrl::PreferLocalFile);
  } else {
    newImageLocation = Config::ImagesInLocalDir;
    newImageDir = ImageFactory::localDir().toString(QUrl::PreferLocalFile);
  }

  const QString fileName = Kernel::self()->URL().fileName();
  const QString imageDir = ImageFactory::imageDir().toString(QUrl::PreferLocalFile);
  QString locationText;
  if(newImageLocation == Config::imageLocation()) {
    if(newImageLocation == Config::ImagesInFile) {
      locationText = i18nc("%1 refers to the file name",
                           "Images are currently saved within <em>%1</em>",
                           fileName);
    } else {
      locationText = i18nc("%1 refers to a directory",
                           "Images are currently saved to <em>%1</em>",
                           imageDir);
    }
  } else {
    if(newImageLocation == Config::ImagesInFile) {
      locationText = i18nc("%1 refers to a directory, %2 to a file name",
                           "Images will be moved from <em>%1</em> to <em>%2</em>",
                           imageDir, fileName);
    } else if(Config::imageLocation() == Config::ImagesInFile) {
      locationText = i18nc("%1 refers to a file name, %2 to a directory",
                           "Images will be moved from <em>%1</em> to <em>%2</em>",
                           fileName, newImageDir);
    } else {
      locationText = i18nc("%1 and %2 are both directories",
                           "Images will be moved from <em>%1</em> to <em>%2</em>",
                           imageDir, newImageDir);
    }
  }
  m_mwImageLocation->setText(locationText);
  if(!m_mwImageLocation->isVisible()) {
    m_mwImageLocation->animatedShow();
  }
  m_infoTimer->start();
}
