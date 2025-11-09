<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Data Crow data.

   Copyright (C) 2022 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:variable name="coll">
 <xsl:choose>
  <xsl:when test="/data-crow-objects//book">
   <xsl:text>2</xsl:text>
  </xsl:when>
  <xsl:when test="/data-crow-objects//movie">
   <xsl:text>3</xsl:text>
  </xsl:when>
  <xsl:when test="/data-crow-objects//music-album">
   <xsl:text>4</xsl:text>
  </xsl:when>
 </xsl:choose>
</xsl:variable>

<!-- ignore anything not explicit -->
<xsl:template match="*"/>

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <xsl:apply-templates select="data-crow-objects"/>
 </tc:tellico>
</xsl:template>

<xsl:template match="data-crow-objects">
 <tc:collection title="Data Crow Import" type="{$coll}">
  <tc:fields>
   <tc:field name="_default"/>
   <xsl:if test=".//book/webpage or .//movie/webpage or .//music-album/webpage">
    <tc:field flags="0" title="URL" category="General" format="4" type="7" name="url" i18n="true"/>
   </xsl:if>
   <xsl:if test=".//container/name">
    <tc:field flags="6" title="Location" category="Personal" format="4" type="1" name="location" i18n="true"/>
   </xsl:if>
   <xsl:if test="$coll='3'">
    <tc:field flags="0" title="Seen" category="Personal" format="4" type="4" name="seen" i18n="true"/>
   </xsl:if>
  </tc:fields>
  <xsl:apply-templates select=".//movie|.//book|.//music-album"/>
 </tc:collection>
</xsl:template>

<xsl:template match="movie|book|music-album">
 <tc:entry>
  <xsl:apply-templates select="@*|*"/>
 </tc:entry>
</xsl:template>

<!-- the easy one matches identical local names -->
<xsl:template match="title|color|series|pages">
 <xsl:element name="{concat('tc:',local-name())}">
  <xsl:value-of select="."/>
 </xsl:element>
</xsl:template>

<xsl:template match="color-items">
 <tc:color><xsl:value-of select="color[1]/name"/></tc:color>
</xsl:template>

<xsl:template match="aspect-ratio">
 <tc:aspect-ratio><xsl:value-of select="."/></tc:aspect-ratio>
 <xsl:variable name="values" select="str:tokenize(., ':')"/>
 <xsl:if test="100*$values[1] div $values[2] &gt; 134">
  <tc:widescreen>true</tc:widescreen>
 </xsl:if>
</xsl:template>

<xsl:template match="aspect-ratio-items">
 <tc:aspect-ratio><xsl:value-of select="aspect-ratio[1]/name"/></tc:aspect-ratio>
 <xsl:variable name="values" select="str:tokenize(aspect-ratio[1]/name, ':')"/>
 <xsl:if test="100*$values[1] div $values[2] &gt; 134">
  <tc:widescreen>true</tc:widescreen>
 </xsl:if>
</xsl:template>

<xsl:template match="playlength">
 <xsl:if test="contains(., ':')">
  <xsl:variable name="values" select="str:tokenize(., ':')"/>
  <tc:running-time>
   <xsl:value-of select="60*$values[1] + $values[2]"/>
  </tc:running-time>
 </xsl:if>
 <xsl:if test="not(contains(., ':'))">
  <tc:running-time>
   <xsl:value-of select=". div 60"/>
  </tc:running-time>
 </xsl:if>
</xsl:template>

<xsl:template match="binding|binding-items">
 <xsl:variable name="binding" select="binding/name | text()[not(parent::*[child::*])]"/>
 <tc:binding i18n="true">
  <xsl:choose>
   <xsl:when test="contains($binding, 'Paperback')">
    <xsl:text>Paperback</xsl:text>
   </xsl:when>
   <xsl:when test="contains($binding, 'Hardcover')">
    <xsl:text>Hardback</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="."/>
   </xsl:otherwise>
  </xsl:choose>
 </tc:binding>
</xsl:template>

<xsl:template match="countries|countries-items">
 <tc:nationalitys>
  <xsl:for-each select="country">
   <tc:nationality i18n="true">
    <xsl:value-of select="name"/>
   </tc:nationality>
  </xsl:for-each>
 </tc:nationalitys>
</xsl:template>

<xsl:template match="actors|actors-items">
 <tc:casts>
  <xsl:for-each select="actor">
   <tc:cast>
    <tc:column>
     <xsl:value-of select="name"/>
    </tc:column>
   </tc:cast>
  </xsl:for-each>
 </tc:casts>
</xsl:template>

<xsl:template match="directors|directors-items">
 <tc:directors>
  <xsl:for-each select="director">
   <tc:director>
    <xsl:value-of select="name"/>
   </tc:director>
  </xsl:for-each>
 </tc:directors>
</xsl:template>

<xsl:template match="authors|authors-items">
 <tc:authors>
  <xsl:for-each select="author">
   <tc:author>
    <xsl:value-of select="name"/>
   </tc:author>
  </xsl:for-each>
 </tc:authors>
</xsl:template>

<xsl:template match="artists|artists-items">
 <tc:artists>
  <xsl:for-each select="artist">
   <tc:artist>
    <xsl:value-of select="name"/>
   </tc:artist>
  </xsl:for-each>
 </tc:artists>
</xsl:template>

<xsl:template match="translator">
 <tc:translators>
  <tc:translator>
   <xsl:value-of select="."/>
  </tc:translator>
 </tc:translators>
</xsl:template>

<xsl:template match="languages|languages-items">
 <tc:languages>
  <xsl:for-each select="language">
   <tc:language>
    <xsl:value-of select="name"/>
   </tc:language>
  </xsl:for-each>
 </tc:languages>
</xsl:template>

<xsl:template match="subtitle-languages|subtitle-languages-items">
 <tc:subtitles>
  <xsl:for-each select="language">
   <tc:subtitle>
    <xsl:value-of select="name"/>
   </tc:subtitle>
  </xsl:for-each>
 </tc:subtitles>
</xsl:template>

<xsl:template match="isbn-10">
 <tc:isbn><xsl:value-of select="."/></tc:isbn>
</xsl:template>

<xsl:template match="edition-type|edition-type-items">
 <tc:edition>
  <xsl:value-of select="edition-type/name | text()[not(parent::*[child::*])]"/>
 </tc:edition>
</xsl:template>

<xsl:template match="description">
 <xsl:choose>
 <xsl:when test="$coll = '2' or $coll = '3'">
  <tc:plot><xsl:value-of select="."/></tc:plot>
 </xsl:when>
 <xsl:when test="$coll = '4'">
  <tc:comments><xsl:value-of select="."/></tc:comments>
 </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="comment">
 <tc:comments><xsl:value-of select="."/></tc:comments>
</xsl:template>

<xsl:template match="container|container-items">
 <tc:location>
  <xsl:value-of select="container/name | text()[not(parent::*[child::*])]"/>
 </tc:location>
</xsl:template>

<xsl:template match="picture-front|picture-01">
 <tc:cover>
  <xsl:value-of select="."/>
 </tc:cover>
</xsl:template>

<xsl:template match="state">
 <xsl:if test=". = 'Seen'">
  <tc:seen>true</tc:seen>
 </xsl:if>
 <xsl:if test=". = 'Read'">
  <tc:read>true</tc:read>
 </xsl:if>
</xsl:template>

<xsl:template match="state-items">
 <xsl:if test="state[1]/name = 'Seen'">
  <tc:seen>true</tc:seen>
 </xsl:if>
 <xsl:if test="state[1]/name = 'Read'">
  <tc:read>true</tc:read>
 </xsl:if>
</xsl:template>

<xsl:template match="webpage">
 <tc:url><xsl:value-of select="."/></tc:url>
</xsl:template>

<xsl:template match="rating">
 <!-- normalize to 5 stars -->
 <xsl:variable name="values" select="str:tokenize(., '/')"/>
 <xsl:variable name="rating" select="normalize-space($values[1])"/>
 <xsl:variable name="outof" select="normalize-space($values[2])"/>
 <xsl:choose>
  <xsl:when test="$outof &gt; 0 and $rating &gt; 0">
   <tc:rating><xsl:value-of select="floor($rating * 5 div $outof)"/></tc:rating>
  </xsl:when>
  <xsl:when test="$rating &gt; 0">
   <!-- assume out of 10 -->
   <tc:rating><xsl:value-of select="floor($rating div 2)"/></tc:rating>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="year">
 <xsl:choose>
  <xsl:when test="$coll = '2'">
   <tc:pub_year><xsl:value-of select="."/></tc:pub_year>
  </xsl:when>
  <xsl:otherwise>
   <tc:year><xsl:value-of select="."/></tc:year>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="genres|genres-items">
 <tc:genres>
  <xsl:choose>
   <xsl:when test="$coll = '4'">
    <xsl:for-each select="music-genre">
     <tc:genre><xsl:value-of select="name"/></tc:genre>
    </xsl:for-each>
   </xsl:when>
   <xsl:otherwise>
    <xsl:for-each select="genre">
     <tc:genre><xsl:value-of select="name"/></tc:genre>
    </xsl:for-each>
   </xsl:otherwise>
  </xsl:choose>
 </tc:genres>
</xsl:template>

<xsl:template match="created">
 <xsl:variable name="numbers" select="str:tokenize(., '/-')"/>
 <xsl:if test="count($numbers) = 3">
  <tc:cdate calendar="gregorian">
   <tc:year><xsl:value-of select="$numbers[1]"/></tc:year>
   <tc:month><xsl:value-of select="$numbers[2]"/></tc:month>
   <tc:day><xsl:value-of select="$numbers[3]"/></tc:day>
  </tc:cdate>
 </xsl:if>
</xsl:template>

<xsl:template match="modified">
 <xsl:variable name="numbers" select="str:tokenize(., '/-')"/>
 <xsl:if test="count($numbers) = 3">
  <tc:mdate calendar="gregorian">
   <tc:year><xsl:value-of select="$numbers[1]"/></tc:year>
   <tc:month><xsl:value-of select="$numbers[2]"/></tc:month>
   <tc:day><xsl:value-of select="$numbers[3]"/></tc:day>
  </tc:mdate>
 </xsl:if>
</xsl:template>

<xsl:template match="music-tracks|music-track-children">
 <tc:tracks>
  <xsl:for-each select="music-track">
   <tc:track>
    <tc:column>
     <xsl:value-of select="title"/>
    </tc:column>
    <tc:column>
     <xsl:value-of select="(artists-items/artist[1]/name|artists/artist[1]/name|../../artists/artist[1]/name)[1]"/>
    </tc:column>
    <tc:column>
     <xsl:choose>
     <xsl:when test="starts-with(playlength, '0:')">
      <xsl:value-of select="substring(playlength,3)"/>
     </xsl:when>
     <xsl:when test="not(contains(playlength, ':'))">
      <xsl:value-of select="concat(format-number(floor(playlength div 60), '00'),
                                   ':',
                                   format-number(playlength mod 60, '00'))"/>
     </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="playlength"/>
      </xsl:otherwise>
     </xsl:choose>
    </tc:column>
   </tc:track>
  </xsl:for-each>
 </tc:tracks>
</xsl:template>
 
<xsl:template match="storage-medium|storage-medium-items">
 <xsl:variable name="medium" select="storage-medium/name | text()[not(parent::*[child::*])]"/>
 <tc:medium i18n="true">
  <xsl:choose>
   <xsl:when test="contains($medium, 'CD')">
    <xsl:text>Compact Disc</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="$medium"/>
   </xsl:otherwise>
  </xsl:choose>
 </tc:medium>
</xsl:template>

<xsl:template match="record-label|record-label-items">
 <tc:labels>
  <tc:label>
   <xsl:value-of select="record-label/name | text()[not(parent::*[child::*])]"/>
  </tc:label>
 </tc:labels>
</xsl:template>

<xsl:template match="publishers|publishers-items">
 <tc:publishers>
  <xsl:for-each select="publisher">
   <tc:publisher>
    <xsl:value-of select="name | text()[not(parent::*[child::*])]"/>
   </tc:publisher>
  </xsl:for-each>
 </tc:publishers>
</xsl:template>

<xsl:template match="tags|tags-items">
 <tc:keywords>
  <xsl:for-each select="tag">
   <tc:keyword>
    <xsl:value-of select="name"/>
   </tc:keyword>
  </xsl:for-each>
 </tc:keywords>
</xsl:template>

<xsl:template match="purchaseprice|cost">
 <tc:pur_price><xsl:value-of select="."/></tc:pur_price>
</xsl:template>

<xsl:template name="year">
 <xsl:param name="value"/>
 <!-- want to find a 4-digit number to treat as the year -->
 <xsl:variable name="nondigits" select="translate($value,'0123456789','')"/>
  <xsl:choose>

   <xsl:when test="string-length($nondigits) = 0">
    <xsl:if test="string-length($value) = 4">
     <xsl:value-of select="."/>
    </xsl:if>
   </xsl:when>

   <xsl:otherwise>
    <xsl:for-each select="str:tokenize($value,$nondigits)">
     <xsl:if test="string-length() = 4">
      <xsl:value-of select="."/>
     </xsl:if>
    </xsl:for-each>
   </xsl:otherwise>

  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
