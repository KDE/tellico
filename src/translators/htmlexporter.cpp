/***************************************************************************
                              htmlexporter.cpp
                             -------------------
    begin                : Sat Aug 2 2003
    copyright            : (C) 2003 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "htmlexporter.h"
#include "xslthandler.h"
#include "bookcasexmlexporter.h"
#include "../bccollection.h"
#include "../bcfilehandler.h"

#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qdom.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

HTMLExporter::HTMLExporter(const BCCollection* coll_, BCUnitList list_) : Exporter(coll_, list_),
    m_printHeaders(true),
    m_printGrouped(false),
    m_widget(0),
    m_xsltfile(QString::fromLatin1("bookcase2html.xsl")),
    m_columns(QString::fromLatin1("title")) {
}

QString HTMLExporter::formatString() const {
  return i18n("HTML");
}

QString HTMLExporter::fileFilter() const {
  return i18n("*.html|HTML files (*.html)") + QString::fromLatin1("\n") + i18n("*|All files");
}

QWidget* HTMLExporter::widget(QWidget* parent_, const char* name_/*=0*/) {
  if(m_widget && m_widget->parent() == parent_) {
    return m_widget;
  }

  m_widget = new QWidget(parent_, name_);
  QVBoxLayout* l = new QVBoxLayout(m_widget);

  QGroupBox* box = new QGroupBox(1, Qt::Horizontal, i18n("HTML Options"), m_widget);
  l->addWidget(box);

  m_checkPrintHeaders = new QCheckBox(i18n("Print field headers"), box);
  QWhatsThis::add(m_checkPrintHeaders, i18n("If checked, the field names will be "
                                            "printed as table headers."));
  m_checkPrintHeaders->setChecked(m_printHeaders);
  
  m_checkPrintGrouped = new QCheckBox(i18n("Group the entries"), box);
  QWhatsThis::add(m_checkPrintGrouped, i18n("If checked, the entries will be grouped by "
                                            "the selected field."));
  m_checkPrintGrouped->setChecked(m_printGrouped);

  l->addStretch(1);
  return m_widget;
}

void HTMLExporter::readOptions(KConfig* config_) {
  config_->setGroup(QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_printHeaders = config_->readBoolEntry("Print Field Headers", m_printHeaders);
  m_printGrouped = config_->readBoolEntry("Print Grouped", m_printGrouped);
}

void HTMLExporter::saveOptions(KConfig* config_) {
  config_->setGroup(QString::fromLatin1("ExportOptions - %1").arg(formatString()));
  m_printHeaders = m_checkPrintHeaders->isChecked();
  config_->writeEntry("Print Field Headers", m_printHeaders);
  m_printGrouped = m_checkPrintGrouped->isChecked();
  config_->writeEntry("Print Grouped", m_printGrouped);
}

// TODO: figure out encoding
QString HTMLExporter::text(bool formatAttributes_, bool encodeUTF8_) {
  QString xsltfile = KGlobal::dirs()->findResource("appdata", m_xsltfile);
  if(xsltfile.isNull()) {
    return QString::null;
  }

  QDomDocument dom = BCFileHandler::readXMLFile(KURL(xsltfile));
  const BCCollection* coll = collection();

  if(m_printGrouped && !m_groupBy.isEmpty()) {
    // since the XSL stylesheet can't use a parameter as a key name, need to replace keys
    QDomNodeList keyNodes = dom.elementsByTagName(QString::fromLatin1("xsl:key"));
    for(unsigned i = 0; i < keyNodes.count(); ++i) {
      QDomElement elem = keyNodes.item(i).toElement();
      // dont't forget bc namespace
      // the entries key needs to use the groupName as the property
      if(elem.attribute(QString::fromLatin1("name"), QString::null) == QString::fromLatin1("entries")) {
        QString s;
        for(QStringList::ConstIterator it = m_groupBy.begin(); it != m_groupBy.end(); ++it) {
          s += QString::fromLatin1(".//bc:") + *it;
          if(coll->attributeByName(*it)->type() == BCAttribute::Table2) {
            s += QString::fromLatin1("/bc:column[1]");
          }
          if(*it != m_groupBy.last()) {
            s += QString::fromLatin1("|");
          }
        }
        elem.setAttribute(QString::fromLatin1("use"), s);
      // the groups key needs to use the groupName as the match
      } else if(elem.attribute(QString::fromLatin1("name"), QString::null) == QString::fromLatin1("groups")) {
        QString s;
        for(QStringList::ConstIterator it = m_groupBy.begin(); it != m_groupBy.end(); ++it) {
          s += QString::fromLatin1("bc:") + *it;
          if(coll->attributeByName(*it)->type() == BCAttribute::Table2) {
            s += QString::fromLatin1("/bc:column[1]");
          }
          if(*it != m_groupBy.last()) {
            s += QString::fromLatin1("|");
          }
        }
        elem.setAttribute(QString::fromLatin1("match"), s);
      }
    }
    QDomNodeList varNodes = dom.elementsByTagName(QString::fromLatin1("xsl:variable"));
    for(unsigned i = 0; i < varNodes.count(); ++i) {
      QDomElement elem = varNodes.item(i).toElement();
      if(elem.attribute(QString::fromLatin1("name"), QString::null) == QString::fromLatin1("all-groups")) {
        QString s;
        for(QStringList::ConstIterator it = m_groupBy.begin(); it != m_groupBy.end(); ++it) {
          s += QString::fromLatin1("//bc:") + *it;
          if(coll->attributeByName(*it)->type() == BCAttribute::Table2) {
            s += QString::fromLatin1("/bc:column[1]");
          }
          if(*it != m_groupBy.last()) {
            s += QString::fromLatin1("|");
          }
        }
        elem.setAttribute(QString::fromLatin1("select"), s);
        break;
      }
    }
  }

//  kdDebug() << dom.toString() << endl;
  XSLTHandler handler(dom.toString());
  handler.addParam("show-headers", m_printHeaders ? "true()" : "false()");
  handler.addParam("group-entries", m_printGrouped ? "true()" : "false()");

  QStringList sortTitles;
  if(!m_sort1.isEmpty()) {
    sortTitles << m_sort1;
  }
  if(!m_sort2.isEmpty()) {
    sortTitles << m_sort2;
  }

  // the third sort column may be same as first
  if(!m_sort3.isEmpty() && sortTitles.findIndex(m_sort3) == -1) {
    sortTitles << m_sort3;
  }

  handler.addStringParam("sort-name1", coll->attributeNameByTitle(sortTitles[0]).utf8());
  if(sortTitles.count() > 1) {
    handler.addStringParam("sort-name2", coll->attributeNameByTitle(sortTitles[1]).utf8());
    if(sortTitles.count() > 2) {
      handler.addStringParam("sort-name3", coll->attributeNameByTitle(sortTitles[2]).utf8());
    }
  }

  QString sortString;
  if(m_printGrouped) {
    QString s;
    // if more than one, then it's the People pseudo-group
    if(m_groupBy.count() > 1) {
      s = i18n("People");
    } else {
      s = coll->attributeTitleByName(m_groupBy[0]);
    }
    sortString = i18n("(grouped by %1; sorted by %2)").arg(s).arg(sortTitles.join(QString::fromLatin1(", ")));
  } else {
    sortString = i18n("(sorted by %1)").arg(sortTitles.join(QString::fromLatin1(", ")));
  }

  handler.addStringParam("sort-title", encodeUTF8_ ? sortString.utf8() : sortString.local8Bit());

  QString pageTitle = QString::fromLatin1("Bookcase: ") + coll->title();
  pageTitle += QString::fromLatin1(" ") + sortString;
  handler.addStringParam("page-title", encodeUTF8_ ? pageTitle.utf8() : pageTitle.local8Bit());

  QStringList printFields;
  for(QStringList::ConstIterator it = m_columns.begin(); it != m_columns.end(); ++it) {
    printFields << coll->attributeNameByTitle(*it);
  }
  handler.addStringParam("column-names",
                         encodeUTF8_ ? printFields.join(QString::fromLatin1(" ")).utf8()
                                     : printFields.join(QString::fromLatin1(" ")).local8Bit());

  BookcaseXMLExporter exporter(coll, unitList());
  dom = exporter.exportXML(formatAttributes_, true);

//  kdDebug() << dom.toString() << endl;
  return handler.applyStylesheet(dom.toString(), encodeUTF8_);
}
