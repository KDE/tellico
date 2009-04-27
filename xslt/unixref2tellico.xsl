<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:cr="http://www.crossref.org/xschema/1.0"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                exclude-result-prefixes="cr"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from crossref.org
   in the 'unixref' format.

   See http://www.crossref.org/schema/unixref1.0.xsd

   Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V10.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v10/tellico.dtd"/>

<!-- by default, don't output text -->
<xsl:template match="text()" />

<xsl:template match="/">
 <tellico syntaxVersion="10">
  <!-- always bibliography -->
  <collection title="CrossRef Import" type="5">
   <fields>
    <field name="_default"/>
    <xsl:if test=".//cr:issn">
     <field flags="0" title="ISSN" category="Publishing" format="4" type="1" name="issn" i18n="true"/>
    </xsl:if>
   </fields>
   <xsl:apply-templates select="cr:doi_records/cr:doi_record/cr:crossref"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="cr:crossref">
 <!-- if there's an error, or none found, a crossref element still shows up, with an error element -->
 <xsl:if test="not(cr:error)">
  <entry>
   <xsl:apply-templates/>
  </entry>
 </xsl:if>
</xsl:template>

<xsl:template match="cr:book">
 <entry-type>book</entry-type>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:journal">
 <entry-type>article</entry-type>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:book_metadata">
 <title>
  <xsl:value-of select="cr:titles/cr:title[1]"/>
 </title>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:journal_article">
 <title>
  <xsl:value-of select="cr:titles/cr:title[1]"/>
 </title>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:isbn">
 <isbn>
  <xsl:value-of select="."/>
 </isbn>
</xsl:template>

<xsl:template match="cr:issn">
 <issn>
  <xsl:value-of select="."/>
 </issn>
</xsl:template>

<xsl:template match="cr:publisher">
 <publisher>
  <xsl:value-of select="cr:publisher_name"/>
 </publisher>
 <address>
  <xsl:value-of select="cr:publisher_place"/>
 </address>
</xsl:template>

<xsl:template match="cr:journal_metadata">
 <journal>
  <xsl:value-of select="cr:full_title"/>
 </journal>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:edition">
 <edition>
  <xsl:value-of select="."/>
 </edition>
</xsl:template>

<xsl:template match="cr:volume">
 <volume>
  <xsl:value-of select="."/>
 </volume>
</xsl:template>

<xsl:template match="cr:issue">
 <number>
  <xsl:value-of select="."/>
 </number>
</xsl:template>

<xsl:template match="cr:series_metadata">
 <series>
  <xsl:value-of select="cr:titles/cr:title[1]"/>
 </series>
</xsl:template>

<xsl:template match="cr:doi_data">
 <doi>
  <xsl:value-of select="cr:doi"/>
 </doi>
 <url>
  <xsl:value-of select="cr:resource"/>
 </url>
</xsl:template>

<xsl:template match="cr:publication_date">
 <year>
  <xsl:value-of select="cr:year"/>
 </year>
 <month>
  <xsl:value-of select="cr:month"/>
 </month>
</xsl:template>

<xsl:template match="cr:pages">
 <pages>
  <xsl:value-of select="concat(cr:first_page,'-',cr:last_page)"/>
 </pages>
</xsl:template>

<xsl:template match="cr:contributors">
 <authors>
  <xsl:for-each select="cr:person_name[@contributor_role='author']">
   <author>
    <xsl:value-of select="concat(cr:given_name,' ',cr:surname)"/>
   </author>
  </xsl:for-each>
 </authors>
 <editors>
  <xsl:for-each select="cr:person_name[@contributor_role='editor']">
   <editor>
    <xsl:value-of select="concat(cr:given_name,' ',cr:surname)"/>
   </editor>
  </xsl:for-each>
 </editors>
 <organization>
  <xsl:value-of select="cr:organization[1]"/>
 </organization>
</xsl:template>

</xsl:stylesheet>
