<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:srw="http://www.loc.gov/zing/srw/"
                xmlns:prism="http://prismstandard.org/namespaces/basic/2.0/"
                xmlns:dc="http://purl.org/dc/elements/1.1/"
                xmlns:dcinfo="info:sru/schema/1/dc-v1.1"
                xmlns:telterms="http://krait.kb.nl/coop/tel/handbook/telterms.html"
                exclude-result-prefixes="srw prism dc dcinfo telterms"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing PRISM/DC data.

   Copyright (C) 2004-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<!-- param to set desired collection type, whether book(2) or bibtex(5) -->
<xsl:param name="ctype" select="'5'"/>

<xsl:variable name="atype">
 <xsl:choose>
  <xsl:when test=".//prism:issn">
   <xsl:text>article</xsl:text>
  </xsl:when>
  <xsl:when test=".//ISBN|.//prism:isbn|.//dc:identifier[@id='isbn']|.//dcinfo:identifier[@id='isbn']">
   <xsl:text>book</xsl:text>
  </xsl:when>
  <xsl:otherwise>
   <xsl:text>article</xsl:text>
  </xsl:otherwise>
 </xsl:choose>
</xsl:variable>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <tc:collection title="Import" type="{$ctype}">
   <tc:fields>
    <tc:field name="_default"/>
    <xsl:if test=".//prism:issn">
     <tc:field flags="0" title="ISSN#" category="Publishing" format="4" type="1" name="issn" description="ISSN#" />
    </xsl:if>
   </tc:fields>
   <xsl:for-each select=".//srw:record">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </tc:collection>
 </tc:tellico>
</xsl:template>

<xsl:template match="srw:record">
 <tc:entry>

  <xsl:if test="$ctype='5'">
   <tc:entry-type>
    <xsl:value-of select="$atype"/>>
   </tc:entry-type>
  </xsl:if>

  <tc:authors>
   <xsl:for-each select=".//dc:creator|.//dcinfo:creator">
    <tc:author>
     <xsl:value-of select="."/>
    </tc:author>
   </xsl:for-each>
  </tc:authors>

  <tc:publishers>
   <xsl:for-each select=".//dc:publisher|.//dcinfo:publisher">
    <tc:publisher>
     <xsl:value-of select="."/>
    </tc:publisher>
   </xsl:for-each>
  </tc:publishers>

  <tc:genres i18n="true">
   <xsl:for-each select=".//prism:genre">
    <tc:genre>
     <xsl:value-of select="."/>
    </tc:genre>
   </xsl:for-each>
  </tc:genres>

  <tc:keywords i18n="true">
   <xsl:for-each select=".//dc:subject|.//dcinfo:subject|.//prism:keyword">
    <tc:keyword>
     <xsl:value-of select="."/>
    </tc:keyword>
   </xsl:for-each>
  </tc:keywords>

  <xsl:apply-templates/>

 </tc:entry>
</xsl:template>

<!-- disable default behavior -->
<xsl:template match="text()|@*"></xsl:template>

<xsl:template match="dc:title|dcinfo:title">
 <tc:title>
  <xsl:value-of select="."/>
 </tc:title>
</xsl:template>

<xsl:template match="dc:description|dcinfo:description">
 <tc:note>
  <xsl:value-of select="."/>
 </tc:note>
</xsl:template>

<xsl:template match="prism:publicationName">
 <tc:journal>
  <xsl:value-of select="."/>
 </tc:journal>
</xsl:template>

<xsl:template match="dc:date|dcinfo:date|prism:publicationDate">
 <!-- the year element for books is pub_year -->
 <xsl:variable name="year">
  <xsl:choose>
  <xsl:when test="$ctype='2'">
   <xsl:text>pub_year</xsl:text>
  </xsl:when>
   <xsl:otherwise>
    <xsl:text>year</xsl:text>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>
 <xsl:element name="{$year}" namespace="http://periapsis.org/tellico/">
  <xsl:call-template name="year">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </xsl:element>
</xsl:template>

<xsl:template match="prism:edition">
 <tc:edition>
  <xsl:value-of select="."/>
 </tc:edition>
</xsl:template>

<!-- ISBN is a particular element of KB with x-fields=ISBN
     See https://www.librarything.com/topic/136014# -->
<xsl:template match="prism:isbn|ISBN|dc:identifier[@id='isbn']|dcinfo:identifier[@id='isbn']">
 <tc:isbn>
  <xsl:value-of select="."/>
 </tc:isbn>
</xsl:template>

<xsl:template match="prism:issn">
 <tc:issn>
  <xsl:value-of select="."/>
 </tc:issn>
</xsl:template>

<xsl:template match="prism:doi">
 <tc:doi>
  <xsl:value-of select="."/>
 </tc:doi>
</xsl:template>

<xsl:template match="prism:volume">
 <tc:volume>
  <xsl:value-of select="."/>
 </tc:volume>
</xsl:template>

<xsl:template match="prism:number">
 <tc:number>
  <xsl:value-of select="."/>
 </tc:number>
</xsl:template>

<xsl:template match="prism:url|telterms:recordIdentifier">
 <tc:url>
  <xsl:value-of select="."/>
 </tc:url>
</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <xsl:variable name="digits">
  <xsl:value-of select="translate($value, translate($value, '0123456789', ''), '')"/>
 </xsl:variable>
 <xsl:variable name="len">
  <xsl:value-of select="string-length($digits)"/>
 </xsl:variable>
 <xsl:choose>
  <!-- return first four digits in value -->
  <xsl:when test="starts-with($digits, '19') or starts-with($digits, '20')">
   <xsl:value-of select="substring($digits, 1, 4)"/>
  </xsl:when>
  <!-- KB returns dc:date as 'Fri Jan 01 01:00:00 CET 1971' for example -->
  <xsl:when test="$len &gt; 5 and (substring($digits, $len - 3, 2) = '19' or substring($digits, $len - 3, 2) = '20')">
   <xsl:value-of select="substring($digits, $len - 3, 4)"/>
  </xsl:when>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
