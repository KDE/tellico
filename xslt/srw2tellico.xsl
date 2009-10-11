<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:srw="http://www.loc.gov/zing/srw/"
                xmlns:prism="http://prismstandard.org/namespaces/basic/2.0/"
                xmlns:dc="http://purl.org/dc/elements/1.1/"
                exclude-result-prefixes="srw prism dc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing PRISM/DC data.

   Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Import" type="5">
   <fields>
    <field name="_default"/>
    <xsl:if test=".//prism:issn">
     <field flags="0" title="ISSN#" category="Publishing" format="4" type="1" name="issn" description="ISSN#" />
    </xsl:if>
   </fields>
   <xsl:for-each select=".//srw:record">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="srw:record">
 <entry>

  <entry-type>
   <xsl:choose>
    <xsl:when test=".//prism:issn">
     <xsl:text>article</xsl:text>
    </xsl:when>
    <xsl:when test=".//prism:isbn">
     <xsl:text>book</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:text>article</xsl:text>
    </xsl:otherwise>
   </xsl:choose>
  </entry-type>

  <authors>
   <xsl:for-each select=".//dc:creator">
    <author>
     <xsl:value-of select="."/>
    </author>
   </xsl:for-each>
  </authors>

  <genres i18n="true">
   <xsl:for-each select=".//prism:genre">
    <genre>
     <xsl:value-of select="."/>
    </genre>
   </xsl:for-each>
  </genres>

  <keywords i18n="true">
   <xsl:for-each select=".//dc:subject|.//prism:keyword">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
  </keywords>

  <xsl:apply-templates/>

 </entry>
</xsl:template>

<!-- disable default behavior -->
<xsl:template match="text()|@*"></xsl:template>

<xsl:template match="dc:title">
 <title>
  <xsl:value-of select="."/>
 </title>
</xsl:template>

<xsl:template match="dc:publisher">
 <publisher>
  <xsl:value-of select="."/>
 </publisher>
</xsl:template>

<xsl:template match="dc:description">
 <note>
  <xsl:value-of select="."/>
 </note>
</xsl:template>

<xsl:template match="prism:publicationName">
 <journal>
  <xsl:value-of select="."/>
 </journal>
</xsl:template>

<xsl:template match="prism:publicationDate">
 <year>
  <xsl:call-template name="year">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </year>
</xsl:template>

<xsl:template match="prism:edition">
 <edition>
  <xsl:value-of select="."/>
 </edition>
</xsl:template>

<xsl:template match="prism:isbn">
 <isbn>
  <xsl:value-of select="."/>
 </isbn>
</xsl:template>

<xsl:template match="prism:issn">
 <issn>
  <xsl:value-of select="."/>
 </issn>
</xsl:template>

<xsl:template match="prism:doi">
 <doi>
  <xsl:value-of select="."/>
 </doi>
</xsl:template>

<xsl:template match="prism:volume">
 <volume>
  <xsl:value-of select="."/>
 </volume>
</xsl:template>

<xsl:template match="prism:number">
 <number>
  <xsl:value-of select="."/>
 </number>
</xsl:template>

<xsl:template match="prism:url">
 <url>
  <xsl:value-of select="."/>
 </url>
</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <!-- return first four digits in value -->
 <xsl:value-of select="substring(translate($value, translate($value, '0123456789', ''), ''), 0, 5)"/>
</xsl:template>

</xsl:stylesheet>
