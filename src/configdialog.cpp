/***************************************************************************
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

#include "configdialog.h"
#include "field.h"
#include "collectionfactory.h"
#include "translators/bibtexhandler.h" // needed for bibtex quote style options

#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kdialogbase.h>
#include <knuminput.h>

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

static const int CONFIG_MIN_WIDTH = 600;
static const int CONFIG_MIN_HEIGHT = 420;

using Bookcase::ConfigDialog;

ConfigDialog::ConfigDialog(QWidget* parent_, const char* name_/*=0*/)
    : KDialogBase(IconList, i18n("Configure Bookcase"), Help|Ok|Apply|Cancel|Default,
                  Ok, parent_, name_, true, false) {
  setupGeneralPage();
  setupPrintingPage();
  setupTemplatePage();
  setupBibliographyPage();

  updateGeometry();
  QSize s = sizeHint();
  resize(QMAX(s.width(), CONFIG_MIN_WIDTH),
         QMAX(s.height(), CONFIG_MIN_HEIGHT));

  setHelp(QString::fromLatin1("general-options"));
  connect(this, SIGNAL(aboutToShowPage(QWidget*)), SLOT(slotUpdateHelpLink(QWidget*)));
}

void ConfigDialog::slotUpdateHelpLink(QWidget* w_) {
  int idx = pageIndex(w_);
  switch(idx) {
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
      setHelp(QString::fromLatin1("bibtex-options"));
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
}

void ConfigDialog::slotDefault() {
  // only change the defaults on the active page
  switch(activePageIndex()) {
    case 0:
      m_cbOpenLastFile->setChecked(true);
      m_cbShowTipDay->setChecked(true);
      m_cbCapitalize->setChecked(true);
      m_cbFormat->setChecked(true);
      m_cbShowCount->setChecked(true);
      m_leArticles->setText(Data::Field::defaultArticleList().join(QString::fromLatin1(", ")));
      m_leSuffixes->setText(Data::Field::defaultSuffixList().join(QString::fromLatin1(", ")));
      m_lePrefixes->setText(Data::Field::defaultSurnamePrefixList().join(QString::fromLatin1(", ")));
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
      for(QIntDictIterator<KComboBox> it(m_cbTemplates); it.current(); ++it) {
        // not translated since it's the file name
        it.current()->setCurrentItem(QString::fromLatin1("Default"));
      }
      break;

    case 3:
      // bibtex options
      m_cbBibtexStyle->setCurrentItem(i18n("Braces"));
      // FIXME: should use KShell::homeDir() ?
      m_leLyxpipe->setText(QString::fromLatin1("$HOME/.lyx/lyxpipe"));
      break;
  }
}

void ConfigDialog::setupGeneralPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("bookcase"), KIcon::User);
  QFrame* frame = addPage(i18n("General"), i18n("General Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  m_cbOpenLastFile = new QCheckBox(i18n("Reopen file at startup"), frame);
  QWhatsThis::add(m_cbOpenLastFile, i18n("If checked, the file that was last open "
                                         "will be re-opened at program start-up."));
  l->addWidget(m_cbOpenLastFile);

  m_cbShowTipDay = new QCheckBox(i18n("Show \"Tip of the Day\" at startup"), frame);
  QWhatsThis::add(m_cbShowTipDay, i18n("If checked, the \"Tip of the Day\" will be "
                                       "shown at program start-up."));
  l->addWidget(m_cbShowTipDay);

  m_cbShowCount = new QCheckBox(i18n("Show number of items in group"), frame);
  QWhatsThis::add(m_cbShowCount, i18n("If checked, the number of items in the group "
                                      "will be appended to the group name."));
  l->addWidget(m_cbShowCount);

  QVGroupBox* formatGroup = new QVGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(formatGroup);

  m_cbCapitalize = new QCheckBox(i18n("Auto capitalize titles and names"), formatGroup);
  QWhatsThis::add(m_cbCapitalize, i18n("If checked, titles and names will "
                                       "be automatically capitalized."));

  m_cbFormat = new QCheckBox(i18n("Auto format titles and names"), formatGroup);
  QWhatsThis::add(m_cbFormat, i18n("If checked, titles and names will "
                                   "be automatically formatted."));
  connect(m_cbFormat, SIGNAL(toggled(bool)), this, SLOT(slotToggleFormatted(bool)));

  QGrid* g1 = new QGrid(2, formatGroup);
  g1->setSpacing(5);

  QLabel* l1 = new QLabel(i18n("Articles:"), g1);
  m_leArticles = new KLineEdit(g1);
  QStringList articles = Data::Field::articleList();
  if(!articles.isEmpty()) {
    m_leArticles->setText(articles.join(QString::fromLatin1("; ")));
  }

  QString whats = i18n("<qt>A list of words which should be considered as articles "
                       "if they are the first word in a title. Multiple values "
                       "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(l1, whats);
  QWhatsThis::add(m_leArticles, whats);

  QStringList suffixes = Data::Field::suffixList();
  QLabel* l2 = new QLabel(i18n("Personal suffixes:"), g1);
  m_leSuffixes = new KLineEdit(g1);
  if(!suffixes.isEmpty()) {
    m_leSuffixes->setText(suffixes.join(QString::fromLatin1("; ")));
  }
  whats = i18n("<qt>A list of suffixes which might be used in personal names. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(l2, whats);
  QWhatsThis::add(m_leSuffixes, whats);

  QStringList prefixes = Data::Field::surnamePrefixList();
  QLabel* l3 = new QLabel(i18n("Surname prefixes:"), g1);
  m_lePrefixes = new KLineEdit(g1);
  if(!prefixes.isEmpty()) {
    m_lePrefixes->setText(prefixes.join(QString::fromLatin1("; ")));
  }
  whats = i18n("<qt>A list of prefixes which might be used in surnames. Multiple values "
               "should be separated by a semi-colon.</qt>");
  QWhatsThis::add(l3, whats);
  QWhatsThis::add(m_lePrefixes, whats);

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupPrintingPage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("print_printer"), KIcon::Desktop);
  QFrame* frame = addPage(i18n("Printing"), i18n("Printing Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  QVGroupBox* formatOptions = new QVGroupBox(i18n("Formatting Options"), frame);
  l->addWidget(formatOptions);

  m_cbPrintFormatted = new QCheckBox(i18n("Format titles and names"), formatOptions);
  QWhatsThis::add(m_cbPrintFormatted, i18n("If checked, titles and names will be automatically formatted."));

  m_cbPrintHeaders = new QCheckBox(i18n("Print field headers"), formatOptions);
  QWhatsThis::add(m_cbPrintHeaders, i18n("If checked, the field names will be printed as table headers."));

  QHGroupBox* groupOptions = new QHGroupBox(i18n("Grouping Options"), frame);
  l->addWidget(groupOptions);

  m_cbPrintGrouped = new QCheckBox(i18n("Group the entries"), groupOptions);
  QWhatsThis::add(m_cbPrintGrouped, i18n("If checked, the entries will be grouped by the selected field."));

  QVGroupBox* imageOptions = new QVGroupBox(i18n("Image Options"), frame);
  l->addWidget(imageOptions);

  QGrid* grid = new QGrid(3, imageOptions);
  grid->setSpacing(5);

  QLabel* l1 = new QLabel(i18n("Maximum Image Width:"), grid);
  m_imageWidthBox = new KIntSpinBox(0, 999, 1, 50, 10, grid);
  m_imageWidthBox->setSuffix(QString::fromLatin1(" px"));
  (void) new QWidget(grid);
  QString whats = i18n("The maximum width of the images in the printout. The aspect ration is preserved.");
  QWhatsThis::add(l1, whats);
  QWhatsThis::add(m_imageWidthBox, whats);

  QLabel* l2 = new QLabel(i18n("Maximum Image Height:"), grid);
  m_imageHeightBox = new KIntSpinBox(0, 999, 1, 50, 10, grid);
  m_imageHeightBox->setSuffix(QString::fromLatin1(" px"));
  (void) new QWidget(grid);
  whats = i18n("The maximum height of the images in the printout. The aspect ration is preserved.");
  QWhatsThis::add(l2, whats);
  QWhatsThis::add(m_imageHeightBox, whats);

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupTemplatePage() {
  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("looknfeel"), KIcon::Desktop);
  QFrame* frame = addPage(i18n("Templates"), i18n("Template Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  QStringList files = KGlobal::dirs()->findAllResources("appdata", QString::fromLatin1("entry-templates/*.xsl"),
                                                        false, true);
  QStringList templates;
  for(QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
    QFileInfo fi(*it);
    templates << fi.fileName().section('.', 0, 0);
  }
  templates.sort();

  QGrid* grid = new QGrid(2, frame);
  grid->setSpacing(5);
  l->addWidget(grid);

  CollectionNameMap nameMap = CollectionFactory::nameMap();
  for(CollectionNameMap::ConstIterator it = nameMap.begin(); it != nameMap.end(); ++it) {
    (void) new QLabel(it.data() + ':', grid);
    KComboBox* cb = new KComboBox(grid);
    cb->insertStringList(templates);
    m_cbTemplates.insert(it.key(), cb);
  }

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::setupBibliographyPage() {
  // FIXME: need a bibtex icon?
  QPixmap pix = KGlobal::iconLoader()->loadIcon(QString::fromLatin1("document"), KIcon::Desktop);
  QFrame* frame = addPage(i18n("Bibtex"), i18n("Bibtex Options"), pix);
  QVBoxLayout* l = new QVBoxLayout(frame, KDialog::marginHint(), KDialog::spacingHint());

  QGrid* grid = new QGrid(2, frame);
  grid->setSpacing(5);
  l->addWidget(grid);

  QLabel* l1 = new QLabel(i18n("Bibtex quotation style:"), grid);
  m_cbBibtexStyle = new KComboBox(grid);
  m_cbBibtexStyle->insertItem(i18n("Braces"));
  m_cbBibtexStyle->insertItem(i18n("Quotes"));
  QString whats = i18n("<qt>The quotation style used when exporting bibtex. All field values will be escaped with either "
                       " braces or quotation marks.</qt>");
  QWhatsThis::add(l1, whats);
  QWhatsThis::add(m_cbBibtexStyle, whats);
  if(BibtexHandler::s_quoteStyle == BibtexHandler::BRACES) {
    m_cbBibtexStyle->setCurrentItem(i18n("Braces"));
  } else {
    m_cbBibtexStyle->setCurrentItem(i18n("Quotes"));
  }

  QLabel* l2 = new QLabel(i18n("Path to LyX server:"), grid);
  m_leLyxpipe = new KLineEdit(grid);
  whats = i18n("<qt>The location of the LyX server for citing bibliographic entries. Also used by other "
               "applications such as Kile or Pybliographer. Do not include the trailing .in suffix.</qt>");
  QWhatsThis::add(l2, whats);
  QWhatsThis::add(m_leLyxpipe, whats);

  // stretch to fill lower area
  l->addStretch(1);
}

void ConfigDialog::readConfiguration(KConfig* config_) {
  config_->setGroup("TipOfDay");
  bool showTipDay = config_->readBoolEntry("RunOnStart", true);
  m_cbShowTipDay->setChecked(showTipDay);

  config_->setGroup("General Options");

  bool openLastFile = config_->readBoolEntry("Reopen Last File", true);
  m_cbOpenLastFile->setChecked(openLastFile);

  bool autoCapitals = config_->readBoolEntry("Auto Capitalization", true);
  m_cbCapitalize->setChecked(autoCapitals);

  bool autoFormat = config_->readBoolEntry("Auto Format", true);
  m_cbFormat->setChecked(autoFormat);
  slotToggleFormatted(autoFormat);

  bool showCount = config_->readBoolEntry("Show Group Count", true);
  m_cbShowCount->setChecked(showCount);

  // PRINTING
  config_->setGroup(QString::fromLatin1("Printing"));

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
  for(QIntDictIterator<KComboBox> it(m_cbTemplates); it.current(); ++it) {
    QString entryName = CollectionFactory::entryName(static_cast<Data::Collection::CollectionType>(it.currentKey()));
    config_->setGroup(QString::fromLatin1("Options - %1").arg(entryName));
    it.current()->setCurrentItem(config_->readEntry("Entry Template", QString::fromLatin1("Default")));
  }

  QString entryName = CollectionFactory::entryName(Data::Collection::Bibtex);
  config_->setGroup(QString::fromLatin1("Options - %1").arg(entryName));
  QString lyxpipe = config_->readPathEntry("lyxpipe", QString::fromLatin1("$HOME/.lyx/lyxpipe"));
  m_leLyxpipe->setText(lyxpipe);
}

void ConfigDialog::saveConfiguration(KConfig* config_) {
  config_->setGroup("TipOfDay");
  config_->writeEntry("RunOnStart", m_cbShowTipDay->isChecked());

  config_->setGroup("General Options");
  config_->writeEntry("Reopen Last File", m_cbOpenLastFile->isChecked());

  bool autoCapitals = m_cbCapitalize->isChecked();
  config_->writeEntry("Auto Capitalization", autoCapitals);
  Data::Field::setAutoCapitalize(autoCapitals);

  bool autoFormat = m_cbFormat->isChecked();
  config_->writeEntry("Auto Format", autoFormat);
  Data::Field::setAutoFormat(autoFormat);

  config_->writeEntry("Show Group Count", m_cbShowCount->isChecked());

  const QRegExp commaSpace = QRegExp(QString::fromLatin1("\\s*;\\s*"));
  const QString sep = QString::fromLatin1(";");
  // there might be semi-colons before or after the commas in the lineedit box
  // it was originally commas, but that was inconsistent
  QString articlesStr = m_leArticles->text().replace(commaSpace, sep);
  QStringList articles = QStringList::split(sep, articlesStr, false);
// ok for articles to be empty
// still use a comma to write list
  config_->writeEntry("Articles", articles, ',');
  Data::Field::setArticleList(articles);

  // there might be spaces before or after the commas in the lineedit box
  // it was originally commas, but that was inconsistent
  QString suffixesStr = m_leSuffixes->text().replace(commaSpace, sep);
  QStringList suffixes = QStringList::split(sep, suffixesStr, false);
// ok to be empty
// still use a comma to write list
  config_->writeEntry("Name Suffixes", suffixes, ',');
  Data::Field::setSuffixList(suffixes);

  // it was originally commas, but that was inconsistent
  QString prefixesStr = m_lePrefixes->text().replace(commaSpace, sep);
  QStringList prefixes = QStringList::split(sep, prefixesStr, false);
// still use a comma to write list
  config_->writeEntry("Surname Prefixes", prefixes, ',');
  Data::Field::setSurnamePrefixList(prefixes);

  config_->setGroup(QString::fromLatin1("Printing"));
  config_->writeEntry("Print Field Headers", m_cbPrintHeaders->isChecked());
  config_->writeEntry("Print Formatted", m_cbPrintFormatted->isChecked());
  config_->writeEntry("Print Grouped", m_cbPrintGrouped->isChecked());
  config_->writeEntry("Max Image Width", m_imageWidthBox->value());
  config_->writeEntry("Max Image Height", m_imageHeightBox->value());

  // entry template selection
  for(QIntDictIterator<KComboBox> it(m_cbTemplates); it.current(); ++it) {
    QString entryName = CollectionFactory::entryName(static_cast<Data::Collection::CollectionType>(it.currentKey()));
    config_->setGroup(QString::fromLatin1("Options - %1").arg(entryName));
    config_->writeEntry("Entry Template", it.current()->currentText());
  }

  // the groups may look odd, but the bibtex export may someday be divorced from the bibliography collection altogether
  config_->setGroup(QString::fromLatin1("ExportOptions - Bibtex"));
  bool useBraces = m_cbBibtexStyle->currentText() == i18n("Braces");
  config_->writeEntry("Use Braces", useBraces);
  if(useBraces) {
    BibtexHandler::s_quoteStyle = BibtexHandler::BRACES;
  } else {
    BibtexHandler::s_quoteStyle = BibtexHandler::QUOTES;
  }

  QString entryName = CollectionFactory::entryName(Data::Collection::Bibtex);
  config_->setGroup(QString::fromLatin1("Options - %1").arg(entryName));
  config_->writePathEntry("lyxpipe", m_leLyxpipe->text());

  config_->sync();
}

void ConfigDialog::slotToggleFormatted(bool checked_) {
  m_leArticles->setEnabled(checked_);
  m_leSuffixes->setEnabled(checked_);
  m_lePrefixes->setEnabled(checked_);
}

