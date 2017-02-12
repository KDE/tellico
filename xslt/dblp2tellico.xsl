<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from dblp.org

   Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="DBLP Import" type="5">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select="result/hits/hit/info"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="info">
 <xsl:variable name="type">
  <xsl:choose>
   <xsl:when test="type='Journal Articles'">
    <xsl:text>article</xsl:text>
   </xsl:when>
   <xsl:when test="type='Conference and Workshop Papers'">
    <xsl:text>inproceedings</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="type"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>

 <entry>

  <title>
   <xsl:value-of select="title"/>
  </title>

  <xsl:if test="$type='inproceedings'">
   <booktitle>
    <xsl:value-of select="(booktitle|venue)[1]"/>
   </booktitle>
  </xsl:if>

  <entry-type>
   <xsl:value-of select="$type"/>
  </entry-type>

  <year>
   <xsl:value-of select="year"/>
  </year>

  <pages>
   <xsl:value-of select="pages"/>
  </pages>

  <xsl:if test="$type='article'">
   <journal>
    <xsl:value-of select="(journal|venue)[1]"/>
   </journal>
  </xsl:if>

  <volume>
   <xsl:value-of select="volume"/>
  </volume>

  <number>
   <xsl:value-of select="number"/>
  </number>

  <publishers>
   <publisher>
    <xsl:value-of select="publisher"/>
   </publisher>
  </publishers>

  <url>
   <xsl:value-of select="url"/>
  </url>

  <authors>
   <xsl:for-each select="authors/author">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <bibtex-key>
   <xsl:call-template name="substring-after-last">
    <xsl:with-param name="input" select="url"/>
    <xsl:with-param name="substr" select="'/'"/>
   </xsl:call-template>
  </bibtex-key>

  <!-- just assume DOI starts with 10. -->
  <xsl:variable name="doi" select="substring-after(title/@ee, '/10.')"/>
  <doi>
   <xsl:if test="$doi">
    <xsl:text>10.</xsl:text>
    <xsl:value-of select="$doi"/>
   </xsl:if>
  </doi>

 </entry>

</xsl:template>

<xsl:template name="substring-after-last">
 <xsl:param name="input"/>
 <xsl:param name="substr"/>
 <xsl:variable name="temp" select="substring-after($input, $substr)"/>
 <xsl:choose>
  <xsl:when test="$substr and contains($temp, $substr)">
   <xsl:call-template name="substring-after-last">
    <xsl:with-param name="input" select="$temp"/>
    <xsl:with-param name="substr" select="$substr"/>
   </xsl:call-template>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$temp"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
