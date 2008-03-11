<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Delicious Library data.

   Copyright (C) 2007 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V10.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v10/tellico.dtd"/>

<xsl:template match="/">
 <tc:tellico syntaxVersion="10">
  <tc:collection title="Delicious Library Import" type="2">
   <tc:fields>
    <tc:field name="_default"/>
    <tc:field flags="0" title="Amazon Link" category="General" format="4" type="7" name="amazon" i18n="true"/>
    <tc:field flags="0" title="UUID" category="General" format="0" type="1" name="uuid"/>
   </tc:fields>
   <xsl:for-each select="library/items/book">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
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
   <xsl:value-of select="netrating"/>
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

</xsl:stylesheet>
