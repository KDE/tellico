<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:str="http://exslt.org/strings"
                xmlns:cc="uri:country-codes"
                extension-element-prefixes="exsl str cc"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Griffith data.

   Copyright (C) 2012 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:param name="imgdir"/>

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <tc:collection title="Griffith Import" type="3">
   <tc:fields>
    <tc:field name="_default"/>
    <tc:field flags="0" title="Original Title" category="General" format="1" type="1" name="origtitle" i18n="true"/>
    <tc:field flags="0" title="Seen" category="Personal" format="4" type="4" name="seen" i18n="true"/>
   </tc:fields>
   <xsl:apply-templates select="root/movie"/>
  </tc:collection>
 </tc:tellico>
</xsl:template>

<xsl:template match="movie">
 <tc:entry id="{number}">
  <!-- only apply non-empty elements -->
  <xsl:for-each select="*">
   <xsl:if test="normalize-space(.)">
    <xsl:apply-templates select="."/>
   </xsl:if>
  </xsl:for-each>
 </tc:entry>
</xsl:template>

<xsl:template match="title">
 <tc:title>
  <xsl:value-of select="."/>
 </tc:title>
</xsl:template>

<xsl:template match="o_title">
 <tc:origtitle>
  <xsl:value-of select="."/>
 </tc:origtitle>
</xsl:template>

<xsl:template match="year">
 <tc:year>
  <xsl:value-of select="."/>
 </tc:year>
</xsl:template>

<xsl:template match="classification">
 <tc:certification>
  <xsl:choose>
   <xsl:when test="normalize-space(../country)">
    <xsl:value-of select="concat(normalize-space(.),
                                 ' (',
                                 normalize-space(../country),
                                 ')'
                                 )"/>

   </xsl:when>
   <xsl:otherwise>
    <!-- just assume USA -->
    <xsl:value-of select="concat(normalize-space(.), ' (USA)')"/>
   </xsl:otherwise>
  </xsl:choose>
 </tc:certification>
</xsl:template>

<xsl:template match="country">
 <tc:nationalitys>
  <tc:nationality>
   <xsl:value-of select="."/>
  </tc:nationality>
 </tc:nationalitys>
</xsl:template>

<xsl:template match="director">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:director'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="genre">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:genre'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="studio">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:studio'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="rating">
 <tc:rating>
  <xsl:value-of select="."/>
 </tc:rating>
</xsl:template>

<xsl:template match="runtime">
 <tc:running-time>
  <xsl:value-of select="."/>
 </tc:running-time>
</xsl:template>

<xsl:template match="seen">
 <tc:seen>
  <xsl:value-of select="boolean(number(.))"/>
 </tc:seen>
</xsl:template>

<xsl:template match="plot">
 <tc:plot>
  <xsl:value-of select="."/>
 </tc:plot>
</xsl:template>

<xsl:template match="media_name">
 <tc:medium>
  <xsl:value-of select="."/>
 </tc:medium>
</xsl:template>

<xsl:template match="cast">
 <tc:casts>
  <xsl:for-each select="str:split(., '&#xa;')">
   <tc:cast>
    <tc:column>
     <xsl:call-template name="str-before">
      <xsl:with-param name="value1" select="."/>
      <xsl:with-param name="value2" select="'as'"/>
     </xsl:call-template>
    </tc:column>
    <tc:column>
     <xsl:call-template name="str-after">
      <xsl:with-param name="value1" select="."/>
      <xsl:with-param name="value2" select="'as'"/>
     </xsl:call-template>
    </tc:column>
   </tc:cast>
  </xsl:for-each>
 </tc:casts>
</xsl:template>

<xsl:template match="image">
 <tc:cover>
  <xsl:value-of select="concat('file://', $imgdir, normalize-space(.))"/>
 </tc:cover>
</xsl:template>

<xsl:template name="split">
 <xsl:param name="name"/>
 <xsl:param name="value"/>
 <xsl:param name="i18n" value="false()"/>

 <xsl:element name="{concat($name,'s')}">
  <xsl:for-each select="str:split($value, '/')">
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
 <!-- unless there's a T or a Z indicating an ISO date string, when use first -->
 <xsl:choose>
  <xsl:when test="contains($value, 'T') or contains($value, 'Z')">
   <xsl:value-of select="substring($numbers, 1, 4)"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="substring($numbers, string-length($numbers)-3, 4)"/>
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

<xsl:template name="str-after">
 <xsl:param name="value1"/>
 <xsl:param name="value2"/>
 <xsl:choose>
  <xsl:when test="string-length($value2) &gt; 0 and contains($value1, $value2)">
   <xsl:value-of select="substring-after($value1, $value2)"/>
  </xsl:when>
  <xsl:otherwise>
   <xsl:value-of select="$value1"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<!-- ignore anything not explicit -->
<xsl:template match="*|@*"/>

</xsl:stylesheet>
