<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/bookcase/"
                xmlns:str="http://exslt.org/strings"
                xmlns:bibtexml="http://bibtexml.sourceforge.net/"
                extension-element-prefixes="str"
                exclude-result-prefixes="bibtexml str"
                version="1.0">

<!--
   ================================================================
   Bookcase XSLT file - used for importing from bibtexml format

   $Id: bibtexml2bookcase.xsl,v 1.7 2003/03/15 06:33:31 robby Exp $

   Copyright (c) 2002 Robby Stephenson

   This XSLT stylesheet is designed to be used with XML data files
   from the 'Bookcase' application, which can be found at:
   http://periapsis.org/bookcase/
   ================================================================
-->

<xsl:output method="xml" indent="yes"
            encoding="UTF-8"
            doctype-system="bookcase.dtd"
            doctype-public="bookcase"/>

<xsl:strip-space elements="*"/>

<xsl:variable name="current-syntax" select="'2'"/>

<xsl:variable name="endl">
<xsl:text>
</xsl:text>
</xsl:variable>

<xsl:template match="/">
 <xsl:apply-templates select="bibtexml:file"/>
</xsl:template>

<xsl:template match="bibtexml:file">
 <bookcase version="{$current-syntax}">
 <!-- any way not to have to put defaults here? -->
  <collection unitTitle="Book" title="My Books" unit="book">
  <!-- want to store key (id) values -->
  <!-- type = "5" is BCAttribute::ReadOnly -->
   <attribute name="bibtex-id" type="5" flags="1" description="BibTex ID"/>
   <xsl:apply-templates select="bibtexml:entry/bibtexml:book"/>
  </collection>
 </bookcase>
</xsl:template>

<xsl:template match="bibtexml:book">
 <book>
  <xsl:if test="../@id">
   <bibtex-id>
    <xsl:value-of select="../@id"/>
   </bibtex-id>
  </xsl:if>
  <xsl:apply-templates select="bibtexml:author|bibtexml:title|bibtexml:publisher|bibtexml:year|bibtexml:isbn|bibtexml:lccn|bibtexml:edition|bibtexml:series|bibtexml:number|bibtexml:keywords|bibtexml:languages|bibtexml:pricec|bibtexml:note"/>
 </book>
</xsl:template>

<xsl:template match="bibtexml:title|bibtexml:publisher|bibtexml:isbn|bibtexml:lccn|bibtexml:edition|bibtexml:series|bibtexml:price">
 <xsl:element name="{local-name()}">
  <xsl:value-of select="."/>
 </xsl:element>
</xsl:template>

<xsl:template match="bibtexml:author">
 <authors>
  <xsl:for-each select="str:tokenize(., ';,')">
   <author>
    <xsl:value-of select="normalize-space()"/>
   </author>
  </xsl:for-each>
 </authors>
</xsl:template>

<xsl:template match="bibtexml:year">
 <cr_years>
  <cr_year>
   <xsl:value-of select="."/>
  </cr_year>
 </cr_years>
</xsl:template>

<xsl:template match="bibtexml:number">
 <series_num>
  <xsl:value-of select="."/>
 </series_num>
</xsl:template>

<xsl:template match="bibtexml:keywords">
 <keywords>
  <xsl:for-each select="str:tokenize(., ',')">
   <keyword>
    <xsl:value-of select="normalize-space()"/>
   </keyword>
  </xsl:for-each>
 </keywords>
</xsl:template>

<xsl:template match="bibtexml:languages">
 <languages>
  <xsl:for-each select="str:tokenize(., ',')">
   <language>
    <xsl:value-of select="normalize-space()"/>
   </language>
  </xsl:for-each>
 </languages>
</xsl:template>

<xsl:template match="bibtexml:note">
 <comments>
  <xsl:value-of select="."/>
 </comments>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
