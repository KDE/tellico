<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Delicious Library data.

   Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:param name="item-type"/>

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <!-- DL libraries can contain mixed types and right now, there's no way to do that in Tellico -->
  <!-- so we're going to limit the export to whatever the type of the first item is -->
  <xsl:variable name="item">
   <xsl:choose>
    <xsl:when test="string-length($item-type) &gt; 0">
     <xsl:value-of select="$item-type"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="local-name(library/items/child::*[1])"/>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:variable>

  <xsl:comment><xsl:text>Choosing all items of type </xsl:text><xsl:value-of select="$item"/></xsl:comment>

  <xsl:variable name="type">
   <xsl:choose>
    <xsl:when test="$item='book'">2</xsl:when>
    <xsl:when test="$item='movie'">3</xsl:when>
    <xsl:when test="$item='game'">11</xsl:when>
    <xsl:otherwise>0</xsl:otherwise>
   </xsl:choose>
  </xsl:variable>
  <tc:collection title="Delicious Library Import">
   <xsl:attribute name="type"><xsl:value-of select="$type"/></xsl:attribute>
   <tc:fields>
    <tc:field name="_default"/>
    <tc:field flags="0" title="Amazon Link" category="General" format="4" type="7" name="amazon" i18n="true"/>
    <tc:field flags="0" title="UUID" category="General" format="0" type="1" name="uuid"/>
   </tc:fields>
   <xsl:apply-templates select="library/items/child::*[local-name()=$item]"/>
  </tc:collection>
 </tc:tellico>
</xsl:template>

<xsl:template match="book">
 <tc:entry>
  <tc:uuid>
   <xsl:value-of select="@uuid"/>
  </tc:uuid>

  <tc:amazon>
   <xsl:if test="@asin">
    <xsl:value-of select="concat('http://www.amazon.com/dp/',@asin,'/?tag=tellico-20')"/>
   </xsl:if>
  </tc:amazon>

  <tc:title>
   <xsl:choose>
    <xsl:when test="contains(@title, ':')">
     <xsl:value-of select="substring-before(@title,':')"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="@title"/>
    </xsl:otherwise>
   </xsl:choose>
  </tc:title>

  <tc:subtitle>
   <xsl:value-of select="substring-after(@title,':')"/>
  </tc:subtitle>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:author'"/>
   <xsl:with-param name="value" select="@author"/>
  </xsl:call-template>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:genre'"/>
   <xsl:with-param name="value" select="@genre"/>
   <xsl:with-param name="i18n" select="true()"/>
  </xsl:call-template>

  <tc:publisher>
   <xsl:value-of select="@publisher"/>
  </tc:publisher>

  <tc:isbn>
   <xsl:value-of select="@asin"/>
  </tc:isbn>

  <tc:binding i18n="true">
   <xsl:choose>
    <xsl:when test="contains(@aspect, 'Hardcover')">
     <xsl:text>Hardback</xsl:text>
    </xsl:when>
    <xsl:when test="contains(@aspect, 'Paperback')">
     <xsl:text>Paperback</xsl:text>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="@aspect"/>
    </xsl:otherwise>
   </xsl:choose>
  </tc:binding>

  <tc:pub_year>
   <xsl:call-template name="year">
    <xsl:with-param name="value" select="@published"/>
   </xsl:call-template>
  </tc:pub_year>

  <tc:pages>
   <xsl:value-of select="@pages"/>
  </tc:pages>

  <tc:pur_price>
   <xsl:value-of select="@price"/>
  </tc:pur_price>

  <tc:pur_date>
   <xsl:value-of select="@purchaseDate"/>
  </tc:pur_date>

  <tc:rating>
   <!-- tellico automatically rounds down  -->
   <xsl:value-of select="@netrating"/>
  </tc:rating>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:nationality'"/>
   <xsl:with-param name="value" select="@country"/>
  </xsl:call-template>

  <tc:comments>
   <!-- it gets cleaned up inside of Tellico -->
   <xsl:value-of select="description"/>
  </tc:comments>

 </tc:entry>
</xsl:template>

<xsl:template match="movie">
 <tc:entry>
  <tc:uuid>
   <xsl:value-of select="@uuid"/>
  </tc:uuid>

  <tc:amazon>
   <xsl:if test="@asin">
    <xsl:value-of select="concat('http://www.amazon.com/dp/',@asin,'/?tag=tellico-20')"/>
   </xsl:if>
  </tc:amazon>

  <tc:title>
   <xsl:call-template name="strip-title">
    <xsl:with-param name="title" select="@title"/>
   </xsl:call-template>
  </tc:title>

  <tc:medium>
   <xsl:value-of select="@aspect"/>
  </tc:medium>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:director'"/>
   <xsl:with-param name="value" select="@director"/>
  </xsl:call-template>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:cast'"/>
   <xsl:with-param name="value" select="@stars"/>
  </xsl:call-template>

  <xsl:if test="contains(@features, 'Widescreen')">
   <tc:widescreen>true</tc:widescreen>
  </xsl:if>

  <xsl:if test="contains(@features, 'NTSC')">
   <tc:format i18n="true">NTSC</tc:format>
  </xsl:if>

  <xsl:if test="contains(@features, 'PAL')">
   <tc:format i18n="true">PAL</tc:format>
  </xsl:if>

  <xsl:if test="contains(@features, 'SECAM')">
   <tc:format i18n="true">SECAM</tc:format>
  </xsl:if>

  <xsl:if test="contains(@features, 'Color')">
   <tc:color i18n="true">Color</tc:color>
  </xsl:if>

  <xsl:if test="contains(@features, 'Black &amp; White')">
   <tc:color i18n="true">Black &amp; White</tc:color>
  </xsl:if>

  <xsl:if test="contains(@features, '1.33:1')">
   <tc:aspect-ratios>
    <tc:aspect-ratio>1.33:1</tc:aspect-ratio>
   </tc:aspect-ratios>
  </xsl:if>

  <xsl:if test="contains(@features, '1.85:1')">
   <tc:aspect-ratios>
    <tc:aspect-ratio>1.85:1</tc:aspect-ratio>
   </tc:aspect-ratios>
  </xsl:if>

  <tc:audio-tracks>
   <xsl:if test="contains(@features, 'Dolby')">
    <tc:audio-track i18n="true">Dolby</tc:audio-track>
   </xsl:if>
   <xsl:if test="contains(@features, 'DTS')">
    <tc:audio-track i18n="true">DTS</tc:audio-track>
   </xsl:if>
  </tc:audio-tracks>

  <xsl:choose>
   <xsl:when test="contains(@genre, 'Region 1')">
    <tc:region i18n="true">Region 1</tc:region>
   </xsl:when>
   <xsl:when test="contains(@genre, 'Region 2')">
    <tc:region i18n="true">Region 2</tc:region>
   </xsl:when>
   <xsl:when test="contains(@genre, 'Region 3')">
    <tc:region i18n="true">Region 3</tc:region>
   </xsl:when>
   <xsl:when test="contains(@genre, 'Region 4')">
    <tc:region i18n="true">Region 4</tc:region>
   </xsl:when>
   <xsl:when test="contains(@genre, 'Region 5')">
    <tc:region i18n="true">Region 5</tc:region>
   </xsl:when>
   <xsl:when test="contains(@genre, 'Region 6')">
    <tc:region i18n="true">Region 6</tc:region>
   </xsl:when>
   <xsl:when test="contains(@genre, 'Region 7')">
    <tc:region i18n="true">Region 7</tc:region>
   </xsl:when>
   <xsl:when test="contains(@genre, 'Region 8')">
    <tc:region i18n="true">Region 8</tc:region>
   </xsl:when>
  </xsl:choose>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:genre'"/>
   <xsl:with-param name="value" select="@genre"/>
   <xsl:with-param name="i18n" select="true()"/>
  </xsl:call-template>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:studio'"/>
   <xsl:with-param name="value" select="@publisher"/>
  </xsl:call-template>

  <tc:running-time>
   <xsl:value-of select="@minutes"/>
  </tc:running-time>

  <tc:certification>
   <xsl:value-of select="concat(@mpaarating, ' (US)')"/>
  </tc:certification>

  <tc:year>
   <xsl:call-template name="year">
    <xsl:with-param name="value" select="@theatricalDate"/>
   </xsl:call-template>
  </tc:year>

  <tc:pur_price>
   <xsl:value-of select="@price"/>
  </tc:pur_price>

  <tc:pur_date>
   <xsl:value-of select="@purchaseDate"/>
  </tc:pur_date>

  <tc:rating>
   <!-- tellico automatically rounds down  -->
   <xsl:value-of select="@netrating"/>
  </tc:rating>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:nationality'"/>
   <xsl:with-param name="value" select="@country"/>
  </xsl:call-template>

  <tc:plot>
   <xsl:value-of select="description"/>
  </tc:plot>
 </tc:entry>
</xsl:template>

<xsl:template match="game">
 <tc:entry>
  <tc:uuid>
   <xsl:value-of select="@uuid"/>
  </tc:uuid>

  <tc:amazon>
   <xsl:if test="@asin">
    <xsl:value-of select="concat('http://www.amazon.com/dp/',@asin,'/?tag=tellico-20')"/>
   </xsl:if>
  </tc:amazon>

  <tc:title>
   <xsl:value-of select="@title"/>
  </tc:title>

  <tc:platform>
   <xsl:value-of select="@platform"/>
  </tc:platform>

  <tc:certification>
   <xsl:value-of select="@esrbrating"/>
  </tc:certification>

  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:genre'"/>
   <xsl:with-param name="value" select="@genre"/>
   <xsl:with-param name="i18n" select="true()"/>
  </xsl:call-template>

  <tc:pur_price>
   <xsl:value-of select="@price"/>
  </tc:pur_price>

  <tc:pur_date>
   <xsl:value-of select="@purchaseDate"/>
  </tc:pur_date>

  <tc:year>
   <xsl:call-template name="year">
    <xsl:with-param name="value" select="@published"/>
   </xsl:call-template>
  </tc:year>

  <tc:rating>
   <!-- tellico automatically rounds down  -->
   <xsl:value-of select="@netrating"/>
  </tc:rating>

  <tc:publisher>
   <xsl:value-of select="@publisher"/>
  </tc:publisher>

  <tc:description>
   <xsl:value-of select="description"/>
  </tc:description>

 </tc:entry>
</xsl:template>

<xsl:template name="split">
 <xsl:param name="name"/>
 <xsl:param name="value"/>
 <xsl:param name="i18n" value="false()"/>

 <xsl:element name="{concat($name,'s')}">
  <xsl:for-each select="str:split($value, '&#10;')">
   <xsl:element name="{$name}">

    <xsl:if test="$i18n">
     <xsl:attribute name="i18n">true</xsl:attribute>
    </xsl:if>

    <xsl:value-of select="."/>

   </xsl:element>
  </xsl:for-each>
 </xsl:element>
</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <xsl:variable name="numbers">
  <xsl:value-of select="translate($value, translate($value, '0123456789', ''), '')"/>
 </xsl:variable>
 <!-- assume that Amazon always encodes the date with the 4-digit year last -->
 <xsl:value-of select="substring($numbers, string-length($numbers)-3, 4)"/>
</xsl:template>

<xsl:template name="strip-title">
 <xsl:param name="title"/>
 <xsl:param name="chars" select="'[('"/>
 <xsl:choose>
  <xsl:when test="string-length($chars) = 0">
   <xsl:value-of select="normalize-space($title)"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:call-template name="strip-title">
    <xsl:with-param name="title">
     <xsl:call-template name="str-before">
      <xsl:with-param name="value1" select="$title"/>
      <xsl:with-param name="value2" select="substring($chars,1,1)"/>
     </xsl:call-template>
    </xsl:with-param>
    <xsl:with-param name="chars" select="substring($chars,2)"/>
   </xsl:call-template>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template name="str-before">
 <xsl:param name="value1"/>
 <xsl:param name="value2"/>
 <xsl:choose>
  <xsl:when test="string-length($value2) &gt; 0 and contains($value1, $value2)">
   <xsl:value-of select="substring-before($value1, $value2)"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$value1"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
