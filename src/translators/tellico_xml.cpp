/***************************************************************************
    copyright            : (C) 2003-2004 by Robby Stephenson
    email                : robby@periapsis.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License as  *
 *   published by the Free Software Foundation;                            *
 *                                                                         *
 ***************************************************************************/

#include "tellico_xml.h"

const QString Tellico::XML::nsXSL = QString::fromLatin1("http://www.w3.org/1999/XSL/Transform");
const QString Tellico::XML::nsBibtexml = QString::fromLatin1("http://bibtexml.sf.net/");
const QString Tellico::XML::dtdBibtexml = QString::fromLatin1("bibtexml.dtd");

/*
 * VERSION 2 added namespaces, changed to multiple elements,
 * and changed the "keywords" field to "keyword"
 *
 * VERSION 3 broke out the formatFlag, and changed NoComplete to AllowCompletion
 *
 * VERSION 4 added a bibtex-field name for Bibtex collections, element name was
 * changed to 'entry', field elements changed to 'field', and boolean fields are now "true"
 *
 * VERSION 5 moved the bibtex-field and any other extended field property to property elements
 * inside the field element, and added the image element.
 *
 * VERSION 6 added id, i18n attributes, and year, month, day elements in date fields with a calendar name
 * attribute.
 *
 * VERSION 7 changed the application name to Tellico, renamed unitTitle to entryTitle, and made the id permanent.
 */
const uint Tellico::XML::syntaxVersion = 7;
const QString Tellico::XML::nsTellico = QString::fromLatin1("http://periapsis.org/tellico/");
const QString Tellico::XML::pubTellico = QString::fromLatin1("-//Robby Stephenson/DTD Tellico V%1.0//EN").arg(Tellico::XML::syntaxVersion);
const QString Tellico::XML::dtdTellico = QString::fromLatin1("http://periapsis.org/tellico/dtd/v%1/tellico.dtd").arg(Tellico::XML::syntaxVersion);

const QString Tellico::XML::nsBookcase = QString::fromLatin1("http://periapsis.org/bookcase/");
