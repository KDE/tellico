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

<!-- base url for data crow collection -->
<xsl:param name="baseDir" select="''"/>

<xsl:variable name="coll">
 <xsl:choose>
  <xsl:when test="/data-crow-objects/book">
   <xsl:text>2</xsl:text>
  </xsl:when>
  <xsl:when test="/data-crow-objects/movie">
   <xsl:text>3</xsl:text>
  </xsl:when>
  <xsl:when test="/data-crow-objects/music-album">
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
   <xsl:if test="book/webpage or movie/webpage or music-album/webpage">
    <tc:field flags="0" title="URL" category="General" format="4" type="7" name="url" i18n="true"/>
   </xsl:if>
   <xsl:if test="*/container/*/name">
    <tc:field flags="6" title="Location" category="Personal" format="4" type="1" name="location" i18n="true"/>
   </xsl:if>
   <xsl:if test="$coll='3'">
    <tc:field flags="0" title="Seen" category="Personal" format="4" type="4" name="seen" i18n="true"/>
   </xsl:if>
  </tc:fields>
  <xsl:apply-templates select="movie|book|music-album"/>
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

<xsl:template match="aspect-ratio">
 <tc:aspect-ratio><xsl:value-of select="."/></tc:aspect-ratio>
 <xsl:variable name="values" select="str:tokenize(., ':')"/>
 <xsl:if test="100*$values[1] div $values[2] &gt; 134">
  <tc:widescreen>true</tc:widescreen>
 </xsl:if>
</xsl:template>

<xsl:template match="playlength">
 <xsl:variable name="values" select="str:tokenize(., ':')"/>
 <tc:running-time>
  <xsl:value-of select="60*$values[1] + $values[2]"/>
 </tc:running-time>
</xsl:template>

<xsl:template match="binding">
 <tc:binding i18n="true">
  <xsl:choose>
   <xsl:when test="contains(., 'Paperback')">
    <xsl:text>Paperback</xsl:text>
   </xsl:when>
   <xsl:when test="contains(., 'Hardcover')">
    <xsl:text>Hardback</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="."/>
   </xsl:otherwise>
  </xsl:choose>
 </tc:binding>
</xsl:template>

<xsl:template match="countries">
 <tc:nationalitys>
  <xsl:for-each select="country">
   <tc:nationality i18n="true">
    <xsl:value-of select="."/>
   </tc:nationality>
  </xsl:for-each>
 </tc:nationalitys>
</xsl:template>

<xsl:template match="actors">
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

<xsl:template match="directors">
 <tc:directors>
  <xsl:for-each select="director">
   <tc:director>
    <xsl:value-of select="name"/>
   </tc:director>
  </xsl:for-each>
 </tc:directors>
</xsl:template>

<xsl:template match="authors">
 <tc:authors>
  <xsl:for-each select="author">
   <tc:author>
    <xsl:value-of select="name"/>
   </tc:author>
  </xsl:for-each>
 </tc:authors>
</xsl:template>

<xsl:template match="artists">
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

<xsl:template match="languages">
 <tc:languages>
  <xsl:for-each select="language">
   <tc:language>
    <xsl:value-of select="name"/>
   </tc:language>
  </xsl:for-each>
 </tc:languages>
</xsl:template>

<xsl:template match="subtitle-languages">
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

<xsl:template match="edition-type">
 <tc:edition><xsl:value-of select="."/></tc:edition>
</xsl:template>

<xsl:template match="description">
 <tc:plot><xsl:value-of select="."/></tc:plot>
</xsl:template>

<xsl:template match="comment">
 <tc:comments><xsl:value-of select="."/></tc:comments>
</xsl:template>

<xsl:template match="container">
 <xsl:choose>
  <xsl:when test="name">
   <tc:location><xsl:value-of select="name"/></tc:location>
  </xsl:when>
  <xsl:otherwise>
   <xsl:apply-templates select="container"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="picture-front">
 <tc:cover>
  <xsl:choose>
   <!-- is the image location relative or not? -->
   <xsl:when test="starts-with(., 'file://') or starts-with(., 'http') or starts-with(., '/')">
    <xsl:value-of select="."/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="concat($baseDir, .)"/>
   </xsl:otherwise>
  </xsl:choose>
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

<xsl:template match="genres">
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

<xsl:template match="music-tracks">
 <tc:tracks>
  <xsl:for-each select="music-track">
   <tc:track>
    <tc:column>
     <xsl:value-of select="title"/>
    </tc:column>
    <tc:column>
     <xsl:value-of select="(artists/artist[1]/name|../../artists/artist[1]/name)[1]"/>
    </tc:column>
    <tc:column>
     <xsl:choose>
      <xsl:when test="starts-with(playlength, '0:')">
       <xsl:value-of select="substring(playlength,3)"/>
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
 
 <xsl:template match="storage-medium">
 <tc:medium i18n="true"><xsl:value-of select="."/></tc:medium>
</xsl:template>

<xsl:template match="record-label">
 <tc:labels>
  <tc:label><xsl:value-of select="."/></tc:label>
 </tc:labels>
</xsl:template>

<xsl:template match="publishers">
 <tc:publishers>
  <xsl:for-each select="publisher">
   <tc:publisher>
    <xsl:value-of select="."/>
   </tc:publisher>
  </xsl:for-each>
 </tc:publishers>
</xsl:template>

<xsl:template match="tags">
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

<xsl:template name="substring-before-last">
 <xsl:param name="input"/>
 <xsl:param name="substr"/>
 <xsl:if test="$substr and contains($input, $substr)">
  <xsl:variable name="temp" select="substring-after($input, $substr)"/>
  <xsl:value-of select="substring-before($input, $substr)"/>
  <xsl:if test="contains($temp, $substr)">
   <xsl:value-of select="$substr"/>
   <xsl:call-template name="substring-before-last">
    <xsl:with-param name="input" select="$temp"/>
    <xsl:with-param name="substr" select="$substr"/>
   </xsl:call-template>
  </xsl:if>
 </xsl:if>
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
