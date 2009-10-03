/***************************************************************************
    Copyright (C) 2003-2009 Robby Stephenson <robby@periapsis.org>
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

#include "tellicoxmlexporter.h"
#include "tellico_xml.h"
#include "../entrygroup.h"
#include "../collections/bibtexcollection.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../images/imageinfo.h"
#include "../core/filehandler.h"
#include "../tellico_utils.h"
#include "../document.h"
#include "../fieldformat.h"
#include "../translators/bibtexhandler.h" // needed for cleaning text
#include "../models/entrysortmodel.h"
#include "../models/modelmanager.h"
#include "../models/modeliterator.h"
#include "../models/models.h"
#include "../tellico_debug.h"

#include <klocale.h>
#include <KConfigGroup>
#include <kcodecs.h>
#include <kglobal.h>
#include <kcalendarsystem.h>

#include <QGroupBox>
#include <QCheckBox>
#include <QDomDocument>
#include <QTextCodec>
#include <QVBoxLayout>

#include <algorithm>

using Tellico::Export::TellicoXMLExporter;

TellicoXMLExporter::TellicoXMLExporter(Tellico::Data::CollPtr coll) : Exporter(coll),
      m_includeImages(false), m_includeGroups(false), m_widget(0) {
  setOptions(options() | Export::ExportImages | Export::ExportImageSize); // not included by default
}

QString TellicoXMLExporter::formatString() const {
  return i18n("XML");
}

QString TellicoXMLExporter::fileFilter() const {
  return i18n("*.xml|XML Files (*.xml)") + QLatin1Char('\n') + i18n("*|All Files");
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
  // don't be hard on people with older versions. The only difference with DTD 10 was adding
  // a board game collection, so use 9 still unless it's a board game
  int exportVersion = (XML::syntaxVersion == 10 && collection()->type() != Data::Collection::BoardGame)
                    ? 9
                    : XML::syntaxVersion;

  QDomImplementation impl;
  QDomDocumentType doctype = impl.createDocumentType(QLatin1String("tellico"),
                                                     XML::pubTellico(exportVersion),
                                                     XML::dtdTellico(exportVersion));
  //default namespace
  const QString& ns = XML::nsTellico;

  QDomDocument dom = impl.createDocument(ns, QLatin1String("tellico"), doctype);

  // root tellico element
  QDomElement root = dom.documentElement();

  QString encodeStr = QLatin1String("version=\"1.0\" encoding=\"");
  if(options() & Export::ExportUTF8) {
    encodeStr += QLatin1String("UTF-8");
  } else {
    encodeStr += QLatin1String(QTextCodec::codecForLocale()->name());
  }
  encodeStr += QLatin1Char('"');

  // createDocument creates a root node, insert the processing instruction before it
  dom.insertBefore(dom.createProcessingInstruction(QLatin1String("xml"), encodeStr), root);

  root.setAttribute(QLatin1String("syntaxVersion"), exportVersion);

  exportCollectionXML(dom, root, options() & Export::ExportFormatted);

  // clear image list
  m_images.clear();

  return dom;
}

QString TellicoXMLExporter::exportXMLString() const {
  return exportXML().toString();
}

void TellicoXMLExporter::exportCollectionXML(QDomDocument& dom_, QDomElement& parent_, bool format_) const {
  Data::CollPtr coll = collection();
  if(!coll) {
    myWarning() << "no collection pointer!";
    return;
  }

  QDomElement collElem = dom_.createElement(QLatin1String("collection"));
  collElem.setAttribute(QLatin1String("type"),       coll->type());
  collElem.setAttribute(QLatin1String("title"),      coll->title());

  QDomElement fieldsElem = dom_.createElement(QLatin1String("fields"));
  collElem.appendChild(fieldsElem);

  foreach(Data::FieldPtr field, coll->fields()) {
    exportFieldXML(dom_, fieldsElem, field);
  }

  if(coll->type() == Data::Collection::Bibtex) {
    const Data::BibtexCollection* c = static_cast<const Data::BibtexCollection*>(coll.data());
    if(!c->preamble().isEmpty()) {
      QDomElement preElem = dom_.createElement(QLatin1String("bibtex-preamble"));
      preElem.appendChild(dom_.createTextNode(c->preamble()));
      collElem.appendChild(preElem);
    }

    QDomElement macrosElem = dom_.createElement(QLatin1String("macros"));
    for(StringMap::ConstIterator macroIt = c->macroList().constBegin(); macroIt != c->macroList().constEnd(); ++macroIt) {
      if(!macroIt.value().isEmpty()) {
        QDomElement macroElem = dom_.createElement(QLatin1String("macro"));
        macroElem.setAttribute(QLatin1String("name"), macroIt.key());
        macroElem.appendChild(dom_.createTextNode(macroIt.value()));
        macrosElem.appendChild(macroElem);
      }
    }
    if(macrosElem.childNodes().count() > 0) {
      collElem.appendChild(macrosElem);
    }
  }

  foreach(Data::EntryPtr entry, entries()) {
    exportEntryXML(dom_, collElem, entry, format_);
  }

  if(!m_images.isEmpty() && (options() & Export::ExportImages)) {
    QDomElement imgsElem = dom_.createElement(QLatin1String("images"));
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
    QDomElement bElem = dom_.createElement(QLatin1String("borrowers"));
    foreach(Data::BorrowerPtr borrower, coll->borrowers()) {
      exportBorrowerXML(dom_, bElem, borrower);
    }
    if(bElem.hasChildNodes()) {
      parent_.appendChild(bElem);
    }

    QDomElement fElem = dom_.createElement(QLatin1String("filters"));
    foreach(FilterPtr filter, coll->filters()) {
      exportFilterXML(dom_, fElem, filter);
    }
    if(fElem.hasChildNodes()) {
      parent_.appendChild(fElem);
    }
  }
}

void TellicoXMLExporter::exportFieldXML(QDomDocument& dom_, QDomElement& parent_, Tellico::Data::FieldPtr field_) const {
  QDomElement elem = dom_.createElement(QLatin1String("field"));

  elem.setAttribute(QLatin1String("name"),     field_->name());
  elem.setAttribute(QLatin1String("title"),    field_->title());
  elem.setAttribute(QLatin1String("category"), field_->category());
  elem.setAttribute(QLatin1String("type"),     field_->type());
  elem.setAttribute(QLatin1String("flags"),    field_->flags());
  elem.setAttribute(QLatin1String("format"),   field_->formatFlag());

  if(field_->type() == Data::Field::Choice) {
    elem.setAttribute(QLatin1String("allowed"), field_->allowed().join(QLatin1String(";")));
  }

  // only save description if it's not equal to title, which is the default
  // title is never empty, so this indirectly checks for empty descriptions
  if(field_->description() != field_->title()) {
    elem.setAttribute(QLatin1String("description"), field_->description());
  }

  for(StringMap::ConstIterator it = field_->propertyList().begin(); it != field_->propertyList().end(); ++it) {
    if(it.value().isEmpty()) {
      continue;
    }
    QDomElement e = dom_.createElement(QLatin1String("prop"));
    e.setAttribute(QLatin1String("name"), it.key());
    e.appendChild(dom_.createTextNode(it.value()));
    elem.appendChild(e);
  }

  parent_.appendChild(elem);
}

void TellicoXMLExporter::exportEntryXML(QDomDocument& dom_, QDomElement& parent_, Tellico::Data::EntryPtr entry_, bool format_) const {
  QDomElement entryElem = dom_.createElement(QLatin1String("entry"));
  entryElem.setAttribute(QLatin1String("id"), QString::number(entry_->id()));

  // iterate through every field for the entry
  Data::FieldList fields = entry_->collection()->fields();
  foreach(Data::FieldPtr fIt, fields) {
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
        myDebug() << "entry: " << entry_->title();
        myDebug() << "skipping image: " << fieldValue;
        continue;
      }
    }

    // if multiple versions are allowed, split them into separate elements
    if(fIt->hasFlag(Data::Field::AllowMultiple)) {
      // parent element if field contains multiple values, child of entryElem
      // who cares about grammar, just add an QLatin1Char('s') to the name
      QDomElement parElem = dom_.createElement(fieldName + QLatin1Char('s'));
      entryElem.appendChild(parElem);

      // the space after the semi-colon is enforced when the field is set for the entry
      QStringList fields = FieldFormat::splitValue(fieldValue);
      for(QStringList::ConstIterator it = fields.constBegin(); it != fields.constEnd(); ++it) {
        // element for field value, child of either entryElem or ParentElem
        QDomElement fieldElem = dom_.createElement(fieldName);
        // special case for multi-column tables
        int ncols = 0;
        if(fIt->type() == Data::Field::Table) {
          bool ok;
          ncols = Tellico::toUInt(fIt->property(QLatin1String("columns")), &ok);
          if(!ok) {
            ncols = 1;
          }
        }
        if(ncols > 1) {
          QStringList rowValues = FieldFormat::splitRow(*it);
          for(int col = 0; col < ncols; ++col) {
            QDomElement elem;
            elem = dom_.createElement(QLatin1String("column"));
            elem.appendChild(dom_.createTextNode(col < rowValues.count() ? rowValues.at(col) : QString()));
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
        fieldElem.setAttribute(QLatin1String("calendar"), KGlobal::locale()->calendar()->calendarType());
        QStringList s = fieldValue.split(QLatin1Char('-'), QString::KeepEmptyParts);
        if(s.count() > 0 && !s[0].isEmpty()) {
          QDomElement e = dom_.createElement(QLatin1String("year"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[0]));
        }
        if(s.count() > 1 && !s[1].isEmpty()) {
          QDomElement e = dom_.createElement(QLatin1String("month"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[1]));
        }
        if(s.count() > 2 && !s[2].isEmpty()) {
          QDomElement e = dom_.createElement(QLatin1String("day"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[2]));
        }
      } else if(fIt->type() == Data::Field::URL &&
                fIt->property(QLatin1String("relative")) == QLatin1String("true") &&
                !url().isEmpty()) {
        // if a relative URL and url() is not empty, change the value!
        KUrl old_url(Data::Document::self()->URL(), fieldValue);
        fieldElem.appendChild(dom_.createTextNode(KUrl::relativeUrl(url(), old_url)));
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
    myDebug() << "empty image!";
    return;
  }
//  myLog() << "id = " << id_;

  QDomElement imgElem = dom_.createElement(QLatin1String("image"));
  if(m_includeImages) {
    const Data::Image& img = ImageFactory::imageById(id_);
    if(img.isNull()) {
      myDebug() << "null image - " << id_;
      return;
    }
    imgElem.setAttribute(QLatin1String("format"), QLatin1String(img.format()));
    imgElem.setAttribute(QLatin1String("id"),     QString(img.id()));
    imgElem.setAttribute(QLatin1String("width"),  img.width());
    imgElem.setAttribute(QLatin1String("height"), img.height());
    if(img.linkOnly()) {
      imgElem.setAttribute(QLatin1String("link"), QLatin1String("true"));
    }
    QByteArray imgText = KCodecs::base64Encode(img.byteArray());
    imgElem.appendChild(dom_.createTextNode(QLatin1String(imgText)));
  } else {
    const Data::ImageInfo& info = ImageFactory::imageInfo(id_);
    if(info.isNull()) {
      return;
    }
    imgElem.setAttribute(QLatin1String("format"), QLatin1String(info.format));
    imgElem.setAttribute(QLatin1String("id"),     QString(info.id));
    // only load the images to read the size if necessary
    const bool loadImageIfNecessary = options() & Export::ExportImageSize;
    imgElem.setAttribute(QLatin1String("width"),  info.width(loadImageIfNecessary));
    imgElem.setAttribute(QLatin1String("height"), info.height(loadImageIfNecessary));
    if(info.linkOnly) {
      imgElem.setAttribute(QLatin1String("link"), QLatin1String("true"));
    }
  }
  parent_.appendChild(imgElem);
}

void TellicoXMLExporter::exportGroupXML(QDomDocument& dom_, QDomElement& parent_) const {
  Data::EntryList vec = entries();
  bool exportAll = collection()->entries().count() == vec.count();
  // iterate over each group, which are the first children
  for(ModelIterator gIt(ModelManager::self()->groupModel()); gIt.group(); ++gIt) {
    if(gIt.group()->isEmpty()) {
      continue;
    }
    QDomElement groupElem = dom_.createElement(QLatin1String("group"));
    groupElem.setAttribute(QLatin1String("title"), gIt.group()->groupName());
    // now iterate over all entry items in the group
    Data::EntryList sorted = sortEntries(*gIt.group());
    foreach(Data::EntryPtr eIt, sorted) {
      if(!exportAll && vec.indexOf(eIt) == -1) {
        continue;
      }
      QDomElement entryRefElem = dom_.createElement(QLatin1String("entryRef"));
      entryRefElem.setAttribute(QLatin1String("id"), QString::number(eIt->id()));
      groupElem.appendChild(entryRefElem);
    }
    if(groupElem.hasChildNodes()) {
      parent_.appendChild(groupElem);
    }
  }
}

void TellicoXMLExporter::exportFilterXML(QDomDocument& dom_, QDomElement& parent_, Tellico::FilterPtr filter_) const {
  QDomElement filterElem = dom_.createElement(QLatin1String("filter"));
  filterElem.setAttribute(QLatin1String("name"), filter_->name());

  QString match = (filter_->op() == Filter::MatchAll) ? QLatin1String("all") : QLatin1String("any");
  filterElem.setAttribute(QLatin1String("match"), match);

  foreach(FilterRule* rule, *filter_) {
    QDomElement ruleElem = dom_.createElement(QLatin1String("rule"));
    ruleElem.setAttribute(QLatin1String("field"), rule->fieldName());
    ruleElem.setAttribute(QLatin1String("pattern"), rule->pattern());
    switch(rule->function()) {
      case FilterRule::FuncContains:
        ruleElem.setAttribute(QLatin1String("function"), QLatin1String("contains"));
        break;
      case FilterRule::FuncNotContains:
        ruleElem.setAttribute(QLatin1String("function"), QLatin1String("notcontains"));
        break;
      case FilterRule::FuncEquals:
        ruleElem.setAttribute(QLatin1String("function"), QLatin1String("equals"));
        break;
      case FilterRule::FuncNotEquals:
        ruleElem.setAttribute(QLatin1String("function"), QLatin1String("notequals"));
        break;
      case FilterRule::FuncRegExp:
        ruleElem.setAttribute(QLatin1String("function"), QLatin1String("regexp"));
        break;
      case FilterRule::FuncNotRegExp:
        ruleElem.setAttribute(QLatin1String("function"), QLatin1String("notregexp"));
        break;
      default:
        myWarning() << "no matching rule function!";
    }
    filterElem.appendChild(ruleElem);
  }

  parent_.appendChild(filterElem);
}

void TellicoXMLExporter::exportBorrowerXML(QDomDocument& dom_, QDomElement& parent_,
                                           Tellico::Data::BorrowerPtr borrower_) const {
  if(borrower_->isEmpty()) {
    return;
  }

  QDomElement bElem = dom_.createElement(QLatin1String("borrower"));
  parent_.appendChild(bElem);

  bElem.setAttribute(QLatin1String("name"), borrower_->name());
  bElem.setAttribute(QLatin1String("uid"), borrower_->uid());

  foreach(Data::LoanPtr it, borrower_->loans()) {
    QDomElement lElem = dom_.createElement(QLatin1String("loan"));
    bElem.appendChild(lElem);

    lElem.setAttribute(QLatin1String("uid"), it->uid());
    lElem.setAttribute(QLatin1String("entryRef"), QString::number(it->entry()->id()));
    lElem.setAttribute(QLatin1String("loanDate"), it->loanDate().toString(Qt::ISODate));
    lElem.setAttribute(QLatin1String("dueDate"), it->dueDate().toString(Qt::ISODate));
    if(it->inCalendar()) {
      lElem.setAttribute(QLatin1String("calendar"), QLatin1String("true"));
    }

    lElem.appendChild(dom_.createTextNode(it->note()));
  }
}

QWidget* TellicoXMLExporter::widget(QWidget* parent_) {
  if(m_widget) {
    return m_widget;
  }

  m_widget = new QWidget(parent_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* gbox = new QGroupBox(i18n("Tellico XML Options"), m_widget);
  QVBoxLayout* vlay = new QVBoxLayout(gbox);

  m_checkIncludeImages = new QCheckBox(i18n("Include images in XML document"), gbox);
  m_checkIncludeImages->setChecked(m_includeImages);
  m_checkIncludeImages->setWhatsThis(i18n("If checked, the images in the document will be included "
                                          "in the XML stream as base64 encoded elements."));

  vlay->addWidget(m_checkIncludeImages);

  l->addWidget(gbox);
  l->addStretch(1);
  return m_widget;
}

void TellicoXMLExporter::readOptions(KSharedConfigPtr config_) {
  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_includeImages = group.readEntry("Include Images", m_includeImages);
}

void TellicoXMLExporter::saveOptions(KSharedConfigPtr config_) {
  m_includeImages = m_checkIncludeImages->isChecked();

  KConfigGroup group(config_, QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  group.writeEntry("Include Images", m_includeImages);
}

Tellico::Data::EntryList TellicoXMLExporter::sortEntries(const Data::EntryList& entries_) const {
  Data::EntryList sorted = entries_;

  EntrySortModel* model = static_cast<EntrySortModel*>(ModelManager::self()->entryModel());
  // have to go in reverse for sorting
  Data::FieldList fields;
  if(model->tertiarySortColumn() > -1) {
    fields << model->headerData(model->tertiarySortColumn(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
  }
  if(model->secondarySortColumn() > -1) {
    fields << model->headerData(model->secondarySortColumn(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
  }
  if(model->sortColumn() > -1) {
    fields << model->headerData(model->sortColumn(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
  }

  // now sort
  foreach(Data::FieldPtr field, fields) {
    std::sort(sorted.begin(), sorted.end(), Data::EntryCmp(field->name()));
  }

  return sorted;
}


#include "tellicoxmlexporter.moc"
