/***************************************************************************
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
#include "latin1literal.h"
#include "tellico_utils.h"
#include "core/tellico_config.h"
#include "imagefactory.h"
#include "gui/previewdialog.h"
#include "newstuff/dialog.h"

#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <knuminput.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <ksortablevaluelist.h>
#include <kaccelmanager.h>
#include <khtmlview.h>
#include <kfiledialog.h>
#include <kinputdialog.h>
#include <kfontcombo.h>
#include <kcolorcombo.h>

#include <qsize.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qptrlist.h>
#include <qpixmap.h>
#include <qgrid.h>
#include <qwhatsthis.h>
#include <qregexp.h>
#include <qhgroupbox.h>
#include <qvgroupbox.h>
#include <qpushbutton.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qfileinfo.h>

namespace {
  static const int CONFIG_MIN_WIDTH = 640;
  static const int CONFIG_MIN_HEIGHT = 420;
}

using Tellico::SourceListViewItem;
using Tellico::ConfigDialog;

SourceListViewItem::SourceListViewItem(KListView* parent_, const GeneralFetcherInfo& info_,
                                       const QString& groupName_)
    : KListViewItem(parent_, info_.name), m_info(info_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()), m_fetcher(0) {
  QPixmap pix = Fetch::Manager::fetcherIcon(info_.type);
  if(!pix.isNull()) {
    setPixmap(0, pix);
  }
}

SourceListViewItem::SourceListViewItem(KListView* parent_, QListViewItem* after_,
                                       const GeneralFetcherInfo& info_, const QString& groupName_)
    : KListViewItem(parent_, after_, info_.name), m_info(info_),
      m_configGroup(groupName_), m_newSource(groupName_.isNull()), m_fetcher(0) {
  QPixmap pix = Fetch::Manager::fetcherIcon(info_.type);
  if(!pix.isNull()) {
    setPixmap(0, pix);
  }
}

void SourceListViewItem::setFetcher(Fetch::Fetcher::Ptr fetcher) {
  m_fetcher = fetcher;
  QPixmap pix = Fetch::Manager::fetcherIcon(fetcher.data());
  if(!pix.isNull()) {
    setPixmap(0, pix);
  }
}

ConfigDialog::ConfigDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(IconList, i18n("Configure Tellico"), Help|Ok|Apply|Cancel|Default,
                  Ok, parent_, name_, true, false)
    , m_modifying(false) {
  setupGeneralPage();
  setupPrintingPage();
  setupTemplatePage();
  setupFetchPage();

  updateGeometry();
  QSize s = sizeHint();
  resize(QMAX(s.width(), CONFIG_MIN_WIDTH), QMAX(s.height(), CONFIG_MIN_HEIGHT));

  // purely for asthetics make all widgets line up
  QPtrList<QWidget> widgets;
  widgets.append(m_fontCombo);
  widgets.append(m_fontSizeInput);
  widgets.append(m_baseColorCombo);
  widgets.append(m_textColorCombo);
  widgets.append(m_highBaseColorCombo);
  widgets.append(m_highTextColorCombo);
  int w = 0;
  for(QPtrListIterator<QWidget> it(widgets); it.current(); ++it) {
    it.current()->polish();
    w = QMAX(w, it.current()->sizeHint().width());
  }
  for(QPtrListIterator<QWidget> it(widgets); it.current(); ++it) {
    it.current()->setMinimumWidth(w);
  }

  enableButtonOK(false);
  enableButtonApply(false);

  setHelp(QString::fromLatin1("general-options"));
  connect(this, SIGNAL(aboutToShowPage(QWidget*)), SLOT(slotUpdateHelpLink(QWidget*)));
}

ConfigDialog::~ConfigDialog() {
  for(QPtrListIterator<Fetch::ConfigWidget> it(m_newStuffConfigWidgets); it.current(); ++it) {
    it.current()->removed();
  }
}

void ConfigDialog::slotUpdateHelpLink(QWidget* w_) {
  switch(pageIndex(w_)) {
    case 0:
      setHelp(QString::fromLatin1("general-options"));
      break;

    case 1:
      setHelp(QString::fromLatin1("printing-options"));
      break;

    case 2:
      setHelp(QString::fromLatin1("template-options"));
      break;

    case 3:
      setHelp(QString::fromLatin1("internet-sources-options"));
      break;

    default:
      break;
  }
}

void ConfigDialog::slotOk() {
  slotApply();
  accept();
}

void ConfigDialog::slotApply() {
  emit signalConfigChanged();
  enableButtonApply(false);
}

void ConfigDialog::slotDefault() {
  // only change the defaults on the active page
  Config::self()->useDefaults(true);
  switch(activePageIndex()) {
    case 0:
      readGeneralConfig(); break;
    case 1:
      readPrintingConfig(); break;
    case 2:
      readTemplateConfig(); break;
  }
  Config::self()->useDefaults(false);
  slotModified();
}

void ConfigDialog::setupGeneralPage() {
  QPixmap pix = DesktopIcon(QString::fromLatin1("tellico"), KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("General"), i18n("General Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  m_cbWriteImagesInFile = new QCheckBox(i18n("&Include images in data file"), frame);
  QWhatsThis::add(m_cbWriteImagesInFile, i18n("If checked, all images will be included in the data file, "
                                              "rather than saved separately in the Tellico data directory. "
                                              "Saving a lot of images in the data file cause Tellico to "
                                              "run more slowly."));
  l->addWidget(m_cbWriteImagesInFile);
  connect(m_cbWriteImagesInFile, SIGNAL(clicked()), SLOT(slotModified()));

  m_cbOpenLastFile = new QCheckBox(i18n("&Reopen file at startup"), frame);
  QWhatsThis::add(m_cbOpenLastFile, i18n("If checked, the file that was last open "
                                         "will be re-opened at program start-up."));
  l->addWidget(m_cbOpenLastFile);
  connect(m_cbOpenLastFile, SIGNAL(clicked()), SLOT(slotModified()));

  m_cbShowTipDay = new QCheckBox(i18n("&Show \"Tip of the Day\" at startup"), frame);
  QWhatsThis::add(m_cbShowTipDay, i18n("If checked, the \"Tip of the Day\" will be "
                                       "shown at program start-up."));
  l->addWidget(m_cbShowTipDay);
  connect(m_cbShowTipDay, SIGNAL(clicked()), SLOT(slotModified()));

  QVGroupBox* formatGroup = new QVGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(formatGroup);

  m_cbCapitalize = new QCheckBox(i18n("Auto capitalize &titles and names"), formatGroup);
  QWhatsThis::add(m_cbCapitalize, i18n("If checked, titles and names will "
                                       "be automatically capitalized."));
  connect(m_cbCapitalize, SIGNAL(toggled(bool)), SLOT(slotToggleCapitalized(bool)));
  connect(m_cbCapitalize, SIGNAL(clicked()), SLOT(slotModified()));

  m_cbFormat = new QCheckBox(i18n("Auto &format titles and names"), formatGroup);
  QWhatsThis::add(m_cbFormat, i18n("If checked, titles and names will "
                                   "be automatically formatted."));
  connect(m_cbFormat, SIGNAL(toggled(bool)), SLOT(slotToggleFormatted(bool)));
  connect(m_cbFormat, SIGNAL(clicked()), SLOT(slotModified()));

  QGrid* g1 = new QGrid(2, formatGroup);
  g1->setSpacing(5);

  QLabel* lab = new QLabel(i18n("No capitali&zation:"), g1);
  m_leCapitals = new KLineEdit(g1);
  lab->setBuddy(m_leCapitals);
  QString whats = i18n("<qt>A list of words which should not be capitalized. Multiple values "
                       "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_leCapitals, whats);
  connect(m_leCapitals, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("Artic&les:"), g1);
  m_leArticles = new KLineEdit(g1);
  lab->setBuddy(m_leArticles);
  whats = i18n("<qt>A list of words which should be considered as articles "
               "if they are the first word in a title. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_leArticles, whats);
  connect(m_leArticles, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("Personal suffi&xes:"), g1);
  m_leSuffixes = new KLineEdit(g1);
  lab->setBuddy(m_leSuffixes);
  whats = i18n("<qt>A list of suffixes which might be used in personal names. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_leSuffixes, whats);
  connect(m_leSuffixes, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("Surname &prefixes:"), g1);
  m_lePrefixes = new KLineEdit(g1);
  lab->setBuddy(m_lePrefixes);
  whats = i18n("<qt>A list of prefixes which might be used in surnames. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_lePrefixes, whats);
  connect(m_lePrefixes, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupPrintingPage() {
  // SuSE changed the icon name on me
  QPixmap pix;
  KIconLoader* loader = KGlobal::iconLoader();
  if(loader) {
    pix = loader->loadIcon(QString::fromLatin1("printer1"), KIcon::Desktop, KIcon::SizeMedium,
                           KIcon::DefaultState, 0, true /*canReturnNull */);
    if(pix.isNull()) {
      pix = loader->loadIcon(QString::fromLatin1("printer2"), KIcon::Desktop, KIcon::SizeMedium,
                             KIcon::DefaultState, 0, true /*canReturnNull */);
    }
    if(pix.isNull()) {
      pix = loader->loadIcon(QString::fromLatin1("print_printer"), KIcon::Desktop, KIcon::SizeMedium);
    }
  }
  QFrame* frame = addPage(i18n("Printing"), i18n("Printing Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  QVGroupBox* formatOptions = new QVGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(formatOptions);

  m_cbPrintFormatted = new QCheckBox(i18n("&Format titles and names"), formatOptions);
  QWhatsThis::add(m_cbPrintFormatted, i18n("If checked, titles and names will be automatically formatted."));
  connect(m_cbPrintFormatted, SIGNAL(clicked()), SLOT(slotModified()));

  m_cbPrintHeaders = new QCheckBox(i18n("&Print field headers"), formatOptions);
  QWhatsThis::add(m_cbPrintHeaders, i18n("If checked, the field names will be printed as table headers."));
  connect(m_cbPrintHeaders, SIGNAL(clicked()), SLOT(slotModified()));

  QHGroupBox* groupOptions = new QHGroupBox(i18n("Grouping Options"), frame);
  l->addWidget(groupOptions);

  m_cbPrintGrouped = new QCheckBox(i18n("&Group the entries"), groupOptions);
  QWhatsThis::add(m_cbPrintGrouped, i18n("If checked, the entries will be grouped by the selected field."));
  connect(m_cbPrintGrouped, SIGNAL(clicked()), SLOT(slotModified()));

  QVGroupBox* imageOptions = new QVGroupBox(i18n("Image Options"), frame);
  l->addWidget(imageOptions);

  QGrid* grid = new QGrid(3, imageOptions);
  grid->setSpacing(5);

  QLabel* lab = new QLabel(i18n("Maximum image &width:"), grid);
  m_imageWidthBox = new KIntSpinBox(0, 999, 1, 50, 10, grid);
  m_imageWidthBox->setSuffix(QString::fromLatin1(" px"));
  lab->setBuddy(m_imageWidthBox);
  (void) new QWidget(grid);
  QString whats = i18n("The maximum width of the images in the printout. The aspect ration is preserved.");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_imageWidthBox, whats);
  connect(m_imageWidthBox, SIGNAL(valueChanged(int)), SLOT(slotModified()));
  // QSpinBox doesn't emit valueChanged if you edit the value with
  // the lineEdit until you change the keyboard focus
  connect(m_imageWidthBox->child("qt_spinbox_edit"), SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("&Maximum image height:"), grid);
  m_imageHeightBox = new KIntSpinBox(0, 999, 1, 50, 10, grid);
  m_imageHeightBox->setSuffix(QString::fromLatin1(" px"));
  lab->setBuddy(m_imageHeightBox);
  (void) new QWidget(grid);
  whats = i18n("The maximum height of the images in the printout. The aspect ration is preserved.");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_imageHeightBox, whats);
  connect(m_imageHeightBox, SIGNAL(valueChanged(int)), SLOT(slotModified()));
  // QSpinBox doesn't emit valueChanged if you edit the value with
  // the lineEdit until you change the keyboard focus
  connect(m_imageHeightBox->child("qt_spinbox_edit"), SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupTemplatePage() {
  QPixmap pix = DesktopIcon(QString::fromLatin1("looknfeel"), KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("Templates"), i18n("Template Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  QGridLayout* gridLayout = new QGridLayout(l);
  gridLayout->setSpacing(KDialogBase::spacingHint());

  int row = -1;
  // so I can reuse an i18n string, a plain label can't have an '&'
  QLabel* lab = new QLabel(i18n("Collection &type:").remove('&'), frame);
  gridLayout->addWidget(lab, ++row, 0);
  const int collType = Kernel::self()->collectionType();
  lab = new QLabel(CollectionFactory::nameMap()[collType], frame);
  gridLayout->addMultiCellWidget(lab, row, row, 1, 2);

  lab = new QLabel(i18n("Template:"), frame);
  m_templateCombo = new GUI::ComboBox(frame);
  connect(m_templateCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_templateCombo);
  QString whats = i18n("Select the template to use for the current type of collections. "
                       "Not all templates will use the font and color settings.");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_templateCombo, whats);
  gridLayout->addWidget(lab, ++row, 0);
  gridLayout->addWidget(m_templateCombo, row, 1);

  KPushButton* btn = new KPushButton(i18n("&Preview..."), frame);
  QWhatsThis::add(btn, i18n("Show a preview of the template"));
  btn->setIconSet(SmallIconSet(QString::fromLatin1("viewmag")));
  gridLayout->addWidget(btn, row, 2);
  connect(btn, SIGNAL(clicked()), SLOT(slotShowTemplatePreview()));

  // so the button is squeezed small
  gridLayout->setColStretch(0, 10);
  gridLayout->setColStretch(1, 10);

  loadTemplateList();

//  QLabel* l1 = new QLabel(i18n("The options below will be passed to the template, but not "
//                               "all templates will use them. Some fonts and colors may be "
//                               "specified directly in the template."), frame);
//  l1->setTextFormat(Qt::RichText);
//  l->addWidget(l1);

  QGroupBox* fontGroup = new QGroupBox(0, Qt::Vertical, i18n("Font Options"), frame);
  l->addWidget(fontGroup);

  row = -1;
  QGridLayout* fontLayout = new QGridLayout(fontGroup->layout());
  fontLayout->setSpacing(KDialogBase::spacingHint());

  lab = new QLabel(i18n("Font:"), fontGroup);
  fontLayout->addWidget(lab, ++row, 0);
  m_fontCombo = new KFontCombo(fontGroup);
  fontLayout->addWidget(m_fontCombo, row, 1);
  connect(m_fontCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_fontCombo);
  whats = i18n("This font is passed to the template used in the Entry View.");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_fontCombo, whats);

  fontLayout->addWidget(new QLabel(i18n("Size:"), fontGroup), ++row, 0);
  m_fontSizeInput = new KIntNumInput(fontGroup);
  m_fontSizeInput->setRange(5, 30); // 30 is same max as konq config
  m_fontSizeInput->setSuffix(QString::fromLatin1("pt"));
  fontLayout->addWidget(m_fontSizeInput, row, 1);
  connect(m_fontSizeInput, SIGNAL(valueChanged(int)), SLOT(slotModified()));
  lab->setBuddy(m_fontSizeInput);
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_fontSizeInput, whats);

  QGroupBox* colGroup = new QGroupBox(0, Qt::Vertical, i18n("Color Options"), frame);
  l->addWidget(colGroup);

  row = -1;
  QGridLayout* colLayout = new QGridLayout(colGroup->layout());
  colLayout->setSpacing(KDialogBase::spacingHint());

  lab = new QLabel(i18n("Background color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_baseColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_baseColorCombo, row, 1);
  connect(m_baseColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_baseColorCombo);
  whats = i18n("This color is passed to the template used in the Entry View.");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_baseColorCombo, whats);

  lab = new QLabel(i18n("Text color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_textColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_textColorCombo, row, 1);
  connect(m_textColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_textColorCombo);
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_textColorCombo, whats);

  lab = new QLabel(i18n("Highlight color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_highBaseColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_highBaseColorCombo, row, 1);
  connect(m_highBaseColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_highBaseColorCombo);
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_highBaseColorCombo, whats);

  lab = new QLabel(i18n("Highlighted text color:"), colGroup);
  colLayout->addWidget(lab, ++row, 0);
  m_highTextColorCombo = new KColorCombo(colGroup);
  colLayout->addWidget(m_highTextColorCombo, row, 1);
  connect(m_highTextColorCombo, SIGNAL(activated(int)), SLOT(slotModified()));
  lab->setBuddy(m_highTextColorCombo);
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_highTextColorCombo, whats);

  QVGroupBox* groupBox = new QVGroupBox(i18n("Manage Templates"), frame);
  l->addWidget(groupBox);

  QHBox* box1 = new QHBox(groupBox);
  box1->setSpacing(KDialog::spacingHint());

  KPushButton* b1 = new KPushButton(i18n("Install..."), box1);
  b1->setIconSet(SmallIconSet(QString::fromLatin1("add")));
  connect(b1, SIGNAL(clicked()), SLOT(slotInstallTemplate()));
  whats = i18n("Click to install a new template directly.");
  QWhatsThis::add(b1, whats);

  KPushButton* b2 = new KPushButton(i18n("Download.."), box1);
  b2->setIconSet(SmallIconSet(QString::fromLatin1("knewstuff")));
  connect(b2, SIGNAL(clicked()), SLOT(slotDownloadTemplate()));
  whats = i18n("Click to download additional templates via the Internet.");
  QWhatsThis::add(b2, whats);

  KPushButton* b3 = new KPushButton(i18n("Delete..."), box1);
  b3->setIconSet(SmallIconSet(QString::fromLatin1("remove")));
  connect(b3, SIGNAL(clicked()), SLOT(slotDeleteTemplate()));
  whats = i18n("Click to select and remove installed templates.");
  QWhatsThis::add(b3, whats);

  // stretch to fill lower area
  l->addStretch(1);

  KAcceleratorManager::manage(frame);
}

void ConfigDialog::setupFetchPage() {
  QPixmap pix = DesktopIcon(QString::fromLatin1("network"), KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("Data Sources"), i18n("Data Source Options"), pix);
  QHBoxLayout* l = new QHBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  QVBoxLayout* leftLayout = new QVBoxLayout(l);
  m_sourceListView = new KListView(frame);
  m_sourceListView->addColumn(i18n("Source"));
  m_sourceListView->setResizeMode(QListView::LastColumn);
  m_sourceListView->setSorting(-1); // no sorting
  m_sourceListView->setSelectionMode(QListView::Single);
  leftLayout->addWidget(m_sourceListView, 1);
  connect(m_sourceListView, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotSelectedSourceChanged(QListViewItem*)));
  connect(m_sourceListView, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)), SLOT(slotModifySourceClicked()));

  QHBox* hb = new QHBox(frame);
  leftLayout->addWidget(hb);
  hb->setSpacing(KDialog::spacingHint());
  m_moveUpSourceBtn = new KPushButton(i18n("Move &Up"), hb);
  m_moveUpSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("up")));
  QWhatsThis::add(m_moveUpSourceBtn, i18n("The order of the data sources sets the order "
                                          "that Tellico uses when entries are automatically updated."));
  m_moveDownSourceBtn = new KPushButton(i18n("Move &Down"), hb);
  m_moveDownSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("down")));
  QWhatsThis::add(m_moveDownSourceBtn, i18n("The order of the data sources sets the order "
                                            "that Tellico uses when entries are automatically updated."));

  // these icons are rather arbitrary, but seem to vaguely fit
  QVBoxLayout* vlay = new QVBoxLayout(l);
  KPushButton* newSourceBtn = new KPushButton(i18n("&New..."), frame);
  newSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("filenew")));
  QWhatsThis::add(newSourceBtn, i18n("Click to add a new data source."));
  m_modifySourceBtn = new KPushButton(i18n("&Modify..."), frame);
  m_modifySourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("network")));
  QWhatsThis::add(m_modifySourceBtn, i18n("Click to modify the selected data source."));
  m_removeSourceBtn = new KPushButton(i18n("&Delete"), frame);
  m_removeSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("remove")));
  QWhatsThis::add(m_removeSourceBtn, i18n("Click to delete the selected data source."));
  m_newStuffBtn = new KPushButton(i18n("Download..."), frame);
  m_newStuffBtn->setIconSet(SmallIconSet(QString::fromLatin1("knewstuff")));
  QWhatsThis::add(m_newStuffBtn, i18n("Click to download additional data sources via the Internet."));

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
}

void ConfigDialog::readConfiguration() {
  m_modifying = true;

  readGeneralConfig();
  readPrintingConfig();
  readTemplateConfig();
  readFetchConfig();

  m_modifying = false;
}

void ConfigDialog::readGeneralConfig() {
  m_cbShowTipDay->setChecked(Config::showTipOfDay());
  m_cbWriteImagesInFile->setChecked(Config::writeImagesInFile());
  m_cbOpenLastFile->setChecked(Config::reopenLastFile());

  bool autoCapitals = Config::autoCapitalization();
  m_cbCapitalize->setChecked(autoCapitals);
  slotToggleCapitalized(autoCapitals);

  bool autoFormat = Config::autoFormat();
  m_cbFormat->setChecked(autoFormat);
  slotToggleFormatted(autoFormat);

  const QRegExp comma(QString::fromLatin1("\\s*,\\s*"));
  const QString semicolon = QString::fromLatin1("; ");

  m_leCapitals->setText(Config::noCapitalizationString().replace(comma, semicolon));
  m_leArticles->setText(Config::articlesString().replace(comma, semicolon));
  m_leSuffixes->setText(Config::nameSuffixesString().replace(comma, semicolon));
  m_lePrefixes->setText(Config::surnamePrefixesString().replace(comma, semicolon));
}

void ConfigDialog::readPrintingConfig() {
  m_cbPrintHeaders->setChecked(Config::printFieldHeaders());
  m_cbPrintFormatted->setChecked(Config::printFormatted());
  m_cbPrintGrouped->setChecked(Config::printGrouped());
  m_imageWidthBox->setValue(Config::maxImageWidth());
  m_imageHeightBox->setValue(Config::maxImageHeight());
}

void ConfigDialog::readTemplateConfig() {
  // entry template selection
  const int collType = Kernel::self()->collectionType();
  QString file = Config::templateName(collType);
  file.replace('_', ' ');
  QString fileContext = file + QString::fromLatin1(" XSL Template");
  m_templateCombo->setCurrentItem(i18n(fileContext.utf8(), file.utf8()));

  m_fontCombo->setCurrentFont(Config::templateFont(collType).family());
  m_fontSizeInput->setValue(Config::templateFont(collType).pointSize());
  m_baseColorCombo->setColor(Config::templateBaseColor(collType));
  m_textColorCombo->setColor(Config::templateTextColor(collType));
  m_highBaseColorCombo->setColor(Config::templateHighlightedBaseColor(collType));
  m_highTextColorCombo->setColor(Config::templateHighlightedTextColor(collType));
}

void ConfigDialog::readFetchConfig() {
  m_sourceListView->clear();
  m_configWidgets.clear();

  Fetch::FetcherVec fetchers = Fetch::Manager::self()->fetchers();
  for(Fetch::FetcherVec::Iterator it = fetchers.begin(); it != fetchers.end(); ++it) {
    GeneralFetcherInfo info(it->type(), it->source(), it->updateOverwrite());
    SourceListViewItem* item = new SourceListViewItem(m_sourceListView, m_sourceListView->lastItem(), info);
    item->setFetcher(it.data());
    // grab the config widget, taking ownership
    Fetch::ConfigWidget* cw = it->configWidget(this);
    if(cw) { // might return 0 when no widget available for fetcher type
      m_configWidgets.insert(item, cw);
      // there's weird layout bug if it's not hidden
      cw->hide();
    }
  }

  if(m_sourceListView->childCount() == 0) {
    m_modifySourceBtn->setEnabled(false);
    m_removeSourceBtn->setEnabled(false);
  } else {
    // go ahead and select the first one
    m_sourceListView->setSelected(m_sourceListView->firstChild(), true);
  }
}

void ConfigDialog::saveConfiguration() {
  Config::setShowTipOfDay(m_cbShowTipDay->isChecked());

  Config::setWriteImagesInFile(m_cbWriteImagesInFile->isChecked());
  Config::setReopenLastFile(m_cbOpenLastFile->isChecked());

  Config::setAutoCapitalization(m_cbCapitalize->isChecked());
  Config::setAutoFormat(m_cbFormat->isChecked());

  const QRegExp semicolon(QString::fromLatin1("\\s*;\\s*"));
  const QChar comma = ',';

  Config::setNoCapitalizationString(m_leCapitals->text().replace(semicolon, comma));
  Config::setArticlesString(m_leArticles->text().replace(semicolon, comma));
  Data::Field::articlesUpdated();
  Config::setNameSuffixesString(m_leSuffixes->text().replace(semicolon, comma));
  Config::setSurnamePrefixesString(m_lePrefixes->text().replace(semicolon, comma));

  Config::setPrintFieldHeaders(m_cbPrintHeaders->isChecked());
  Config::setPrintFormatted(m_cbPrintFormatted->isChecked());
  Config::setPrintGrouped(m_cbPrintGrouped->isChecked());
  Config::setMaxImageWidth(m_imageWidthBox->value());
  Config::setMaxImageHeight(m_imageHeightBox->value());

  // entry template selection
  const int collType = Kernel::self()->collectionType();
  Config::setTemplateName(collType, m_templateCombo->currentData().toString());
  QFont font(m_fontCombo->currentFont(), m_fontSizeInput->value());
  Config::setTemplateFont(collType, font);
  Config::setTemplateBaseColor(collType, m_baseColorCombo->color());
  Config::setTemplateTextColor(collType, m_textColorCombo->color());
  Config::setTemplateHighlightedBaseColor(collType, m_highBaseColorCombo->color());
  Config::setTemplateHighlightedTextColor(collType, m_highTextColorCombo->color());

  // first, tell config widgets they got deleted
  for(QPtrListIterator<Fetch::ConfigWidget> it(m_removedConfigWidgets); it.current(); ++it) {
    it.current()->removed();
  }
  m_removedConfigWidgets.clear();

  KConfig* config = KGlobal::config();

  bool reloadFetchers = false;
  int count = 0; // start group numbering at 0
  for(QListViewItemIterator it(m_sourceListView); it.current(); ++it, ++count) {
    SourceListViewItem* item = static_cast<SourceListViewItem*>(it.current());
    Fetch::ConfigWidget* cw = m_configWidgets[item];
    if(!cw || (!cw->shouldSave() && !item->isNewSource())) {
      continue;
    }
    m_newStuffConfigWidgets.removeRef(cw);
    QString group = QString::fromLatin1("Data Source %1").arg(count);
    // in case we later change the order, clear the group now
    config->deleteGroup(group);
    KConfigGroupSaver saver(config, group);
    config->writeEntry("Name", item->text(0));
    config->writeEntry("Type", item->fetchType());
    config->writeEntry("UpdateOverwrite", item->updateOverwrite());
    cw->saveConfig(config);
    item->setNewSource(false);
    // in case the ordering changed
    item->setConfigGroup(group);
    reloadFetchers = true;
  }
  // now update total number of sources
  config->setGroup("Data Sources");
  config->writeEntry("Sources Count", count);
  // and purge old config groups
  QString group = QString::fromLatin1("Data Source %1").arg(count);
  while(config->hasGroup(group)) {
    config->deleteGroup(group);
    ++count;
    group = QString::fromLatin1("Data Source %1").arg(count);
  }

  config->sync();
  Config::writeConfig();

  QString s = m_sourceListView->selectedItem() ? m_sourceListView->selectedItem()->text(0) : QString();
  if(reloadFetchers) {
    Fetch::Manager::self()->loadFetchers();
    Controller::self()->updatedFetchers();
    // reload fetcher items
    readFetchConfig();
    if(!s.isEmpty()) {
      for(QListViewItemIterator it(m_sourceListView); it.current(); ++it) {
        if(it.current()->text(0) == s) {
          m_sourceListView->setSelected(it.current(), true);
          m_sourceListView->ensureItemVisible(it.current());
          break;
        }
      }
    }
  }
}

void ConfigDialog::slotToggleCapitalized(bool checked_) {
  m_leCapitals->setEnabled(checked_);
}

void ConfigDialog::slotToggleFormatted(bool checked_) {
  m_leArticles->setEnabled(checked_);
  m_leSuffixes->setEnabled(checked_);
  m_lePrefixes->setEnabled(checked_);
}

void ConfigDialog::slotModified() {
  if(m_modifying) {
    return;
  }
  enableButtonOK(true);
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
  SourceListViewItem* item = new SourceListViewItem(m_sourceListView, m_sourceListView->lastItem(), info);
  m_sourceListView->ensureItemVisible(item);
  m_sourceListView->setSelected(item, true);
  Fetch::ConfigWidget* cw = dlg.configWidget();
  if(cw) {
    cw->setAccepted(true);
    cw->slotSetModified();
    cw->reparent(this, QPoint()); // keep the config widget around
    m_configWidgets.insert(item, cw);
  }
  m_modifySourceBtn->setEnabled(true);
  m_removeSourceBtn->setEnabled(true);
  slotModified(); // toggle apply button
}

void ConfigDialog::slotModifySourceClicked() {
  SourceListViewItem* item = static_cast<SourceListViewItem*>(m_sourceListView->selectedItem());
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
    myDebug() << "ConfigDialog::slotModifySourceClicked() - no config widget for source " << item->text(0) << endl;
    return;
  }
  FetcherConfigDialog dlg(item->text(0), item->fetchType(), item->updateOverwrite(), cw, this);

  if(dlg.exec() == QDialog::Accepted) {
    cw->setAccepted(true); // mark to save
    QString newName = dlg.sourceName();
    if(newName != item->text(0)) {
      item->setText(0, newName);
      cw->slotSetModified();
    }
    item->setUpdateOverwrite(dlg.updateOverwrite());
    slotModified(); // toggle apply button
  }
  cw->reparent(this, QPoint()); // keep the config widget around
}

void ConfigDialog::slotRemoveSourceClicked() {
  SourceListViewItem* item = static_cast<SourceListViewItem*>(m_sourceListView->selectedItem());
  if(!item) {
    return;
  }

  Fetch::ConfigWidget* cw = m_configWidgets[item];
  if(cw) {
    m_removedConfigWidgets.append(cw);
    // it gets deleted by the parent
  }
  m_configWidgets.remove(item);
  delete item;
  m_sourceListView->setSelected(m_sourceListView->currentItem(), true);
  slotModified(); // toggle apply button
}

void ConfigDialog::slotMoveUpSourceClicked() {
  QListViewItem* item = m_sourceListView->selectedItem();
  if(!item) {
    return;
  }
  SourceListViewItem* prev = static_cast<SourceListViewItem*>(item->itemAbove()); // could be 0
  if(prev) {
    GeneralFetcherInfo info(prev->fetchType(), prev->text(0), prev->updateOverwrite());
    SourceListViewItem* newItem = new SourceListViewItem(m_sourceListView, item, info, prev->configGroup());
    newItem->setFetcher(prev->fetcher());
    Fetch::ConfigWidget* cw = m_configWidgets[prev];
    m_configWidgets.remove(prev);
    m_configWidgets.insert(newItem, cw);
    delete prev;
    slotModified(); // toggle apply button
    slotSelectedSourceChanged(item);
  }
}

void ConfigDialog::slotMoveDownSourceClicked() {
  SourceListViewItem* item = static_cast<SourceListViewItem*>(m_sourceListView->selectedItem());
  if(!item) {
    return;
  }
  QListViewItem* next = item->nextSibling(); // could be 0
  if(next) {
    GeneralFetcherInfo info(item->fetchType(), item->text(0), item->updateOverwrite());
    SourceListViewItem* newItem = new SourceListViewItem(m_sourceListView, next, info, item->configGroup());
    newItem->setFetcher(item->fetcher());
    Fetch::ConfigWidget* cw = m_configWidgets[item];
    m_configWidgets.remove(item);
    m_configWidgets.insert(newItem, cw);
    delete item;
    slotModified(); // toggle apply button
    m_sourceListView->setSelected(newItem, true);
  }
}

void ConfigDialog::slotSelectedSourceChanged(QListViewItem* item_) {
  m_moveUpSourceBtn->setEnabled(item_ && item_->itemAbove());
  m_moveDownSourceBtn->setEnabled(item_ && item_->nextSibling());
}

void ConfigDialog::slotNewStuffClicked() {
  NewStuff::Dialog dlg(NewStuff::DataScript, this);
  dlg.exec();

  QPtrList<NewStuff::DataSourceInfo> infoList = dlg.dataSourceInfo();
  for(QPtrListIterator<NewStuff::DataSourceInfo> it(infoList); it.current(); ++it) {
    const NewStuff::DataSourceInfo& info = *it.current();
    Fetch::ExecExternalFetcher::ConfigWidget* cw = 0;
    SourceListViewItem* item = 0;

    // yes, this is checking if item exists
    if(info.isUpdate && (item = findItem(info.sourceExec))) {
      m_sourceListView->setSelected(item, true);
      cw = dynamic_cast<Fetch::ExecExternalFetcher::ConfigWidget*>(m_configWidgets[item]);
    } else {
      cw = new Fetch::ExecExternalFetcher::ConfigWidget(this);
      m_newStuffConfigWidgets.append(cw);

      GeneralFetcherInfo fetchInfo(Fetch::ExecExternal, info.sourceName, false);
      item = new SourceListViewItem(m_sourceListView, m_sourceListView->lastItem(), fetchInfo);
      m_configWidgets.insert(item, cw);
    }

    if(!cw) {
      continue;
    }

    KConfig spec(info.specFile, false, false);
    cw->readConfig(&spec);
    cw->slotSetModified();
    cw->setAccepted(true);

    if(item) {
      m_sourceListView->setSelected(item, true);
      m_sourceListView->ensureItemVisible(item);
    }
  }

  if(infoList.count() > 0) {
    m_modifySourceBtn->setEnabled(true);
    m_removeSourceBtn->setEnabled(true);
    slotModified(); // toggle apply button
  }
}

Tellico::SourceListViewItem* ConfigDialog::findItem(const QString& path_) const {
  if(path_.isEmpty()) {
    kdWarning() << "ConfigDialog::findItem() - empty path" << endl;
    return 0;
  }

  // this is a bit ugly, loop over all items, find the execexternal one
  // that matches the path
  for(QListViewItemIterator it(m_sourceListView); it.current(); ++it) {
    SourceListViewItem* item = static_cast<SourceListViewItem*>(it.current());
    if(item->fetchType() != Fetch::ExecExternal) {
      continue;
    }
    Fetch::ExecExternalFetcher* f = dynamic_cast<Fetch::ExecExternalFetcher*>(item->fetcher().data());
    if(f && f->execPath() == path_) {
      return item;
    }
  }
  myDebug() << "ConfigDialog::findItem() - no matching item found" << endl;
  return 0;
}

void ConfigDialog::slotShowTemplatePreview() {
  GUI::PreviewDialog* dlg = new GUI::PreviewDialog(this);

  const QString templateName = m_templateCombo->currentData().toString();
  dlg->setXSLTFile(templateName + QString::fromLatin1(".xsl"));

  StyleOptions options;
  options.fontFamily = m_fontCombo->currentFont();
  options.fontSize   = m_fontSizeInput->value();
  options.baseColor  = m_baseColorCombo->color();
  options.textColor  = m_textColorCombo->color();
  options.highlightedTextColor = m_highTextColorCombo->color();
  options.highlightedBaseColor = m_highBaseColorCombo->color();
  dlg->setXSLTOptions(options);

  Data::CollPtr c = CollectionFactory::collection(Kernel::self()->collectionType(), true);
  Data::EntryPtr e = new Data::Entry(c);
  for(Data::FieldVec::ConstIterator f = c->fields().begin(); f != c->fields().end(); ++f) {
    if(f->name() == Latin1Literal("title")) {
      e->setField(f->name(), m_templateCombo->currentText());
    } else if(f->type() == Data::Field::Image) {
      continue;
    } else if(f->type() == Data::Field::Choice) {
      e->setField(f->name(), f->allowed().front());
    } else if(f->type() == Data::Field::Number) {
      e->setField(f->name(), QString::fromLatin1("1"));
    } else if(f->type() == Data::Field::Bool) {
      e->setField(f->name(), QString::fromLatin1("true"));
    } else if(f->type() == Data::Field::Rating) {
      e->setField(f->name(), QString::fromLatin1("5"));
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
  QStringList files = KGlobal::dirs()->findAllResources("appdata", QString::fromLatin1("entry-templates/*.xsl"),
                                                        false, true);
  KSortableValueList<QString, QString> templates;
  for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
    QFileInfo fi(*it);
    QString file = fi.fileName().section('.', 0, -2);
    QString name = file;
    name.replace('_', ' ');
    QString title = i18n((name + QString::fromLatin1(" XSL Template")).utf8(), name.utf8());
    templates.insert(title, file);
  }
  templates.sort();

  QString s = m_templateCombo->currentText();
  m_templateCombo->clear();
  for(KSortableValueList<QString, QString>::iterator it2 = templates.begin(); it2 != templates.end(); ++it2) {
    m_templateCombo->insertItem((*it2).index(), (*it2).value());
  }
  m_templateCombo->setCurrentItem(s);
}

void ConfigDialog::slotInstallTemplate() {
  QString filter = i18n("*.xsl|XSL Files (*.xsl)") + '\n';
  filter += i18n("*.tar.gz *.tgz|Template Packages (*.tar.gz)") + '\n';
  filter += i18n("*|All Files");

  KURL u = KFileDialog::getOpenURL(QString::null, filter, this);
  if(u.isEmpty() || !u.isValid()) {
    return;
  }

  NewStuff::Manager man(this);
  if(man.installTemplate(u)) {
    loadTemplateList();
  }
}

void ConfigDialog::slotDownloadTemplate() {
  NewStuff::Dialog dlg(NewStuff::EntryTemplate, this);
  dlg.exec();
  loadTemplateList();
}

void ConfigDialog::slotDeleteTemplate() {
  QDir dir(Tellico::saveLocation(QString::fromLatin1("entry-templates/")));
  dir.setNameFilter(QString::fromLatin1("*.xsl"));
  dir.setFilter(QDir::Files | QDir::Writable);
  QStringList files = dir.entryList();
  QMap<QString, QString> nameFileMap;
  for(QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
    (*it).truncate((*it).length()-4); // remove ".xsl"
    QString name = (*it);
    name.replace('_', ' ');
    nameFileMap.insert(name, *it);
  }
  bool ok;
  QString name = KInputDialog::getItem(i18n("Delete Template"),
                                       i18n("Select template to delete:"),
                                       nameFileMap.keys(), 0, false, &ok, this);
  if(ok && !name.isEmpty()) {
    QString file = nameFileMap[name];
    NewStuff::Manager man(this);
    man.removeTemplate(file);
    loadTemplateList();
  }
}

#include "configdialog.moc"
