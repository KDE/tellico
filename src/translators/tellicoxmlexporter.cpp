/***************************************************************************
    copyright            : (C) 2003-2006 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellicoxmlexporter.h"
#include "../collections/bibtexcollection.h"
#include "../imagefactory.h"
#include "../image.h"
#include "../controller.h" // needed for getting groupView pointer
#include "../entryitem.h"
#include "../latin1literal.h"
#include "../filehandler.h"
#include "../groupiterator.h"
#include "../tellico_utils.h"
#include "../tellico_kernel.h"
#include "../tellico_debug.h"
#include "tellico_xml.h"
#include "../document.h" // needed for sorting groups
#include "../translators/bibtexhandler.h" // needed for cleaning text

#include <klocale.h>
#include <kconfig.h>
#include <kmdcodec.h>
#include <kglobal.h>
#include <kcalendarsystem.h>

#include <qlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qdom.h>
#include <qtextcodec.h>

using Tellico::Export::TellicoXMLExporter;

TellicoXMLExporter::TellicoXMLExporter() : Exporter(),
      m_includeImages(false), m_includeGroups(false), m_widget(0) {
  setOptions(options() | Export::ExportImages); // not included by default
}

TellicoXMLExporter::TellicoXMLExporter(Data::CollPtr coll) : Exporter(coll),
      m_includeImages(false), m_includeGroups(false), m_widget(0) {
  setOptions(options() | Export::ExportImages); // not included by default
}

QString TellicoXMLExporter::formatString() const {
  return i18n("XML");
}

QString TellicoXMLExporter::fileFilter() const {
  return i18n("*.xml|XML Files (*.xml)") + QChar('\n') + i18n("*|All Files");
}

bool TellicoXMLExporter::exec() {
  QDomDocument doc = exportXML();
  if(doc.isNull()) {
    return false;
  }
  return FileHandler::writeTextURL(url(), doc.toString(),
                                   options() & ExportUTF8,
                                   options() & Export::ExportForce);
}

QDomDocument TellicoXMLExporter::exportXML() const {
  QDomImplementation impl;
  QDomDocumentType doctype = impl.createDocumentType(QString::fromLatin1("tellico"),
                                                     XML::pubTellico,
                                                     XML::dtdTellico);
  //default namespace
  const QString& ns = XML::nsTellico;

  QDomDocument dom = impl.createDocument(ns, QString::fromLatin1("tellico"), doctype);

  // root tellico element
  QDomElement root = dom.documentElement();

  QString encodeStr = QString::fromLatin1("version=\"1.0\" encoding=\"");
  if(options() & Export::ExportUTF8) {
    encodeStr += QString::fromLatin1("UTF-8");
  } else {
    encodeStr += QString::fromLatin1(QTextCodec::codecForLocale()->mimeName());
  }
  encodeStr += QChar('"');

  // createDocument creates a root node, insert the processing instruction before it
  dom.insertBefore(dom.createProcessingInstruction(QString::fromLatin1("xml"), encodeStr), root);

  root.setAttribute(QString::fromLatin1("syntaxVersion"), XML::syntaxVersion);

  exportCollectionXML(dom, root, options() & Export::ExportFormatted);

  // clear image list
  m_images.clear();

  return dom;
}

void TellicoXMLExporter::exportCollectionXML(QDomDocument& dom_, QDomElement& parent_, bool format_) const {
  Data::CollPtr coll = collection();
  if(!coll) {
    kdWarning() << "TellicoXMLExporter::exportCollectionXML() - no collection pointer!" << endl;
    return;
  }

  QDomElement collElem = dom_.createElement(QString::fromLatin1("collection"));
  collElem.setAttribute(QString::fromLatin1("type"),       coll->type());
  collElem.setAttribute(QString::fromLatin1("title"),      coll->title());

  QDomElement fieldsElem = dom_.createElement(QString::fromLatin1("fields"));
  collElem.appendChild(fieldsElem);

  Data::FieldVec fields = coll->fields();
  for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
    exportFieldXML(dom_, fieldsElem, fIt);
  }

  if(coll->type() == Data::Collection::Bibtex) {
    const Data::BibtexCollection* c = static_cast<const Data::BibtexCollection*>(coll.data());
    if(!c->preamble().isEmpty()) {
      QDomElement preElem = dom_.createElement(QString::fromLatin1("bibtex-preamble"));
      preElem.appendChild(dom_.createTextNode(c->preamble()));
      collElem.appendChild(preElem);
    }

    QDomElement macrosElem = dom_.createElement(QString::fromLatin1("macros"));
    for(StringMap::ConstIterator macroIt = c->macroList().constBegin(); macroIt != c->macroList().constEnd(); ++macroIt) {
      if(!macroIt.data().isEmpty()) {
        QDomElement macroElem = dom_.createElement(QString::fromLatin1("macro"));
        macroElem.setAttribute(QString::fromLatin1("name"), macroIt.key());
        macroElem.appendChild(dom_.createTextNode(macroIt.data()));
        macrosElem.appendChild(macroElem);
      }
    }
    if(macrosElem.childNodes().count() > 0) {
      collElem.appendChild(macrosElem);
    }
  }

  Data::EntryVec evec = entries();
  for(Data::EntryVec::Iterator entry = evec.begin(); entry != evec.end(); ++entry) {
    exportEntryXML(dom_, collElem, entry, format_);
  }

  if(!m_images.isEmpty() && (options() & Export::ExportImages)) {
    QDomElement imgsElem = dom_.createElement(QString::fromLatin1("images"));
    collElem.appendChild(imgsElem);
    const QStringList imageIds = m_images.toList();
    for(QStringList::ConstIterator it = imageIds.begin(); it != imageIds.end(); ++it) {
      exportImageXML(dom_, imgsElem, *it);
    }
  }

  if(m_includeGroups) {
    exportGroupXML(dom_, collElem);
  }

  parent_.appendChild(collElem);

  // the borrowers and filters are in the tellico object, not the collection
  if(options() & Export::ExportComplete) {
    QDomElement bElem = dom_.createElement(QString::fromLatin1("borrowers"));
    Data::BorrowerVec borrowers = coll->borrowers();
    for(Data::BorrowerVec::Iterator bIt = borrowers.begin(); bIt != borrowers.end(); ++bIt) {
      exportBorrowerXML(dom_, bElem, bIt);
    }
    if(bElem.hasChildNodes()) {
      parent_.appendChild(bElem);
    }

    QDomElement fElem = dom_.createElement(QString::fromLatin1("filters"));
    FilterVec filters = coll->filters();
    for(FilterVec::Iterator fIt = filters.begin(); fIt != filters.end(); ++fIt) {
      exportFilterXML(dom_, fElem, fIt);
    }
    if(fElem.hasChildNodes()) {
      parent_.appendChild(fElem);
    }
  }
}

void TellicoXMLExporter::exportFieldXML(QDomDocument& dom_, QDomElement& parent_, Data::FieldPtr field_) const {
  QDomElement elem = dom_.createElement(QString::fromLatin1("field"));

  elem.setAttribute(QString::fromLatin1("name"),     field_->name());
  elem.setAttribute(QString::fromLatin1("title"),    field_->title());
  elem.setAttribute(QString::fromLatin1("category"), field_->category());
  elem.setAttribute(QString::fromLatin1("type"),     field_->type());
  elem.setAttribute(QString::fromLatin1("flags"),    field_->flags());
  elem.setAttribute(QString::fromLatin1("format"),   field_->formatFlag());

  if(field_->type() == Data::Field::Choice) {
    elem.setAttribute(QString::fromLatin1("allowed"), field_->allowed().join(QString::fromLatin1(";")));
  }

  // only save description if it's not equal to title, which is the default
  // title is never empty, so this indirectly checks for empty descriptions
  if(field_->description() != field_->title()) {
    elem.setAttribute(QString::fromLatin1("description"), field_->description());
  }

  for(StringMap::ConstIterator it = field_->propertyList().begin(); it != field_->propertyList().end(); ++it) {
    if(it.data().isEmpty()) {
      continue;
    }
    QDomElement e = dom_.createElement(QString::fromLatin1("prop"));
    e.setAttribute(QString::fromLatin1("name"), it.key());
    e.appendChild(dom_.createTextNode(it.data()));
    elem.appendChild(e);
  }

  parent_.appendChild(elem);
}

void TellicoXMLExporter::exportEntryXML(QDomDocument& dom_, QDomElement& parent_, Data::EntryPtr entry_, bool format_) const {
  QDomElement entryElem = dom_.createElement(QString::fromLatin1("entry"));
  entryElem.setAttribute(QString::fromLatin1("id"), entry_->id());

  // iterate through every field for the entry
  Data::FieldVec fields = entry_->collection()->fields();
  for(Data::FieldVec::Iterator fIt = fields.begin(); fIt != fields.end(); ++fIt) {
    QString fieldName = fIt->name();

    // Date fields are special, don't format in export
    QString fieldValue = (format_ && fIt->type() != Data::Field::Date) ? entry_->formattedField(fieldName)
                                                                       : entry_->field(fieldName);
    if(options() & ExportClean) {
      BibtexHandler::cleanText(fieldValue);
    }

    // if empty, then no field element is added and just continue
    if(fieldValue.isEmpty()) {
      continue;
    }

    // optionally, verify images exist
    if(fIt->type() == Data::Field::Image && (options() & Export::ExportVerifyImages)) {
      if(!ImageFactory::validImage(fieldValue)) {
        myDebug() << "TellicoXMLExporter::exportEntryXML() - entry: " << entry_->title() << endl;
        myDebug() << "TellicoXMLExporter::exportEntryXML() - skipping image: " << fieldValue << endl;
        continue;
      }
    }

    // if multiple versions are allowed, split them into separate elements
    if(fIt->flags() & Data::Field::AllowMultiple) {
      // parent element if field contains multiple values, child of entryElem
      // who cares about grammar, just add an 's' to the name
      QDomElement parElem = dom_.createElement(fieldName + 's');
      entryElem.appendChild(parElem);

      // the space after the semi-colon is enforced when the field is set for the entry
      QStringList fields = QStringList::split(QString::fromLatin1("; "), fieldValue, true);
      for(QStringList::ConstIterator it = fields.begin(); it != fields.end(); ++it) {
        // element for field value, child of either entryElem or ParentElem
        QDomElement fieldElem = dom_.createElement(fieldName);
        // special case for multi-column tables
        int ncols = 0;
        if(fIt->type() == Data::Field::Table) {
          bool ok;
          ncols = Tellico::toUInt(fIt->property(QString::fromLatin1("columns")), &ok);
          if(!ok) {
            ncols = 1;
          }
        }
        if(ncols > 1) {
          for(int col = 0; col < ncols; ++col) {
            QDomElement elem;
            elem = dom_.createElement(QString::fromLatin1("column"));
            elem.appendChild(dom_.createTextNode((*it).section(QString::fromLatin1("::"), col, col)));
            fieldElem.appendChild(elem);
          }
        } else {
          fieldElem.appendChild(dom_.createTextNode(*it));
        }
        parElem.appendChild(fieldElem);
      }
    } else {
      QDomElement fieldElem = dom_.createElement(fieldName);
      entryElem.appendChild(fieldElem);
      // Date fields get special treatment
      if(fIt->type() == Data::Field::Date) {
        fieldElem.setAttribute(QString::fromLatin1("calendar"), KGlobal::locale()->calendar()->calendarName());
        QStringList s = QStringList::split('-', fieldValue, true);
        if(s.count() > 0 && !s[0].isEmpty()) {
          QDomElement e = dom_.createElement(QString::fromLatin1("year"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[0]));
        }
        if(s.count() > 1 && !s[1].isEmpty()) {
          QDomElement e = dom_.createElement(QString::fromLatin1("month"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[1]));
        }
        if(s.count() > 2 && !s[2].isEmpty()) {
          QDomElement e = dom_.createElement(QString::fromLatin1("day"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[2]));
        }
      } else if(fIt->type() == Data::Field::URL &&
                fIt->property(QString::fromLatin1("relative")) == Latin1Literal("true") &&
                !url().isEmpty()) {
        // if a relative URL and url() is not empty, change the value!
        KURL old_url(Kernel::self()->URL(), fieldValue);
        KURL new_url = KURL::relativeURL(url(), old_url);
        fieldElem.appendChild(dom_.createTextNode(new_url.url()));
      } else {
        fieldElem.appendChild(dom_.createTextNode(fieldValue));
      }
    }

    if(fIt->type() == Data::Field::Image) {
      // possible to have more than one entry with the same image
      // only want to include it in the output xml once
      m_images.add(fieldValue);
    }
  } // end field loop

  parent_.appendChild(entryElem);
}

void TellicoXMLExporter::exportImageXML(QDomDocument& dom_, QDomElement& parent_, const QString& id_) const {
  if(id_.isEmpty()) {
    myDebug() << "TellicoXMLExporter::exportImageXML() - empty image!" << endl;
    return;
  }
//  myDebug() << "TellicoXMLExporter::exportImageXML() - id = " << id_ << endl;

  QDomElement imgElem = dom_.createElement(QString::fromLatin1("image"));
  if(m_includeImages) {
    const Data::Image& img = ImageFactory::imageById(id_);
    if(img.isNull()) {
      return;
    }
    imgElem.setAttribute(QString::fromLatin1("format"), img.format());
    imgElem.setAttribute(QString::fromLatin1("id"),     img.id());
    imgElem.setAttribute(QString::fromLatin1("width"),  img.width());
    imgElem.setAttribute(QString::fromLatin1("height"), img.height());
    QCString imgText = KCodecs::base64Encode(img.byteArray());
    imgElem.appendChild(dom_.createTextNode(QString::fromLatin1(imgText)));
  } else {
    const Data::ImageInfo& info = ImageFactory::imageInfo(id_);
    if(info.id.isEmpty()) {
      return;
    }
    imgElem.setAttribute(QString::fromLatin1("format"), info.format);
    imgElem.setAttribute(QString::fromLatin1("id"),     info.id);
    imgElem.setAttribute(QString::fromLatin1("width"),  info.width);
    imgElem.setAttribute(QString::fromLatin1("height"), info.height);
  }
  parent_.appendChild(imgElem);
}

void TellicoXMLExporter::exportGroupXML(QDomDocument& dom_, QDomElement& parent_) const {
  Data::EntryVec vec = entries(); // need a copy for ::contains();
  bool exportAll = collection()->entries().count() == vec.count();
  // iterate over each group, which are the first children
  for(GroupIterator gIt = Controller::self()->groupIterator(); gIt.group(); ++gIt) {
    if(gIt.group()->isEmpty()) {
      continue;
    }
    QDomElement groupElem = dom_.createElement(QString::fromLatin1("group"));
    groupElem.setAttribute(QString::fromLatin1("title"), gIt.group()->groupName());
    // now iterate over all entry items in the group
    Data::EntryVec sorted = Data::Document::self()->sortEntries(*gIt.group());
    for(Data::EntryVec::Iterator eIt = sorted.begin(); eIt != sorted.end(); ++eIt) {
      if(!exportAll && !vec.contains(eIt)) {
        continue;
      }
      QDomElement entryRefElem = dom_.createElement(QString::fromLatin1("entryRef"));
      entryRefElem.setAttribute(QString::fromLatin1("id"), eIt->id());
      groupElem.appendChild(entryRefElem);
    }
    if(groupElem.hasChildNodes()) {
      parent_.appendChild(groupElem);
    }
  }
}

void TellicoXMLExporter::exportFilterXML(QDomDocument& dom_, QDomElement& parent_, FilterPtr filter_) const {
  QDomElement filterElem = dom_.createElement(QString::fromLatin1("filter"));
  filterElem.setAttribute(QString::fromLatin1("name"), filter_->name());

  QString match = (filter_->op() == Filter::MatchAll) ? QString::fromLatin1("all") : QString::fromLatin1("any");
  filterElem.setAttribute(QString::fromLatin1("match"), match);

  for(QPtrListIterator<FilterRule> it(*filter_); it.current(); ++it) {
    QDomElement ruleElem = dom_.createElement(QString::fromLatin1("rule"));
    ruleElem.setAttribute(QString::fromLatin1("field"), it.current()->fieldName());
    ruleElem.setAttribute(QString::fromLatin1("pattern"), it.current()->pattern());
    switch(it.current()->function()) {
      case FilterRule::FuncContains:
        ruleElem.setAttribute(QString::fromLatin1("function"), QString::fromLatin1("contains"));
        break;
      case FilterRule::FuncNotContains:
        ruleElem.setAttribute(QString::fromLatin1("function"), QString::fromLatin1("notcontains"));
        break;
      case FilterRule::FuncEquals:
        ruleElem.setAttribute(QString::fromLatin1("function"), QString::fromLatin1("equals"));
        break;
      case FilterRule::FuncNotEquals:
        ruleElem.setAttribute(QString::fromLatin1("function"), QString::fromLatin1("notequals"));
        break;
      case FilterRule::FuncRegExp:
        ruleElem.setAttribute(QString::fromLatin1("function"), QString::fromLatin1("regexp"));
        break;
      case FilterRule::FuncNotRegExp:
        ruleElem.setAttribute(QString::fromLatin1("function"), QString::fromLatin1("notregexp"));
        break;
      default:
        kdWarning() << "TellicoXMLExporter::exportFilterXML() - no matching rule function!" << endl;
    }
    filterElem.appendChild(ruleElem);
  }

  parent_.appendChild(filterElem);
}

void TellicoXMLExporter::exportBorrowerXML(QDomDocument& dom_, QDomElement& parent_,
                                           Data::BorrowerPtr borrower_) const {
  if(borrower_->isEmpty()) {
    return;
  }

  QDomElement bElem = dom_.createElement(QString::fromLatin1("borrower"));
  parent_.appendChild(bElem);

  bElem.setAttribute(QString::fromLatin1("name"), borrower_->name());
  bElem.setAttribute(QString::fromLatin1("uid"), borrower_->uid());

  const Data::LoanVec& loans = borrower_->loans();
  for(Data::LoanVec::ConstIterator it = loans.constBegin(); it != loans.constEnd(); ++it) {
    QDomElement lElem = dom_.createElement(QString::fromLatin1("loan"));
    bElem.appendChild(lElem);

    lElem.setAttribute(QString::fromLatin1("uid"), it->uid());
    lElem.setAttribute(QString::fromLatin1("entryRef"), it->entry()->id());
    lElem.setAttribute(QString::fromLatin1("loanDate"), it->loanDate().toString(Qt::ISODate));
    lElem.setAttribute(QString::fromLatin1("dueDate"), it->dueDate().toString(Qt::ISODate));
    if(it->inCalendar()) {
      lElem.setAttribute(QString::fromLatin1("calendar"), QString::fromLatin1("true"));
    }

    lElem.appendChild(dom_.createTextNode(it->note()));
  }
}

QWidget* TellicoXMLExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Horizontal, i18n("Tellico XML Options"), m_widget);
  l->addWidget(box);

  m_checkIncludeImages = new QCheckBox(i18n("Include images in XML document"), box);
  m_checkIncludeImages->setChecked(m_includeImages);
  QWhatsThis::add(m_checkIncludeImages, i18n("If checked, the images in the document will be included "
                                             "in the XML stream as base64 encoded elements."));

  return m_widget;
}

void TellicoXMLExporter::readOptions(KConfig* config_) {
  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_includeImages = config_->readBoolEntry("Include Images", m_includeImages);
}

void TellicoXMLExporter::saveOptions(KConfig* config_) {
  m_includeImages = m_checkIncludeImages->isChecked();

  KConfigGroupSaver group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  config_->writeEntry("Include Images", m_includeImages);
}

#include "tellicoxmlexporter.moc"
