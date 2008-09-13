<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:math="http://exslt.org/math"
                xmlns:a="uri:attribute"
                exclude-result-prefixes="tc a"
                extension-element-prefixes="math"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for exporting to GCstar

   Copyright (C) 2008 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at
   http://www.periapsis.org/tellico/

   ===================================================================
-->

<!-- the mapping from gcstar attribute to tellico element is automated here -->
<!-- @name is the gcstar attribute name, the value is the tellico element local-name() -->
<!-- bool attributes are special, and some only apply to certain collection types -->
<a:attributes>
 <a:attribute name="isbn">isbn</a:attribute>
  <a:attribute name="title">title</a:attribute>
  <a:attribute name="publisher">publisher</a:attribute>
  <a:attribute name="publication">pub_year</a:attribute>
  <a:attribute name="language">language</a:attribute>
  <a:attribute name="serie">series</a:attribute>
  <a:attribute name="edition">edition</a:attribute>
  <a:attribute name="pages">pages</a:attribute>
  <a:attribute name="added">pur_date</a:attribute>
  <a:attribute name="acquisition">pur_date</a:attribute>
  <a:attribute name="location">location</a:attribute>
  <a:attribute name="translator">translator</a:attribute>
  <a:attribute name="artist">artist</a:attribute>
  <a:attribute name="director">director</a:attribute>
  <a:attribute name="date">year</a:attribute>
  <a:attribute name="video">format</a:attribute>
  <a:attribute name="original">origtitle</a:attribute>
  <a:attribute name="format">binding</a:attribute>
  <a:attribute name="format">medium</a:attribute>
  <a:attribute name="format">format</a:attribute>
  <a:attribute name="web">url</a:attribute>
  <a:attribute name="webPage">url</a:attribute>
  <a:attribute name="read" format="bool" type="GCbooks">read</a:attribute>
  <a:attribute name="seen" format="bool" type="GCfilms">seen</a:attribute>
  <a:attribute name="favourite" format="bool">favorite</a:attribute>
  <a:attribute name="label">label</a:attribute>
  <a:attribute name="release">year</a:attribute>
  <a:attribute name="composer">composer</a:attribute>
  <a:attribute name="producer">producer</a:attribute>
</a:attributes>
<xsl:variable name="collType">
 <xsl:choose>
  <xsl:when test="tc:tellico/tc:collection/@type=2 or tc:tellico/tc:collection/@type=5">
   <xsl:text>GCbooks</xsl:text>
  </xsl:when>
  <xsl:when test="tc:tellico/tc:collection/@type=3">
   <xsl:text>GCfilms</xsl:text>
  </xsl:when>
  <xsl:when test="tc:tellico/tc:collection/@type=4">
   <xsl:text>GCmusics</xsl:text>
  </xsl:when>
  <xsl:when test="tc:tellico/tc:collection/@type=8">
   <xsl:text>GCcoins</xsl:text>
  </xsl:when>
 </xsl:choose>
</xsl:variable>
<!-- grab all the applicable attributes once -->
<xsl:variable name="attributes" select="document('')/*/a:attributes/a:attribute[not(@type) or @type=$collType]"/>

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <xsl:apply-templates select="tc:collection"/>
</xsl:template>

<xsl:template match="tc:collection[@type&lt;2 or @type&gt;5 and not(@type=8)]">
 <xsl:message terminate="yes">
  <xsl:text>GCstar export is not supported for this collection type.</xsl:text>
 </xsl:message>
</xsl:template>

<xsl:template match="tc:collection[@type&gt;1 and @type&lt;6]">
 <collection items="{count(tc:entry)}" type="{$collType}">
  <information>
   <maxId>
    <xsl:value-of select="math:max(tc:entry/@id)"/>
   </maxId>
  </information>
  <xsl:apply-templates select="tc:entry"/>
 </collection>
</xsl:template>

<!-- no output for fields or images -->
<xsl:template match="tc:fields"/>
<xsl:template match="tc:images"/>

<xsl:template match="tc:entry">
 <xsl:variable name="entry" select="."/>
 <item id="{@id}" rating="{tc:rating * 2}">
  <xsl:for-each select="$attributes">
   <xsl:call-template name="handle-attribute">
    <xsl:with-param name="att" select="."/>
    <xsl:with-param name="entry" select="$entry"/>
   </xsl:call-template>
  </xsl:for-each>

  <xsl:if test="tc:running-time">
   <xsl:attribute name="time">
    <xsl:value-of select="concat(tc:running-time, ' min')"/>
   </xsl:attribute>
  </xsl:if>
  <xsl:if test="tc:nationalitys">
   <xsl:attribute name="country">
    <xsl:for-each select="tc:nationalitys/tc:nationality">
     <xsl:value-of select="."/>
     <xsl:if test="position() &lt; last()">
      <xsl:text> / </xsl:text>
     </xsl:if>
    </xsl:for-each>
   </xsl:attribute>
  </xsl:if>
  <xsl:if test="tc:certification">
   <xsl:attribute name="age">
    <xsl:choose>
     <xsl:when test="tc:certification = 'U (USA)'">
      <xsl:text>1</xsl:text>
     </xsl:when>
     <xsl:when test="tc:certification = 'G (USA)'">
      <xsl:text>2</xsl:text>
     </xsl:when>
     <xsl:when test="tc:certification = 'PG (USA)'">
      <xsl:text>5</xsl:text>
     </xsl:when>
     <xsl:when test="tc:certification = 'PG-13 (USA)'">
      <xsl:text>13</xsl:text>
     </xsl:when>
     <xsl:when test="tc:certification = 'R (USA)'">
      <xsl:text>18</xsl:text>
     </xsl:when>
    </xsl:choose>
   </xsl:attribute>
  </xsl:if>
  <!-- for coin grade, GCstar uses numbers only -->
  <xsl:if test="tc:grade">
   <xsl:attribute name="condition">
    <!-- remove everything but numbers -->
    <xsl:value-of select="translate(tc:grade,translate(tc:grade,'0123456789', ''),'')"/>
   </xsl:attribute>
  </xsl:if>

  <!-- for books -->
  <comments>
   <xsl:value-of select="tc:comments"/>
  </comments>
  <authors>
   <xsl:call-template name="multiline">
    <xsl:with-param name="elem" select="tc:authors"/>
   </xsl:call-template>
  </authors>
  <genre>
   <xsl:call-template name="multiline">
    <xsl:with-param name="elem" select="tc:genres"/>
   </xsl:call-template>
  </genre>
  <tags>
   <xsl:call-template name="multiline">
    <xsl:with-param name="elem" select="tc:keywords"/>
   </xsl:call-template>
  </tags>

  <!-- for movies -->
  <comment> <!-- note the lack of an 's' -->
   <xsl:value-of select="tc:comments"/>
  </comment>
  <synopsis>
   <xsl:value-of select="tc:plot"/>
  </synopsis>
<!--
  <directors>
   <xsl:call-template name="multiline">
    <xsl:with-param name="elem" select="tc:directors"/>
   </xsl:call-template>
  </directors>
-->
  <actors>
   <xsl:call-template name="table">
    <xsl:with-param name="elem" select="tc:casts"/>
   </xsl:call-template>
  </actors>
  <subt>
   <xsl:call-template name="multiline">
    <xsl:with-param name="elem" select="tc:subtitles"/>
   </xsl:call-template>
  </subt>
  <xsl:apply-templates select="tc:languages"/>

  <!-- for music -->
  <xsl:apply-templates select="tc:tracks"/>

 </item>
</xsl:template>

<xsl:template match="tc:languages">
 <audio>
  <xsl:for-each select="tc:language">
   <line>
    <col>
     <xsl:value-of select="."/>
    </col>
    <col>
     <!-- expect a language to always have a track -->
     <xsl:value-of select="../../tc:audio-tracks/tc:audio-track[position()]"/>
    </col>
   </line>
  </xsl:for-each>
 </audio>
</xsl:template>

<xsl:template match="tc:tracks">
 <tracks>
  <xsl:for-each select="tc:track">
   <line>
    <col>
     <xsl:value-of select="position()"/>
    </col>
    <col>
     <xsl:value-of select="tc:column[1]"/>
    </col>
    <col>
     <xsl:value-of select="tc:column[3]"/>
    </col>
   </line>
  </xsl:for-each>
 </tracks>
</xsl:template>

<xsl:template name="multiline">
 <xsl:param name="elem"/>
 <xsl:for-each select="$elem/child::*">
  <line>
   <col>
    <xsl:value-of select="."/>
   </col>
  </line>
 </xsl:for-each>
</xsl:template>

<xsl:template name="table">
 <xsl:param name="elem"/>
 <xsl:for-each select="$elem/child::*">
  <line>
   <xsl:for-each select="child::*">
    <col>
     <xsl:value-of select="."/>
    </col>
   </xsl:for-each>
  </line>
 </xsl:for-each>
</xsl:template>

<xsl:template name="handle-attribute">
 <xsl:param name="att"/>
 <xsl:param name="entry"/>
 <!-- should technically check namespace, too, but unlikely to match -->
 <xsl:variable name="value" select="$entry//*[local-name()=$att][1]"/>
 <xsl:choose>
  <xsl:when test="$att/@format='bool'">
   <xsl:attribute name="{$att/@name}">
    <xsl:value-of select="number($value='true')"/>
   </xsl:attribute>
  </xsl:when>
  <xsl:otherwise>
   <xsl:if test="string-length($value) &gt; 0">
    <xsl:attribute name="{$att/@name}">
     <xsl:value-of select="$value"/>
    </xsl:attribute>
   </xsl:if>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

</xsl:stylesheet>
<!-- Local Variables: -->
<!-- sgml-indent-step: 1 -->
<!-- sgml-indent-data: 1 -->
<!-- End: -->
