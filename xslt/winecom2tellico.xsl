<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from wine.com

   Copyright (C) 2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Wine.com Import" type="7">
   <fields>
    <field name="_default"/>
    <field flags="0" title="URL" category="General" format="4" type="7" name="url" i18n="true"/>
   </fields>
   <xsl:apply-templates select="Catalog/Products/List/Product[Type='Wine']"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="Product">
 <entry>

  <appellation>
   <xsl:value-of select="Appellation/Name"/>
  </appellation>

  <url>
   <xsl:value-of select="Url"/>
  </url>

  <vintage>
   <xsl:value-of select="Vintage"/>
   <xsl:if test="string-length(Vintage) = 0">
     <!-- guess and grab first 4 digits in name -->
    <xsl:value-of select="substring(translate(Name, translate(Name, '0123456789', ''), ''), 0, 5)"/>
   </xsl:if>
  </vintage>

  <producer>
   <xsl:value-of select="Vineyard/Name"/>
  </producer>

  <varietal>
   <xsl:value-of select="Varietal/Name"/>
  </varietal>

  <description>
   <xsl:value-of select="Description"/>
  </description>

  <type i18n="true">
   <xsl:choose>
    <xsl:when test="Varietal/WineType/Id = 123">
     <xsl:text>Sparkling Wine</xsl:text>
    </xsl:when>
    <xsl:when test="Varietal/WineType/Id = 124">
     <xsl:text>Red Wine</xsl:text>
    </xsl:when>
    <xsl:when test="Varietal/WineType/Id = 125">
     <xsl:text>White Wine</xsl:text>
    </xsl:when>
   </xsl:choose>
  </type>

  <keywords>
   <xsl:for-each select="ProductAttributes/ProductAttribute">
    <keyword>
     <xsl:value-of select="Name"/>
    </keyword>
   </xsl:for-each>
  </keywords>

  <label>
   <xsl:value-of select="Labels/Label[1]/Url"/>
  </label>

 </entry>
</xsl:template>

<xsl:template name="token-union">
 <xsl:param name="tokens"/>
 <xsl:choose>
  <xsl:when test="not($tokens)"/>
  <xsl:otherwise>
   <xsl:copy-of select="exsl:node-set(str:tokenize($tokens[1], ','))"/>
   <xsl:call-template name="token-union">
    <xsl:with-param name="tokens" select="$tokens[position() &gt; 1]"/>
   </xsl:call-template>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
