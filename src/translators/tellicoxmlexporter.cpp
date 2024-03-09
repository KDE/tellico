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
#include "../utils/bibtexhandler.h" // needed for cleaning text
#include "../utils/string_utils.h"
#include "../utils/urlfieldlogic.h"
#include "../entrygroup.h"
#include "../collections/bibtexcollection.h"
#include "../images/imagefactory.h"
#include "../images/image.h"
#include "../images/imageinfo.h"
#include "../core/filehandler.h"
#include "../document.h"
#include "../fieldformat.h"
#include "../models/entrysortmodel.h"
#include "../models/modelmanager.h"
#include "../models/modeliterator.h"
#include "../models/models.h"
#include "../tellico_debug.h"

#include <KLocalizedString>
#include <KConfigGroup>

#include <QDir>
#include <QGroupBox>
#include <QCheckBox>
#include <QDomDocument>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QTextCodec>
#else
#include <QStringConverter>
#endif
#include <QVBoxLayout>

#include <algorithm>

using namespace Tellico;
using Tellico::Export::TellicoXMLExporter;

TellicoXMLExporter::TellicoXMLExporter(Tellico::Data::CollPtr coll) : Exporter(coll),
      m_includeImages(false), m_includeGroups(false), m_widget(nullptr), m_checkIncludeImages(nullptr) {
  setOptions(options() | Export::ExportImages | Export::ExportImageSize); // not included by default
}

TellicoXMLExporter::~TellicoXMLExporter() {
}

QString TellicoXMLExporter::formatString() const {
  return QStringLiteral("XML");
}

QString TellicoXMLExporter::fileFilter() const {
  return i18n("XML Files") + QLatin1String(" (*.xml)") + QLatin1String(";;") + i18n("All Files") + QLatin1String(" (*)");
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

QString TellicoXMLExporter::text() const {
  return exportXML().toString();
}

QDomDocument TellicoXMLExporter::exportXML() const {
  int exportVersion = XML::syntaxVersion;

  if(exportVersion == 12 && !version12Needed()) {
    exportVersion = 11;
  }

  QDomImplementation impl;
  // Bug 443845 - but do not just silent drop the invalid characters
  // since that drops emojis and unicode points with surrogate encoding
  // instead Tellico::removeControlCodes is used everywhere that
  // QDomDocument::createTextNode() is called
//  impl.setInvalidDataPolicy(QDomImplementation::DropInvalidChars);
  QDomDocumentType doctype = impl.createDocumentType(QStringLiteral("tellico"),
                                                     XML::pubTellico(exportVersion),
                                                     XML::dtdTellico(exportVersion));
  //default namespace
  const QString& ns = XML::nsTellico;

  QDomDocument dom = impl.createDocument(ns, QStringLiteral("tellico"), doctype);

  // root tellico element
  QDomElement root = dom.documentElement();

  QString encodeStr = QStringLiteral("version=\"1.0\" encoding=\"");
  if(options() & Export::ExportUTF8) {
    encodeStr += QLatin1String("UTF-8");
  } else {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    encodeStr += QLatin1String(QTextCodec::codecForLocale()->name());
#else
    encodeStr += QLatin1String(QStringConverter::nameForEncoding(QStringConverter::System));
#endif
  }
  encodeStr += QLatin1Char('"');

  // createDocument creates a root node, insert the processing instruction before it
  dom.insertBefore(dom.createProcessingInstruction(QStringLiteral("xml"), encodeStr), root);

  root.setAttribute(QStringLiteral("syntaxVersion"), exportVersion);

  FieldFormat::Request format = (options() & Export::ExportFormatted ?
                                                FieldFormat::ForceFormat :
                                                FieldFormat::AsIsFormat);

  exportCollectionXML(dom, root, format);

  // clear image list
  m_images.clear();

  return dom;
}

void TellicoXMLExporter::exportCollectionXML(QDomDocument& dom_, QDomElement& parent_, int format_) const {
  Data::CollPtr coll = collection();
  if(!coll) {
    myWarning() << "no collection pointer!";
    return;
  }

  QDomElement collElem = dom_.createElement(QStringLiteral("collection"));
  collElem.setAttribute(QStringLiteral("type"), coll->type());
  collElem.setAttribute(QStringLiteral("title"), coll->title());

  QDomElement fieldsElem = dom_.createElement(QStringLiteral("fields"));
  collElem.appendChild(fieldsElem);

  foreach(Data::FieldPtr field, fields()) {
    exportFieldXML(dom_, fieldsElem, field);
  }

  if(coll->type() == Data::Collection::Bibtex) {
    const Data::BibtexCollection* c = static_cast<const Data::BibtexCollection*>(coll.data());
    if(!c->preamble().isEmpty()) {
      QDomElement preElem = dom_.createElement(QStringLiteral("bibtex-preamble"));
      preElem.appendChild(dom_.createTextNode(removeControlCodes(c->preamble())));
      collElem.appendChild(preElem);
    }

    QDomElement macrosElem = dom_.createElement(QStringLiteral("macros"));
    for(StringMap::ConstIterator macroIt = c->macroList().constBegin(); macroIt != c->macroList().constEnd(); ++macroIt) {
      if(!macroIt.value().isEmpty()) {
        QDomElement macroElem = dom_.createElement(QStringLiteral("macro"));
        macroElem.setAttribute(QStringLiteral("name"), macroIt.key());
        macroElem.appendChild(dom_.createTextNode(removeControlCodes(macroIt.value())));
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
    QDomElement imgsElem = dom_.createElement(QStringLiteral("images"));
    const QStringList imageIds = m_images.values();
    foreach(const QString& id, m_images) {
      exportImageXML(dom_, imgsElem, id);
    }
    if(imgsElem.hasChildNodes()) {
      collElem.appendChild(imgsElem);
    }
  }

  if(m_includeGroups) {
    exportGroupXML(dom_, collElem);
  }

  parent_.appendChild(collElem);

  // the borrowers and filters are in the tellico object, not the collection
  if(options() & Export::ExportComplete) {
    QDomElement bElem = dom_.createElement(QStringLiteral("borrowers"));
    foreach(Data::BorrowerPtr borrower, coll->borrowers()) {
      exportBorrowerXML(dom_, bElem, borrower);
    }
    if(bElem.hasChildNodes()) {
      parent_.appendChild(bElem);
    }

    QDomElement fElem = dom_.createElement(QStringLiteral("filters"));
    foreach(FilterPtr filter, coll->filters()) {
      exportFilterXML(dom_, fElem, filter);
    }
    if(fElem.hasChildNodes()) {
      parent_.appendChild(fElem);
    }
  }
}

void TellicoXMLExporter::exportFieldXML(QDomDocument& dom_, QDomElement& parent_, Tellico::Data::FieldPtr field_) const {
  QDomElement elem = dom_.createElement(QStringLiteral("field"));

  elem.setAttribute(QStringLiteral("name"),     field_->name());
  elem.setAttribute(QStringLiteral("title"),    field_->title());
  elem.setAttribute(QStringLiteral("category"), field_->category());
  elem.setAttribute(QStringLiteral("type"),     field_->type());
  elem.setAttribute(QStringLiteral("flags"),    field_->flags());
  elem.setAttribute(QStringLiteral("format"),   field_->formatType());

  if(field_->type() == Data::Field::Choice) {
    elem.setAttribute(QStringLiteral("allowed"), field_->allowed().join(QLatin1String(";")));
  }

  // only save description if it's not equal to title, which is the default
  // title is never empty, so this indirectly checks for empty descriptions
  if(field_->description() != field_->title()) {
    elem.setAttribute(QStringLiteral("description"), field_->description());
  }

  for(StringMap::ConstIterator it = field_->propertyList().begin(); it != field_->propertyList().end(); ++it) {
    if(it.value().isEmpty()) {
      continue;
    }
    QDomElement e = dom_.createElement(QStringLiteral("prop"));
    e.setAttribute(QStringLiteral("name"), it.key());
    e.appendChild(dom_.createTextNode(removeControlCodes(it.value())));
    elem.appendChild(e);
  }

  parent_.appendChild(elem);
}

void TellicoXMLExporter::exportEntryXML(QDomDocument& dom_, QDomElement& parent_, Tellico::Data::EntryPtr entry_, int format_) const {
  QDomElement entryElem = dom_.createElement(QStringLiteral("entry"));
  entryElem.setAttribute(QStringLiteral("id"), QString::number(entry_->id()));

  // iterate through every field for the entry
  foreach(Data::FieldPtr fIt, fields()) {
    QString fieldName = fIt->name();

    // Date fields are special, don't format in export
    QString fieldValue = (format_ == FieldFormat::ForceFormat && fIt->type() != Data::Field::Date) ?
                                                           entry_->formattedField(fieldName, FieldFormat::ForceFormat) :
                                                           entry_->field(fieldName);
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

    if(fIt->type() == Data::Field::Table) {
      // who cares about grammar, just add an 's' to the name
      QDomElement parElem = dom_.createElement(fieldName + QLatin1Char('s'));
      entryElem.appendChild(parElem);

      bool ok;
      int ncols = Tellico::toUInt(fIt->property(QStringLiteral("columns")), &ok);
      if(!ok || ncols < 1) {
        ncols = 1;
      }
      foreach(const QString& rowValue, FieldFormat::splitTable(fieldValue)) {
        QDomElement fieldElem = dom_.createElement(fieldName);
        parElem.appendChild(fieldElem);

        QStringList columnValues = FieldFormat::splitRow(rowValue);
        if(ncols < columnValues.count()) {
          // need to combine all the last values, from ncols-1 to end
          QString lastValue = QStringList(columnValues.mid(ncols-1)).join(FieldFormat::columnDelimiterString());
          columnValues = columnValues.mid(0, ncols);
          columnValues.replace(ncols-1, lastValue);
        }
        for(int col = 0; col < columnValues.count(); ++col) {
          QDomElement elem = dom_.createElement(QStringLiteral("column"));
          elem.appendChild(dom_.createTextNode(removeControlCodes(columnValues.at(col))));
          fieldElem.appendChild(elem);
        }
      }
      continue;
    }

    if(fIt->hasFlag(Data::Field::AllowMultiple)) {
      // if multiple versions are allowed, split them into separate elements
      // parent element if field contains multiple values, child of entryElem
      // who cares about grammar, just add an QLatin1Char('s') to the name
      QDomElement parElem = dom_.createElement(fieldName + QLatin1Char('s'));
      entryElem.appendChild(parElem);

      // the space after the semi-colon is enforced when the field is set for the entry
      QStringList fields = FieldFormat::splitValue(fieldValue);
      for(QStringList::ConstIterator it = fields.constBegin(); it != fields.constEnd(); ++it) {
        // element for field value, child of either entryElem or ParentElem
        QDomElement fieldElem = dom_.createElement(fieldName);
        fieldElem.appendChild(dom_.createTextNode(removeControlCodes(*it)));
        parElem.appendChild(fieldElem);
      }
    } else {
      QDomElement fieldElem = dom_.createElement(fieldName);
      entryElem.appendChild(fieldElem);
      // Date fields get special treatment
      if(fIt->type() == Data::Field::Date) {
        // as of Tellico in KF5 (3.0), just forget about the calendar attribute for the moment, always use gregorian
        fieldElem.setAttribute(QStringLiteral("calendar"), QStringLiteral("gregorian"));
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
        QStringList s = fieldValue.split(QLatin1Char('-'), QString::KeepEmptyParts);
#else
        QStringList s = fieldValue.split(QLatin1Char('-'), Qt::KeepEmptyParts);
#endif
        if(s.count() > 0 && !s[0].isEmpty()) {
          QDomElement e = dom_.createElement(QStringLiteral("year"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[0]));
        }
        if(s.count() > 1 && !s[1].isEmpty()) {
          QDomElement e = dom_.createElement(QStringLiteral("month"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[1]));
        }
        if(s.count() > 2 && !s[2].isEmpty()) {
          QDomElement e = dom_.createElement(QStringLiteral("day"));
          fieldElem.appendChild(e);
          e.appendChild(dom_.createTextNode(s[2]));
        }
      } else if(fIt->type() == Data::Field::URL &&
                fIt->property(QStringLiteral("relative")) == QLatin1String("true")) {
        // if a relative URL and url() is not empty, change the value!
        QUrl old_url = Data::Document::self()->URL().resolved(QUrl(fieldValue));
        if(options() & Export::ExportAbsoluteLinks) {
          fieldElem.appendChild(dom_.createTextNode(old_url.url()));
        } else if(!url().isEmpty()) {
          QUrl new_url(url());
          if(new_url.scheme() == old_url.scheme() &&
             new_url.host() == old_url.host()) {
            // calculate a new relative url
            UrlFieldLogic logic;
            logic.setRelative(true);
            logic.setBaseUrl(url());
            fieldElem.appendChild(dom_.createTextNode(logic.urlText(old_url)));
          } else {
            // use the absolute url here
            fieldElem.appendChild(dom_.createTextNode(old_url.url()));
          }
        } else {
          fieldElem.appendChild(dom_.createTextNode(removeControlCodes(fieldValue)));
        }
      } else {
        fieldElem.appendChild(dom_.createTextNode(removeControlCodes(fieldValue)));
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

  QDomElement imgElem = dom_.createElement(QStringLiteral("image"));
  if(m_includeImages) {
    const Data::Image& img = ImageFactory::imageById(id_);
    if(img.isNull()) {
      return;
    }
    imgElem.setAttribute(QStringLiteral("format"), QLatin1String(img.format()));
    imgElem.setAttribute(QStringLiteral("id"),     QString(img.id()));
    imgElem.setAttribute(QStringLiteral("width"),  img.width());
    imgElem.setAttribute(QStringLiteral("height"), img.height());
    if(img.linkOnly()) {
      imgElem.setAttribute(QStringLiteral("link"), QStringLiteral("true"));
    }
    QByteArray imgText = img.byteArray().toBase64();
    imgElem.appendChild(dom_.createTextNode(QLatin1String(imgText)));
  } else {
    const Data::ImageInfo& info = ImageFactory::imageInfo(id_);
    if(info.isNull()) {
      return;
    }
    imgElem.setAttribute(QStringLiteral("format"), QLatin1String(info.format));
    imgElem.setAttribute(QStringLiteral("id"),     QString(info.id));
    // only load the images to read the size if necessary
    const bool loadImageIfNecessary = options() & Export::ExportImageSize;
    imgElem.setAttribute(QStringLiteral("width"),  info.width(loadImageIfNecessary));
    imgElem.setAttribute(QStringLiteral("height"), info.height(loadImageIfNecessary));
    if(info.linkOnly) {
      imgElem.setAttribute(QStringLiteral("link"), QStringLiteral("true"));
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
    QDomElement groupElem = dom_.createElement(QStringLiteral("group"));
    groupElem.setAttribute(QStringLiteral("title"), gIt.group()->groupName());
    // now iterate over all entry items in the group
    Data::EntryList sorted = sortEntries(*gIt.group());
    foreach(Data::EntryPtr eIt, sorted) {
      if(!exportAll && vec.indexOf(eIt) == -1) {
        continue;
      }
      QDomElement entryRefElem = dom_.createElement(QStringLiteral("entryRef"));
      entryRefElem.setAttribute(QStringLiteral("id"), QString::number(eIt->id()));
      groupElem.appendChild(entryRefElem);
    }
    if(groupElem.hasChildNodes()) {
      parent_.appendChild(groupElem);
    }
  }
}

void TellicoXMLExporter::exportFilterXML(QDomDocument& dom_, QDomElement& parent_, Tellico::FilterPtr filter_) const {
  QDomElement filterElem = dom_.createElement(QStringLiteral("filter"));
  filterElem.setAttribute(QStringLiteral("name"), filter_->name());

  QString match = (filter_->op() == Filter::MatchAll) ? QStringLiteral("all") : QStringLiteral("any");
  filterElem.setAttribute(QStringLiteral("match"), match);

  foreach(FilterRule* rule, *filter_) {
    QDomElement ruleElem = dom_.createElement(QStringLiteral("rule"));
    ruleElem.setAttribute(QStringLiteral("field"), rule->fieldName());
    ruleElem.setAttribute(QStringLiteral("pattern"), rule->pattern());
    switch(rule->function()) {
      case FilterRule::FuncContains:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("contains"));
        break;
      case FilterRule::FuncNotContains:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("notcontains"));
        break;
      case FilterRule::FuncEquals:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("equals"));
        break;
      case FilterRule::FuncNotEquals:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("notequals"));
        break;
      case FilterRule::FuncRegExp:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("regexp"));
        break;
      case FilterRule::FuncNotRegExp:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("notregexp"));
        break;
      case FilterRule::FuncBefore:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("before"));
        break;
      case FilterRule::FuncAfter:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("after"));
        break;
      case FilterRule::FuncGreater:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("greaterthan"));
        break;
      case FilterRule::FuncLess:
        ruleElem.setAttribute(QStringLiteral("function"), QStringLiteral("lessthan"));
        break;
      /* If anything is updated here, be sure to update xmlstatehandler */
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

  QDomElement bElem = dom_.createElement(QStringLiteral("borrower"));
  parent_.appendChild(bElem);

  bElem.setAttribute(QStringLiteral("name"), borrower_->name());
  bElem.setAttribute(QStringLiteral("uid"), borrower_->uid());

  foreach(Data::LoanPtr it, borrower_->loans()) {
    QDomElement lElem = dom_.createElement(QStringLiteral("loan"));
    bElem.appendChild(lElem);

    lElem.setAttribute(QStringLiteral("uid"), it->uid());
    lElem.setAttribute(QStringLiteral("entryRef"), QString::number(it->entry()->id()));
    lElem.setAttribute(QStringLiteral("loanDate"), it->loanDate().toString(Qt::ISODate));
    lElem.setAttribute(QStringLiteral("dueDate"), it->dueDate().toString(Qt::ISODate));
    if(it->inCalendar()) {
      lElem.setAttribute(QStringLiteral("calendar"), QStringLiteral("true"));
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
  KConfigGroup group(config_, QStringLiteral("ExportOptions - %1").arg(formatString()));
  m_includeImages = group.readEntry("Include Images", m_includeImages);
}

void TellicoXMLExporter::saveOptions(KSharedConfigPtr config_) {
  m_includeImages = m_checkIncludeImages->isChecked();

  KConfigGroup group(config_, QStringLiteral("ExportOptions - %1").arg(formatString()));
  group.writeEntry("Include Images", m_includeImages);
}

Tellico::Data::EntryList TellicoXMLExporter::sortEntries(const Data::EntryList& entries_) const {
  Data::EntryList sorted = entries_;

  EntrySortModel* model = static_cast<EntrySortModel*>(ModelManager::self()->entryModel());
  // have to go in reverse for sorting
  Data::FieldList fields;
  Data::FieldPtr field;
  if(model->tertiarySortColumn() > -1) {
    field = model->headerData(model->tertiarySortColumn(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field) {
      fields << field;
    } else {
      myDebug() << "no field for tertiary sort column" << model->tertiarySortColumn();
    }
  }
  if(model->secondarySortColumn() > -1) {
    field = model->headerData(model->secondarySortColumn(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field) {
      fields << field;
    } else {
      myDebug() << "no field for secondary sort column" << model->secondarySortColumn();
    }
  }
  if(model->sortColumn() > -1) {
    field = model->headerData(model->sortColumn(), Qt::Horizontal, FieldPtrRole).value<Data::FieldPtr>();
    if(field) {
      fields << field;
    } else {
      myDebug() << "no field for primary sort column" << model->sortColumn();
    }
  }

  // now sort
  foreach(Data::FieldPtr field, fields) {
    std::sort(sorted.begin(), sorted.end(), Data::EntryCmp(field->name()));
  }

  return sorted;
}

bool TellicoXMLExporter::version12Needed() const {
  // version 12 is only necessary if the new filter rules are used
  foreach(FilterPtr filter, collection()->filters()) {
    foreach(FilterRule* rule, *filter) {
      if(rule->function() == FilterRule::FuncBefore ||
         rule->function() == FilterRule::FuncAfter ||
         rule->function() == FilterRule::FuncGreater ||
         rule->function() == FilterRule::FuncLess) {
        return true;
      }
    }
  }
  return false;
}
