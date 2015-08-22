<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
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
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<!-- by default, don't output text -->
<xsl:template match="text()" />

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <!-- always bibliography -->
  <tc:collection title="CrossRef Import" type="5">
   <tc:fields>
    <tc:field name="_default"/>
    <xsl:if test=".//cr:issn|.//issn">
     <tc:field flags="0" title="ISSN" category="Publishing" format="4" type="1" name="issn" i18n="true"/>
    </xsl:if>
   </tc:fields>
   <xsl:apply-templates select=".//cr:doi_record/cr:crossref|.//doi_record/crossref"/>
  </tc:collection>
 </tc:tellico>
</xsl:template>

<xsl:template match="cr:crossref|crossref">
 <!-- if there's an error, or none found, a crossref element still shows up, with an error element -->
 <xsl:if test="not(cr:error) and not(error)">
  <tc:entry>
   <xsl:apply-templates/>
  </tc:entry>
 </xsl:if>
</xsl:template>

<xsl:template match="cr:book|book">
 <tc:entry-type>book</tc:entry-type>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:journal|journal">
 <tc:entry-type>article</tc:entry-type>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:book_metadata|book_metadata|cr:journal_article|journal_article">
 <tc:title>
  <xsl:value-of select="(cr:titles/cr:title[1]|titles/title[1])[1]"/>
 </tc:title>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:isbn|isbn">
 <tc:isbn>
  <xsl:value-of select="."/>
 </tc:isbn>
</xsl:template>

<xsl:template match="cr:issn|issn">
 <tc:issn>
  <xsl:value-of select="."/>
 </tc:issn>
</xsl:template>

<xsl:template match="cr:publisher|publisher">
 <tc:publishers>
  <tc:publisher>
   <xsl:value-of select="cr:publisher_name|publisher_name"/>
  </tc:publisher>
 </tc:publishers>
 <tc:address>
  <xsl:value-of select="cr:publisher_place|publisher_place"/>
 </tc:address>
</xsl:template>

<xsl:template match="cr:journal_metadata|journal_metadata">
 <tc:journal>
  <xsl:value-of select="cr:full_title|full_title"/>
 </tc:journal>
 <xsl:apply-templates/>
</xsl:template>

<xsl:template match="cr:edition|edition">
 <tc:edition>
  <xsl:value-of select="."/>
 </tc:edition>
</xsl:template>

<xsl:template match="cr:volume|volume">
 <tc:volume>
  <xsl:value-of select="."/>
 </tc:volume>
</xsl:template>

<xsl:template match="cr:issue|issue">
 <tc:number>
  <xsl:value-of select="."/>
 </tc:number>
</xsl:template>

<xsl:template match="cr:series_metadata|series_metadata">
 <tc:series>
  <xsl:value-of select="(cr:titles/cr:title[1]|titles/title[1])[1]"/>
 </tc:series>
</xsl:template>

<xsl:template match="cr:doi_data|doi_data">
 <tc:doi>
  <xsl:value-of select="cr:doi|doi"/>
 </tc:doi>
 <tc:url>
  <xsl:value-of select="cr:resource|resource"/>
 </tc:url>
</xsl:template>

<xsl:template match="cr:publication_date|publication_date">
 <tc:year>
  <xsl:value-of select="cr:year|year"/>
 </tc:year>
 <tc:month>
  <xsl:value-of select="cr:month|month"/>
 </tc:month>
</xsl:template>

<xsl:template match="cr:pages|pages">
 <tc:pages>
  <xsl:if test="cr:first_page">
   <xsl:value-of select="concat(cr:first_page,'-',cr:last_page)"/>
  </xsl:if>
  <xsl:if test="first_page">
   <xsl:value-of select="concat(first_page,'-',last_page)"/>
  </xsl:if>
 </tc:pages>
</xsl:template>

<xsl:template match="cr:contributors|contributors">
 <tc:authors>
  <xsl:for-each select="cr:person_name[@contributor_role='author']|person_name[@contributor_role='author']">
   <tc:author>
    <xsl:if test="cr:given_name">
     <xsl:value-of select="concat(cr:given_name,' ',cr:surname)"/>
    </xsl:if>
    <xsl:if test="given_name">
     <xsl:value-of select="concat(given_name,' ',surname)"/>
    </xsl:if>
   </tc:author>
  </xsl:for-each>
 </tc:authors>
 <tc:editors>
  <xsl:for-each select="cr:person_name[@contributor_role='editor']|person_name[@contributor_role='editor']">
   <tc:editor>
    <xsl:if test="cr:given_name">
     <xsl:value-of select="concat(cr:given_name,' ',cr:surname)"/>
    </xsl:if>
    <xsl:if test="given_name">
     <xsl:value-of select="concat(given_name,' ',surname)"/>
    </xsl:if>
   </tc:editor>
  </xsl:for-each>
 </tc:editors>
 <tc:organization>
  <xsl:value-of select="(cr:organization|organization)[1]"/>
 </tc:organization>
</xsl:template>

</xsl:stylesheet>
