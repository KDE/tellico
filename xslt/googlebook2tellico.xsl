<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:atom="http://www.w3.org/2005/Atom"
                xmlns:gbs="http://schemas.google.com/books/2008"
                xmlns:dc="http://purl.org/dc/terms"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing results from Google book search

   Copyright (C) 2011 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V9.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v9/tellico.dtd"/>

<!-- disable default behavior -->
<xsl:template match="text()|@*"></xsl:template>

<xsl:template match="/">
 <tellico syntaxVersion="9">
  <collection title="Google Book Search Results" type="2"> <!-- 2 is books -->
   <fields>
    <field name="_default"/>
    <field flags="0" title="URL" category="General" format="4" type="7" name="url" i18n="true"/>
    <field title="Plot" flags="0" category="Plot" format="4" type="2" name="plot" i18n="true"/>
   </fields>
   <xsl:for-each select="atom:feed/atom:entry">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="atom:entry">
 <entry>
  <title>
   <xsl:value-of select="atom:title"/>
  </title>

  <subtitle>
   <xsl:value-of select="dc:title[2]"/>
  </subtitle>

  <publisher>
   <xsl:value-of select="dc:publisher"/>
  </publisher>

  <authors>
   <xsl:for-each select="dc:creator">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <isbn>
   <xsl:value-of select="substring(dc:identifier[starts-with(., 'ISBN')], 6)"/>
  </isbn>

  <pages>
   <xsl:value-of select="normalize-space(substring-before(dc:format, 'pages'))"/>
  </pages>

  <cover>
   <xsl:value-of select="atom:link[@type='image/jpeg' or @type='image/x-unknown'][1]/@href"/>
  </cover>

  <url>
   <xsl:value-of select="atom:link[@rel='alternate' and @type='text/html']/@href"/>
  </url>

  <pub_year>
   <xsl:call-template name="year">
    <xsl:with-param name="value" select="dc:date"/>
   </xsl:call-template>
  </pub_year>

  <keywords>
   <xsl:for-each select="dc:subject">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
  </keywords>

  <plot>
   <xsl:value-of select="dc:description"/>
  </plot>

 </entry>

</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <!-- assume that the year is first 4 characters -->
 <xsl:value-of select="substring($value, 0, 5)"/>
</xsl:template>

</xsl:stylesheet>
