<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data Referencer

   Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:key name="tags" match="/library/taglist/tag" use="uid"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Referencer Import" type="5">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select="library/doclist/doc"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="doc">
 <entry>

  <title>
   <xsl:value-of select="bib_title"/>
  </title>

  <entry-type>
   <xsl:value-of select="translate(bib_type, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',
                                             'abcdefghijklmnopqrstuvwxyz')"/>
  </entry-type>

  <bibtex-key>
   <xsl:value-of select="key"/>
  </bibtex-key>

  <year>
   <xsl:value-of select="bib_year"/>
  </year>

  <doi>
   <xsl:value-of select="bib_doi"/>
  </doi>

  <pages>
   <xsl:value-of select="bib_pages"/>
  </pages>

  <journal>
   <xsl:value-of select="bib_journal"/>
  </journal>

  <number>
   <xsl:value-of select="bib_number"/>
  </number>

  <volume>
   <xsl:value-of select="bib_volume"/>
  </volume>

  <url>
   <xsl:value-of select="filename"/>
  </url>

  <authors>
   <xsl:for-each select="str:tokenize(bib_authors, '/;')">
    <xsl:call-template name="author_split">
     <xsl:with-param name="value" select="."/>
    </xsl:call-template>
   </xsl:for-each>
  </authors>

  <keywords>
   <xsl:for-each select="tagged">
    <keyword i18n="true">
     <xsl:value-of select="key('tags', .)/name"/>
    </keyword>
   </xsl:for-each>
  </keywords>

</entry>

</xsl:template>

<xsl:template name="author_split">
 <xsl:param name="value"/>
 <xsl:if test="string-length($value) &gt; 0">
  <xsl:variable name="before" select="substring-before($value, ' and ')"/>
  <author>
   <xsl:if test="string-length($before) &gt; 0">
    <xsl:value-of select="normalize-space($before)"/>
   </xsl:if>
   <xsl:if test="string-length($before) = 0">
    <xsl:value-of select="normalize-space($value)"/>
   </xsl:if>
  </author>
  <xsl:call-template name="author_split">
   <xsl:with-param name="value" select="substring-after($value, ' and ')"/>
  </xsl:call-template>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
