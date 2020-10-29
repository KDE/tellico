<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="exsl str"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing Collectorz data.

   Copyright (C) 2020 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:variable name="item">
 <xsl:choose>
  <xsl:when test="bookinfo">
   <xsl:text>book</xsl:text>
  </xsl:when>
  <xsl:when test="movieinfo">
   <xsl:text>movie</xsl:text>
  </xsl:when>
  <xsl:when test="musicinfo">
   <xsl:text>music</xsl:text>
  </xsl:when>
 </xsl:choose>
</xsl:variable>

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <xsl:variable name="type">
   <xsl:choose>
    <xsl:when test="$item='book'">2</xsl:when>
    <xsl:when test="$item='movie'">3</xsl:when>
    <xsl:when test="$item='music'">4</xsl:when>
    <xsl:otherwise>0</xsl:otherwise>
   </xsl:choose>
  </xsl:variable>
  <tc:collection title="Collectorz Import">
   <xsl:attribute name="type">
    <xsl:value-of select="$type"/>
   </xsl:attribute>
   <tc:fields>
    <tc:field name="_default"/>
    <tc:field flags="0" title="Cover String" category="General" format="0" type="1" name="coverstring"/>
    <tc:field flags="0" title="Amazon Link" category="General" format="4" type="7" name="amazon" i18n="true"/>
    <xsl:if test="$item='movie'">
     <tc:field flags="0" title="IMDb Link" category="General" format="4" type="7" name="imdb" i18n="true"/>
    </xsl:if>
    <xsl:if test="$item='music'">
     <xsl:if test="musicinfo/musiclist/music/conductor/displayname">
      <tc:field name="conductor" title="Conductor" format="2" flags="7" category="General" type="1" i18n="true"/>
     </xsl:if>
    </xsl:if>
    <!-- other potential custom fields: Location, Collection Status,  -->
   </tc:fields>
   <xsl:apply-templates select="bookinfo/booklist/book |
                                movieinfo/movielist/movie |
                                musicinfo/musiclist/music"/>
  </tc:collection>
 </tc:tellico>
</xsl:template>

<xsl:template match="book|movie|music">
 <tc:entry>
  <xsl:attribute name="id">
   <!-- yes, use index instead of collectorz id -->
   <xsl:value-of select="index"/>
  </xsl:attribute>
  <xsl:apply-templates select="*"/>
 </tc:entry>
</xsl:template>

<xsl:template match="mainsection">
 <xsl:apply-templates select="*"/>
</xsl:template>

<!-- All the real data gets handled below -->

<xsl:template match="title">
 <tc:title>
  <xsl:call-template name="strip-title">
   <xsl:with-param name="title" select="."/>
  </xsl:call-template>
 </tc:title>
</xsl:template>

<xsl:template match="subtitle">
 <tc:subtitle>
  <xsl:value-of select="."/>
 </tc:subtitle>
</xsl:template>

<xsl:template match="isbn">
 <tc:isbn>
  <xsl:value-of select="."/>
 </tc:isbn>
</xsl:template>

<xsl:template match="lccn">
 <tc:lccn>
  <xsl:value-of select="."/>
 </tc:lccn>
</xsl:template>

<xsl:template match="lastmodified">
 <tc:mdate calendar="gregorian">
  <xsl:call-template name="formatdate">
   <xsl:with-param name="datestr">
    <xsl:value-of select="substring-before(date,' ')"/>
   </xsl:with-param>
  </xsl:call-template>
 </tc:mdate>
</xsl:template>

<xsl:template match="pagecount">
 <tc:pages>
  <xsl:value-of select="."/>
 </tc:pages>
</xsl:template>

<xsl:template match="genres">
 <tc:genres>
  <xsl:for-each select="genre">
   <tc:genre>
    <xsl:value-of select="displayname"/>
   </tc:genre>
  </xsl:for-each>
 </tc:genres>
</xsl:template>

<xsl:template match="myrating|rating">
 <xsl:if test="string-length() &gt; 0 and not(text()='0' or displayname='0')">
  <tc:rating>
   <xsl:value-of select="(text()|displayname)[1]"/>
  </tc:rating>
 </xsl:if>
</xsl:template>

<xsl:template match="purchaseprice">
 <tc:pur_price>
  <xsl:value-of select="."/>
 </tc:pur_price>
</xsl:template>

<xsl:template match="purchasedate">
 <xsl:if test="year">
  <tc:pur_date>
   <xsl:value-of select="concat(year/displayname,'-',
                                format-number(month,'00'),'-',
                                format-number(day,'00'))"/>
  </tc:pur_date>
 </xsl:if>
</xsl:template>

<xsl:template match="plot">
 <tc:plot>
  <xsl:value-of select="."/>
 </tc:plot>
</xsl:template>

<xsl:template match="notes">
 <tc:comments>
  <xsl:value-of select="."/>
 </tc:comments>
</xsl:template>

<xsl:template match="coverfront[not(starts-with(.,'Generic'))]">
 <tc:coverstring>
  <xsl:value-of select="."/>
 </tc:coverstring>
</xsl:template>

<!-- only set the read field if it's true -->
<xsl:template match="readit[@boolvalue='1']">
 <tc:read>true</tc:read>
</xsl:template>

<xsl:template match="series">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:series>
    <xsl:value-of select="displayname"/>
   </tc:series>
  </xsl:when>
  <xsl:when test="$item = 'movie'">
   <tc:keywords>
    <tc:keyword>
     <xsl:value-of select="displayname"/>
    </tc:keyword>
   </tc:keywords>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="issuenr">
 <xsl:if test="string-length() &gt; 0 and not(text()='0')">
  <tc:series_num>
   <xsl:value-of select="."/>
  </tc:series_num>
 </xsl:if>
</xsl:template>

<!-- don't set condition new, only used -->
<xsl:template match="condition[displayname='Fair']">
 <tc:condition i18n="true">
  <xsl:text>Used</xsl:text>
 </tc:condition>
</xsl:template>

<xsl:template match="publicationdate|releasedate">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:pub_year>
    <xsl:value-of select="year/displayname"/>
   </tc:pub_year>
  </xsl:when>
  <xsl:otherwise>
   <tc:year>
    <xsl:value-of select="year/displayname"/>
   </tc:year>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="format">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:binding i18n="true">
    <xsl:choose>
     <xsl:when test="contains(displayname, 'Hardcover')">
      <xsl:text>Hardback</xsl:text>
     </xsl:when>
     <xsl:when test="contains(displayname, 'Softcover')">
      <xsl:text>Paperback</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="displayname"/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:binding>
  </xsl:when>
  <xsl:when test="$item = 'music'">
   <tc:medium i18n="true">
    <xsl:choose>
     <xsl:when test="contains(displayname, 'CD')">
      <xsl:text>Compact Disc</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="displayname"/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:medium>
  </xsl:when>
  <xsl:otherwise>
   <tc:medium i18n="true">
    <xsl:choose>
     <xsl:when test="contains(displayname, 'VHS')">
      <xsl:text>VHS</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="displayname"/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:medium>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="publisher">
 <tc:publishers>
  <tc:publisher>
   <xsl:value-of select="displayname"/>
  </tc:publisher>
 </tc:publishers>
</xsl:template>

<xsl:template match="studios">
 <tc:studios>
  <xsl:for-each select="studio">
   <tc:studio>
    <xsl:value-of select="displayname"/>
   </tc:studio>
  </xsl:for-each>
 </tc:studios>
</xsl:template>

<xsl:template match="authors">
 <tc:authors>
  <xsl:for-each select="author">
   <tc:author>
    <xsl:value-of select="person/displayname"/>
   </tc:author>
  </xsl:for-each>
 </tc:authors>
</xsl:template>

<xsl:template match="cast">
 <tc:casts>
  <xsl:for-each select="star">
   <tc:cast>
    <tc:column><xsl:value-of select="person/displayname"/></tc:column>
    <tc:column><xsl:value-of select="character"/></tc:column>
   </tc:cast>
  </xsl:for-each>
 </tc:casts>
</xsl:template>

<xsl:template match="crew">
 <tc:directors>
  <xsl:for-each select="crewmember[role='Director']">
   <tc:director>
    <xsl:value-of select="person/displayname"/>
   </tc:director>
  </xsl:for-each>
 </tc:directors>

 <tc:producers>
  <xsl:for-each select="crewmember[role='Producer']">
   <tc:producer>
    <xsl:value-of select="person/displayname"/>
   </tc:producer>
  </xsl:for-each>
 </tc:producers>

 <tc:writers>
  <xsl:for-each select="crewmember[role='Writer']">
   <tc:writer>
    <xsl:value-of select="person/displayname"/>
   </tc:writer>
  </xsl:for-each>
 </tc:writers>
</xsl:template>

<xsl:template match="mpaarating">
 <tc:certification i18n="true">
  <xsl:choose>
   <xsl:when test="starts-with(displayname, 'R')">R (USA)</xsl:when>
   <xsl:when test="starts-with(displayname, 'PG-13')">PG-13 (USA)</xsl:when>
   <xsl:when test="starts-with(displayname, 'PG')">PG (USA)</xsl:when>
   <xsl:when test="starts-with(displayname, 'G')">G (USA)</xsl:when>
   <xsl:when test="starts-with(displayname, 'NR')">U (USA)</xsl:when>
  </xsl:choose>
 </tc:certification>
</xsl:template>

<xsl:template match="language">
 <tc:language>
  <xsl:value-of select="displayname"/>
 </tc:language>
</xsl:template>

<xsl:template match="color">
 <tc:color i18n="true">
  <xsl:choose>
   <xsl:when test="contains(., 'Black')">
    <xsl:text>Black &amp; White</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="."/>
   </xsl:otherwise>
  </xsl:choose>
 </tc:color>
</xsl:template>

<xsl:template match="runtimeminutes">
 <tc:running-time>
  <xsl:value-of select="substring-before(., ' ')"/>
 </tc:running-time>
</xsl:template>

<xsl:template match="country">
 <xsl:choose>
  <xsl:when test="$item = 'movie'">
   <tc:nationality>
    <xsl:value-of select="displayname"/>
   </tc:nationality>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="ratios">
 <xsl:for-each select="ratio[contains(displayname,'Widescreen')][1]">
  <tc:widescreen>true</tc:widescreen>
 </xsl:for-each>
 <tc:aspect-ratios>
  <xsl:for-each select="ratio">
   <!-- extract actual ratio from text -->
   <xsl:variable name="ratio">
    <xsl:value-of select="translate(displayname,translate(displayname,'1234567890.:',''),'')"/>
   </xsl:variable>
   <tc:aspect-ratio>
    <xsl:choose>
     <xsl:when test="contains(displayname, '16:9')">
      <xsl:text>16:9</xsl:text>
     </xsl:when>
     <xsl:when test="contains(displayname, '4:3')">
      <xsl:text>4:3</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="$ratio"/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:aspect-ratio>
  </xsl:for-each>
 </tc:aspect-ratios>
</xsl:template>

<xsl:template match="audios">
 <tc:audio-tracks>
  <xsl:for-each select="audio">
   <tc:audio-track>
    <xsl:value-of select="displayname"/>
   </tc:audio-track>
  </xsl:for-each>
 </tc:audio-tracks>
</xsl:template>

<xsl:template match="subtitles">
 <tc:subtitles>
  <xsl:for-each select="subtitle">
   <tc:subtitle>
    <xsl:value-of select="displayname"/>
   </tc:subtitle>
  </xsl:for-each>
 </tc:subtitles>
</xsl:template>

<!-- only take first region -->
<xsl:template match="regions">
 <tc:region i18n="true">
  <xsl:value-of select="region[1]/displayname"/>
 </tc:region>
</xsl:template>

<xsl:template match="label">
 <tc:label>
  <xsl:value-of select="displayname"/>
 </tc:label>
</xsl:template>

<xsl:template match="artists">
 <tc:artists>
  <xsl:for-each select="artist">
   <tc:artist>
    <xsl:value-of select="displayname"/>
   </tc:artist>
  </xsl:for-each>
 </tc:artists>
</xsl:template>

<xsl:template match="composers">
 <tc:composers>
  <xsl:for-each select="composer">
   <tc:composer>
    <xsl:value-of select="displayname"/>
   </tc:composer>
  </xsl:for-each>
 </tc:composers>
</xsl:template>

<xsl:template match="conductor">
 <tc:conductors>
  <tc:conductor>
   <xsl:value-of select="displayname"/>
  </tc:conductor>
 </tc:conductors>
</xsl:template>

<xsl:template match="details">
 <xsl:if test="$item = 'music'">
  <tc:tracks>
   <xsl:for-each select="detail[@type='disc']/details/detail[@type='track']">
    <!-- sort first by disc index, then track index -->
    <xsl:sort select="../../index"/>
    <xsl:sort select="index"/>
    <tc:track>
     <tc:column>
      <xsl:value-of select="title"/>
     </tc:column>
     <tc:column>
      <!-- just take first artist -->
      <xsl:value-of select="artists/artist[1]/displayname"/>
     </tc:column>
     <tc:column>
      <xsl:value-of select="length"/>
     </tc:column>
    </tc:track>
   </xsl:for-each>
  </tc:tracks>
 </xsl:if>
</xsl:template>

<xsl:template match="links">
 <!-- grab first link that has amazon in the url -->
 <tc:amazon>
  <xsl:value-of select="link/url[contains(.,'amazon')][1]"/>
 </tc:amazon>
</xsl:template>

<xsl:template match="imdburl">
 <tc:imdb>
  <xsl:value-of select="."/>
 </tc:imdb>
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

<xsl:template name="formatdate">
 <xsl:param name="datestr" select="'robby'"/>
 <!-- input format mm/dd/yyyy -->

 <tc:year>
  <xsl:value-of select="substring-after(substring-after($datestr,'/'), '/')" />
 </tc:year>
 <tc:month>
  <xsl:value-of select="format-number(substring-before($datestr,'/'),'00')"/>
 </tc:month>
 <tc:day>
  <xsl:value-of select="format-number(substring-before(substring-after($datestr,'/'), '/'),'00')" />
 </tc:day>
</xsl:template>

<!-- ignore anything not explicit -->
<xsl:template match="*|@*"/>

</xsl:stylesheet>
