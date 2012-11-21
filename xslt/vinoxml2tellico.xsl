<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing VinoXML data

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
  <collection title="VinoXML Import" type="7">
   <fields>
    <field name="_default"/>
   </fields>
   <xsl:apply-templates select="wine|wineCollection/collectionitem"/>
   <images>
    <xsl:apply-templates select="wine/vintage/bottlelabel/imageembedded"/>
   </images>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="wine|collectionitem">
 <entry>

  <producer>
   <xsl:value-of select="producer/name"/>
  </producer>

  <varietal>
   <!-- The Wine Cellar Book seems to use additionalname
         instead of variety so check for both -->
   <xsl:choose>
    <xsl:when test="string-length(variety) &gt; 0">
     <xsl:value-of select="variety"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="additionalname"/>
    </xsl:otherwise>
   </xsl:choose>
  </varietal>

  <vintage>
   <xsl:value-of select="vintage/year"/>
  </vintage>

  <appellation>
   <xsl:value-of select="origin/wineregion"/>
  </appellation>

  <country>
   <xsl:value-of select="origin/country/name"/>
  </country>

  <type i18n="true">
   <xsl:choose>
    <xsl:when test="winetype/winetype = 'Red wine'">
     <xsl:text>Red Wine</xsl:text>
    </xsl:when>
    <xsl:when test="winetype/winetype = 'White wine'">
     <xsl:text>White Wine</xsl:text>
    </xsl:when>
    <xsl:when test="contains(winetype/winetype, 'Champagne') or
                    contains(winetype/winetype, 'Sparkling')">
     <xsl:text>Sparkling Wine</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="winetype/winetype"/>
    </xsl:otherwise>
   </xsl:choose>
  </type>

  <label>
   <xsl:choose>
    <xsl:when test="vintage/bottlelabel/imageurl">
     <xsl:value-of select="vintage/bottlelabel/imageurl"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="vintage/bottlelabel/imageembedded/imagefilename"/>
    </xsl:otherwise>
   </xsl:choose>
  </label>

  <pur_date>
   <xsl:value-of select="vintage/bottle/purchases[1]/date"/>
  </pur_date>

  <pur_price>
   <!-- broken, but we'll ignore currency code -->
   <xsl:value-of select="vintage/bottle/price/pricevalue"/>
  </pur_price>

  <!-- for now, the quantitye is just how many bottles are left -->
  <quantity>
   <xsl:value-of select="sum(vintage/bottle/purchases/quantity) - sum(vintage/bottle/consumptions/quantity)"/>
  </quantity>

  <description>
   <xsl:value-of select="concat(notes, '&lt;br/&gt;&lt;br/&gt;', vintage/notes)"/>
  </description>

 </entry>
</xsl:template>

<xsl:template match="imageembedded">
 <image format="{imageformat}" id="{imagefilename}">
  <xsl:value-of select="imagedata"/>
 </image>
</xsl:template>

</xsl:stylesheet>
