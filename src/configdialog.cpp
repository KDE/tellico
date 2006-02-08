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
#include "collectionfactory.h"
#include "fetch/fetcher.h"
#include "fetch/fetchmanager.h"
#include "fetch/configwidget.h"
#include "controller.h"
#include "fetcherconfigdialog.h"
#include "tellico_kernel.h"

#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kdialogbase.h>
#include <knuminput.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <ksortablevaluelist.h>
#include <kaccelmanager.h>

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

using Tellico::ConfigDialog;

ConfigDialog::ConfigDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(IconList, i18n("Configure Tellico"), Help|Ok|Apply|Cancel|Default,
                  Ok, parent_, name_, true, false), m_modifying(false) {
  setupGeneralPage();
  setupPrintingPage();
  setupTemplatePage();
  setupFetchPage();

  // the proxies do not auto-delete since they're not QObjects
  m_cbTemplates.setAutoDelete(true);

  updateGeometry();
  QSize s = sizeHint();
  resize(KMAX(s.width(), CONFIG_MIN_WIDTH), KMAX(s.height(), CONFIG_MIN_HEIGHT));

  enableButtonOK(false);
  enableButtonApply(false);

  setHelp(QString::fromLatin1("general-options"));
  connect(this, SIGNAL(aboutToShowPage(QWidget*)), SLOT(slotUpdateHelpLink(QWidget*)));
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
  switch(activePageIndex()) {
    case 0:
      m_cbOpenLastFile->setChecked(true);
      m_cbShowTipDay->setChecked(true);
      m_cbCapitalize->setChecked(true);
      m_cbFormat->setChecked(true);
      m_leCapitals->setText(Data::Field::defaultNoCapitalizationList().join(QString::fromLatin1("; ")));
      m_leArticles->setText(Data::Field::defaultArticleList().join(QString::fromLatin1("; ")));
      m_leSuffixes->setText(Data::Field::defaultSuffixList().join(QString::fromLatin1("; ")));
      m_lePrefixes->setText(Data::Field::defaultSurnamePrefixList().join(QString::fromLatin1("; ")));
      break;

    case 1:
      m_cbPrintHeaders->setChecked(true);
      m_cbPrintFormatted->setChecked(true);
      m_cbPrintGrouped->setChecked(false);
      m_imageWidthBox->setValue(50);
      m_imageHeightBox->setValue(50);
      break;

    case 2:
      // entry template selection
      for(QIntDictIterator<CBProxy> it(m_cbTemplates); it.current(); ++it) {
        Data::Collection::Type type = static_cast<Data::Collection::Type>(it.currentKey());
        if(type == Data::Collection::Album) {
          it.current()->setCurrentItem(i18n("Album XSL Template", "Album"));
        } else if(type == Data::Collection::Video) {
          it.current()->setCurrentItem(i18n("Video XSL Template", "Video"));
        } else {
          it.current()->setCurrentItem(i18n("Fancy XSL Template", "Fancy"));
        }
      }
      break;
  }
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
  QStringList words = Data::Field::noCapitalizeList();
  if(!words.isEmpty()) {
    m_leCapitals->setText(words.join(QString::fromLatin1("; ")));
  }
  lab->setBuddy(m_leCapitals);

  QString whats = i18n("<qt>A list of words which should not be capitalized. Multiple values "
                       "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_leCapitals, whats);
  connect(m_leCapitals, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  lab = new QLabel(i18n("Artic&les:"), g1);
  m_leArticles = new KLineEdit(g1);
  lab->setBuddy(m_leArticles);
  QStringList articles = Data::Field::articleList();
  if(!articles.isEmpty()) {
    m_leArticles->setText(articles.join(QString::fromLatin1("; ")));
  }

  whats = i18n("<qt>A list of words which should be considered as articles "
               "if they are the first word in a title. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_leArticles, whats);
  connect(m_leArticles, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  QStringList suffixes = Data::Field::suffixList();
  lab = new QLabel(i18n("Personal suffi&xes:"), g1);
  m_leSuffixes = new KLineEdit(g1);
  lab->setBuddy(m_leSuffixes);
  if(!suffixes.isEmpty()) {
    m_leSuffixes->setText(suffixes.join(QString::fromLatin1("; ")));
  }
  whats = i18n("<qt>A list of suffixes which might be used in personal names. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_leSuffixes, whats);
  connect(m_leSuffixes, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  QStringList prefixes = Data::Field::surnamePrefixList();
  lab = new QLabel(i18n("Surname &prefixes:"), g1);
  m_lePrefixes = new KLineEdit(g1);
  lab->setBuddy(m_lePrefixes);
  if(!prefixes.isEmpty()) {
    m_lePrefixes->setText(prefixes.join(QString::fromLatin1("; ")));
  }
  whats = i18n("<qt>A list of prefixes which might be used in surnames. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(lab, whats);
  QWhatsThis::add(m_lePrefixes, whats);
  connect(m_lePrefixes, SIGNAL(textChanged(const QString&)), SLOT(slotModified()));

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupPrintingPage() {
  QPixmap pix = DesktopIcon(QString::fromLatin1("print_printer"), KIcon::SizeMedium);
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

  QGrid* grid = new QGrid(2, frame);
  grid->setSpacing(5);
  l->addWidget(grid);

  const CollectionNameMap nameMap = CollectionFactory::nameMap();
  for(CollectionNameMap::ConstIterator it = nameMap.begin(); it != nameMap.end(); ++it) {
    QLabel* lab = new QLabel(i18n("Edit Label", "%1:").arg(it.data()), grid);
    CBProxy* cb = new CBProxy(grid);
    for(KSortableValueList<QString, QString>::iterator it2 = templates.begin(); it2 != templates.end(); ++it2) {
      cb->insertItem((*it2).index(), (*it2).value());
    }
    m_cbTemplates.insert(it.key(), cb);
    connect(cb->comboBox(), SIGNAL(activated(int)), SLOT(slotModified()));
    lab->setBuddy(cb->comboBox());
  }

  // stretch to fill lower area
  l->addStretch(1);

  KAcceleratorManager::manage(frame);
}

void ConfigDialog::setupFetchPage() {
  QPixmap pix = DesktopIcon(QString::fromLatin1("network"), KIcon::SizeMedium);
  QFrame* frame = addPage(i18n("Data Sources"), i18n("Data Source Options"), pix);
  QHBoxLayout* l = new QHBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  m_sourceListView = new KListView(frame);
  m_sourceListView->addColumn(i18n("Source"));
  m_sourceListView->setResizeMode(QListView::LastColumn);
  m_sourceListView->setSorting(-1); // no sorting
  m_sourceListView->setSelectionMode(QListView::Single);
  l->addWidget(m_sourceListView, 1);
  connect(m_sourceListView, SIGNAL(selectionChanged(QListViewItem*)), SLOT(slotSelectedSourceChanged(QListViewItem*)));
  connect(m_sourceListView, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)), SLOT(slotModifySourceClicked()));

  // these icons are rather arbitrary, but seem to vaguely fit
  QVBoxLayout* vlay = new QVBoxLayout(l);
  KPushButton* newSourceBtn = new KPushButton(i18n("&New..."), frame);
  newSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("knewstuff")));
  m_modifySourceBtn = new KPushButton(i18n("&Modify..."), frame);
  m_modifySourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("network")));
  m_moveUpSourceBtn = new KPushButton(i18n("Move &Up"), frame);
  m_moveUpSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("up")));
  m_moveDownSourceBtn = new KPushButton(i18n("Move &Down"), frame);
  m_moveDownSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("down")));
  m_removeSourceBtn = new KPushButton(i18n("Remo&ve"), frame);
  m_removeSourceBtn->setIconSet(SmallIconSet(QString::fromLatin1("remove")));
  vlay->addWidget(newSourceBtn);
  vlay->addWidget(m_modifySourceBtn);
  vlay->addWidget(m_moveUpSourceBtn);
  vlay->addWidget(m_moveDownSourceBtn);
  vlay->addWidget(m_removeSourceBtn);
  vlay->addStretch(1);

  connect(newSourceBtn, SIGNAL(clicked()), SLOT(slotNewSourceClicked()));
  connect(m_modifySourceBtn, SIGNAL(clicked()), SLOT(slotModifySourceClicked()));
  connect(m_moveUpSourceBtn, SIGNAL(clicked()), SLOT(slotMoveUpSourceClicked()));
  connect(m_moveDownSourceBtn, SIGNAL(clicked()), SLOT(slotMoveDownSourceClicked()));
  connect(m_removeSourceBtn, SIGNAL(clicked()), SLOT(slotRemoveSourceClicked()));
}

void ConfigDialog::readConfiguration(KConfig* config_) {
  m_modifying = true;

  config_->setGroup("TipOfDay");
  bool showTipDay = config_->readBoolEntry("RunOnStart", true);
  m_cbShowTipDay->setChecked(showTipDay);

  config_->setGroup("General Options");

  bool writeImagesInFile = config_->readBoolEntry("Write Images In File", true);
  m_cbWriteImagesInFile->setChecked(writeImagesInFile);

  bool openLastFile = config_->readBoolEntry("Reopen Last File", true);
  m_cbOpenLastFile->setChecked(openLastFile);

  bool autoCapitals = config_->readBoolEntry("Auto Capitalization", true);
  m_cbCapitalize->setChecked(autoCapitals);
  slotToggleCapitalized(autoCapitals);

  bool autoFormat = config_->readBoolEntry("Auto Format", true);
  m_cbFormat->setChecked(autoFormat);
  slotToggleFormatted(autoFormat);

  // PRINTING
  config_->setGroup("Printing");

  bool printHeaders = config_->readBoolEntry("Print Field Headers", true);
  m_cbPrintHeaders->setChecked(printHeaders);

  bool printFormatted = config_->readBoolEntry("Print Formatted", true);
  m_cbPrintFormatted->setChecked(printFormatted);

  bool printGrouped = config_->readBoolEntry("Print Grouped", false);
  m_cbPrintGrouped->setChecked(printGrouped);

  int imageWidth = config_->readNumEntry("Max Image Width", 50);
  m_imageWidthBox->setValue(imageWidth);

  int imageHeight = config_->readNumEntry("Max Image Height", 50);
  m_imageHeightBox->setValue(imageHeight);

  // entry template selection
  for(QIntDictIterator<CBProxy> it(m_cbTemplates); it.current(); ++it) {
    QString typeName = CollectionFactory::typeName(static_cast<Data::Collection::Type>(it.currentKey()));
    config_->setGroup(QString::fromLatin1("Options - %1").arg(typeName));
    QString file = config_->readEntry("Entry Template", QString::fromLatin1("Fancy"));
    file.replace('_', ' ');
    // added by prepare_i18n_xslt script
    it.current()->setCurrentItem(i18n((file + QString::fromLatin1(" XSL Template")).utf8(), file.utf8()));
  }

  loadFetcherConfig();

  m_modifying = false;
}

void ConfigDialog::saveConfiguration(KConfig* config_) {
  config_->setGroup("TipOfDay");
  config_->writeEntry("RunOnStart", m_cbShowTipDay->isChecked());

  config_->setGroup("General Options");
  // write images in file is special, I only want to save the preference if the
  // current setting is not the default or if the config key already exists in the data file
  bool imagesInFile = m_cbWriteImagesInFile->isChecked();
  // default is true
  if(!imagesInFile || config_->hasKey("Write Images In File")) {
    config_->writeEntry("Write Images In File", imagesInFile);
    Kernel::self()->setWriteImagesInFile(imagesInFile);
  }
  config_->writeEntry("Reopen Last File", m_cbOpenLastFile->isChecked());

  bool autoCapitals = m_cbCapitalize->isChecked();
  config_->writeEntry("Auto Capitalization", autoCapitals);
  Data::Field::setAutoCapitalize(autoCapitals);

  bool autoFormat = m_cbFormat->isChecked();
  config_->writeEntry("Auto Format", autoFormat);
  Data::Field::setAutoFormat(autoFormat);

  const QRegExp sepSpace(QString::fromLatin1("\\s*;\\s*"));
  const QChar sep = ';';

  QString wordsStr = m_leCapitals->text().replace(sepSpace, sep);
  QStringList words = QStringList::split(sep, wordsStr, false);
// ok to be empty
// still use a comma to write list
  config_->writeEntry("No Capitalization", words, ',');
  Data::Field::setNoCapitalizeList(words);

  QString articlesStr = m_leArticles->text().replace(sepSpace, sep);
  QStringList articles = QStringList::split(sep, articlesStr, false);
// ok for articles to be empty
// still use a comma to write list
  config_->writeEntry("Articles", articles, ',');
  Data::Field::setArticleList(articles);

  QString suffixesStr = m_leSuffixes->text().replace(sepSpace, sep);
  QStringList suffixes = QStringList::split(sep, suffixesStr, false);
// ok to be empty
// still use a comma to write list
  config_->writeEntry("Name Suffixes", suffixes, ',');
  Data::Field::setSuffixList(suffixes);

  QString prefixesStr = m_lePrefixes->text().replace(sepSpace, sep);
  QStringList prefixes = QStringList::split(sep, prefixesStr, false);
// still use a comma to write list
  config_->writeEntry("Surname Prefixes", prefixes, ',');
  Data::Field::setSurnamePrefixList(prefixes);

  config_->setGroup("Printing");
  config_->writeEntry("Print Field Headers", m_cbPrintHeaders->isChecked());
  config_->writeEntry("Print Formatted", m_cbPrintFormatted->isChecked());
  config_->writeEntry("Print Grouped", m_cbPrintGrouped->isChecked());
  config_->writeEntry("Max Image Width", m_imageWidthBox->value());
  config_->writeEntry("Max Image Height", m_imageHeightBox->value());

  // entry template selection
  for(QIntDictIterator<CBProxy> it(m_cbTemplates); it.current(); ++it) {
    QString typeName = CollectionFactory::typeName(static_cast<Data::Collection::Type>(it.currentKey()));
    config_->setGroup(QString::fromLatin1("Options - %1").arg(typeName));
    config_->writeEntry("Entry Template", it.current()->currentData());
  }

  bool reloadFetchers = false;
  int count = 0; // start group numbering at 0
  for(QListViewItemIterator it(m_sourceListView); it.current(); ++it, ++count) {
    SourceListViewItem* item = static_cast<SourceListViewItem*>(it.current());
    Fetch::ConfigWidget* cw = m_configWidgets[item];
    if(!cw || (!cw->shouldSave() && !item->isNewSource())) {
      continue;
    }
    QString group = QString::fromLatin1("Data Source %1").arg(count);
    // in case we later change the order, clear the group now
    config_->deleteGroup(group);
    KConfigGroupSaver saver(config_, group);
    config_->writeEntry("Name", item->text(0));
    config_->writeEntry("Type", item->fetchType());
    cw->saveConfig(config_);
    item->setNewSource(false);
    // in case the ordering changed
    item->setConfigGroup(group);
    reloadFetchers = true;
  }
  // now update total number of sources
  config_->setGroup("Data Sources");
  config_->writeEntry("Sources Count", count);
  // and purge old config groups
  QString group = QString::fromLatin1("Data Source %1").arg(count);
  while(config_->hasGroup(group)) {
    config_->deleteGroup(group);
    ++count;
    group = QString::fromLatin1("Data Source %1").arg(count);
  }

  config_->sync();

  if(reloadFetchers) {
    Fetch::Manager::self()->loadFetchers();
    Controller::self()->updatedFetchers();
    // reload fetcher items
    loadFetcherConfig();
  }
}

void ConfigDialog::loadFetcherConfig() {
  m_sourceListView->clear();
  m_configWidgets.clear();

  Fetch::FetcherVec fetchers = Fetch::Manager::self()->fetchers();
  for(Fetch::FetcherVec::ConstIterator it = fetchers.constBegin(); it != fetchers.constEnd(); ++it) {
    SourceListViewItem* item = new SourceListViewItem(m_sourceListView,  m_sourceListView->lastItem(),
                                                      it->source(), it->type());
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

  SourceListViewItem* item = new SourceListViewItem(m_sourceListView, m_sourceListView->lastItem(),
                                                    dlg.sourceName(), type);
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

// there's a lot of duplicated code between here and slotNewSourceClicked()
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
  FetcherConfigDialog dlg(item->text(0), item->fetchType(), cw, this);

  if(dlg.exec() == QDialog::Accepted) {
    cw->setAccepted(true); // mark to save
    QString newName = dlg.sourceName();
    if(newName != item->text(0)) {
      item->setText(0, newName);
      cw->slotSetModified();
    }
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
  m_configWidgets.remove(item);
  delete item;
  delete cw;
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
    SourceListViewItem* newItem = new SourceListViewItem(m_sourceListView, item, prev->text(0),
                                                          prev->fetchType(), prev->configGroup());
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
    SourceListViewItem* newItem = new SourceListViewItem(m_sourceListView, next, item->text(0),
                                                         item->fetchType(), item->configGroup());
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

#include "configdialog.moc"
