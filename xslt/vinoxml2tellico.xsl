<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:vino="http://www.vinoxml.org/XMLschema"
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
   <xsl:apply-templates select="vino:wine"/>
   <images>
    <xsl:apply-templates select="vino:wine/vino:vintage/vino:bottlelabel/vino:imageembedded"/>
   </images>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="vino:wine">
 <entry>

  <producer>
   <xsl:value-of select="vino:producer/vino:name"/>
  </producer>

  <varietal>
   <!-- The Wine Cellar Book seems to use additionalname
         instead of variety so check for both -->
   <xsl:choose>
    <xsl:when test="string-length(vino:variety) &gt; 0">
     <xsl:value-of select="vino:variety"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="vino:additionalname"/>
    </xsl:otherwise>
   </xsl:choose>
  </varietal>

  <vintage>
   <xsl:value-of select="vino:vintage/vino:year"/>
  </vintage>

  <appellation>
   <xsl:value-of select="vino:origin/vino:wineregion"/>
  </appellation>

  <country>
   <xsl:value-of select="vino:origin/vino:country/vino:name"/>
  </country>

  <type i18n="true">
   <xsl:choose>
    <xsl:when test="vino:winetype/vino:winetype = 'Red wine'">
     <xsl:text>Red Wine</xsl:text>
    </xsl:when>
    <xsl:when test="vino:winetype/vino:winetype = 'White wine'">
     <xsl:text>White Wine</xsl:text>
    </xsl:when>
    <xsl:when test="contains(vino:winetype/vino:winetype, 'Champagne') or
                    contains(vino:winetype/vino:winetype, 'Sparkling')">
     <xsl:text>Sparkling Wine</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="vino:winetype/vino:winetype"/>
    </xsl:otherwise>
   </xsl:choose>
  </type>

  <label>
   <xsl:choose>
    <xsl:when test="vino:vintage/vino:bottlelabel/vino:imageurl">
     <xsl:value-of select="vino:vintage/vino:bottlelabel/vino:imageurl"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="vino:vintage/vino:bottlelabel/vino:imageembedded/vino:imagefilename"/>
    </xsl:otherwise>
   </xsl:choose>
  </label>

  <pur_date>
   <xsl:value-of select="vino:vintage/vino:bottle/vino:purchases[1]/vino:date"/>
  </pur_date>

  <pur_price>
   <!-- broken, but we'll ignore currency code -->
   <xsl:value-of select="vino:vintage/vino:bottle/vino:price/vino:pricevalue"/>
  </pur_price>

  <!-- for now, the quantitye is just how many bottles are left -->
  <quantity>
   <xsl:value-of select="sum(vino:vintage/vino:bottle/vino:purchases/vino:quantity) - sum(vino:vintage/vino:bottle/vino:consumptions/vino:quantity)"/>
  </quantity>

  <description>
   <xsl:value-of select="concat(vino:notes, '&lt;br/&gt;&lt;br/&gt;', vino:vintage/vino:notes)"/>
  </description>

 </entry>
</xsl:template>

<xsl:template match="vino:imageembedded">
 <image format="{vino:imageformat}" id="{vino:imagefilename}">
  <xsl:value-of select="vino:imagedata"/>
 </image>
</xsl:template>

</xsl:stylesheet>
