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
   Tellico XSLT file - used for importing Delicious Library data.

   Copyright (C) 2007-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:param name="item-type"/>

<cc:codes>
  <cc:code id="us">USA</cc:code>
  <cc:code id="fr">France</cc:code>
  <cc:code id="de">Germany</cc:code>
  <cc:code id="es">Spain</cc:code>
</cc:codes>
<xsl:key name="ccodes" match="cc:code" use="@id"/>
<xsl:variable name="ccodes-top" select="document('')/*/cc:codes"/>

<xsl:key name="shelflinks" match="shelf" use="linkto/@uuid"/>

<!-- for lower-casing -->
<xsl:variable name="lcletters">abcdefghijklmnopqrstuvwxyz</xsl:variable>
<xsl:variable name="ucletters">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

<!-- DL libraries can contain mixed types and right now, there's no way to do that in Tellico -->
<!-- so we're going to limit the export to whatever the type of the first item is -->
<xsl:variable name="item">
 <xsl:choose>
  <xsl:when test="string-length($item-type) &gt; 0">
   <xsl:value-of select="$item-type"/>
  </xsl:when>
  <xsl:when test="library/items">
   <xsl:value-of select="local-name(library/items/child::*[1])"/>
  </xsl:when>
  <xsl:otherwise>
   <!-- case has been inconsistent, make sure it's lower-case -->
   <xsl:value-of select="translate(plist/array/dict[1]/key[.='type']/following-sibling::string[1],$ucletters,$lcletters)"/>
  </xsl:otherwise>
 </xsl:choose>
</xsl:variable>

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <xsl:comment><xsl:text>Choosing all items of type </xsl:text><xsl:value-of select="$item"/></xsl:comment>

  <xsl:variable name="type">
   <xsl:choose>
    <xsl:when test="$item='book'">2</xsl:when>
    <xsl:when test="$item='movie'">3</xsl:when>
    <xsl:when test="$item='music'">4</xsl:when>
    <xsl:when test="$item='game'">11</xsl:when>
    <xsl:otherwise>0</xsl:otherwise>
   </xsl:choose>
  </xsl:variable>
  <tc:collection title="Delicious Library Import">
   <xsl:attribute name="type">
    <xsl:value-of select="$type"/>
   </xsl:attribute>
   <tc:fields>
    <tc:field name="_default"/>
    <tc:field flags="0" title="Amazon Link" category="General" format="4" type="7" name="amazon" i18n="true"/>
    <tc:field flags="0" title="UUID" category="General" format="0" type="1" name="uuid"/>
   </tc:fields>
   <xsl:apply-templates select="library/items/child::*[local-name()=$item]"/>
   <xsl:apply-templates select="plist/array/dict"/>
  </tc:collection>
 </tc:tellico>
</xsl:template>

<xsl:template match="book|movie|music|game">
 <tc:entry>
  <!-- For simplicity, convert all attributes to elements -->
  <xsl:for-each select="@*">
   <xsl:variable name="dummies">
    <xsl:element name="{name()}">
     <xsl:value-of select="."/>
    </xsl:element>
   </xsl:variable>
   <xsl:for-each select="exsl:node-set($dummies)/*">
    <xsl:apply-templates select="."/>
   </xsl:for-each>
  </xsl:for-each>
  <!-- we can't do this in the uuid template because of context change -->
  <tc:keywords>
   <tc:keyword>
    <xsl:value-of select="key('shelflinks', @uuid)/@name"/>
   </tc:keyword>
  </tc:keywords>
  <!-- catch the description too -->
  <xsl:apply-templates select="*"/>
 </tc:entry>
</xsl:template>

<xsl:template match="dict">
 <tc:entry>
  <xsl:apply-templates select="key"/>
  <!-- we can't do this in the uuid template because of context change -->
  <tc:keywords>
   <tc:keyword>
    <xsl:value-of select="key('shelflinks', key[text()='uuid']/following-sibling::*[1])/@name"/>
   </tc:keyword>
  </tc:keywords>
 </tc:entry>
</xsl:template>

<xsl:template match="key">
 <!-- For simplicity, convert all keys to elements -->
 <xsl:variable name="dummies">
  <xsl:element name="{text()}">
   <xsl:value-of select="following-sibling::*[1]"/>
  </xsl:element>
 </xsl:variable>
 <xsl:for-each select="exsl:node-set($dummies)/*">
  <xsl:apply-templates select="."/>
 </xsl:for-each>
</xsl:template>

<!-- All the real data gets handled below -->

<xsl:template match="title">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:title>
    <xsl:choose>
     <xsl:when test="contains(., ':')">
      <xsl:value-of select="substring-before(.,':')"/>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="."/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:title>
   
   <tc:subtitle>
    <xsl:value-of select="substring-after(.,':')"/>
   </tc:subtitle>
  </xsl:when>
  
  <xsl:otherwise>
   <tc:title>
    <xsl:call-template name="strip-title">
     <xsl:with-param name="title" select="."/>
    </xsl:call-template>
   </tc:title>
  </xsl:otherwise>
 </xsl:choose>
 
</xsl:template>

<xsl:template match="uuidString|uuid">
 <tc:uuid>
  <xsl:value-of select="."/>
 </tc:uuid>
</xsl:template>

<xsl:template match="asin">
 <tc:amazon>
  <xsl:value-of select="concat('http://www.amazon.com/dp/',text(),'/?tag=tellico-20')"/>
 </tc:amazon>

 <tc:isbn>
  <xsl:value-of select="."/>
 </tc:isbn>
</xsl:template>

<xsl:template match="isbn">
 <tc:isbn>
  <xsl:value-of select="."/>
 </tc:isbn>
</xsl:template>

<!-- publishDate is handled farther below -->
<xsl:template match="published">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:pub_year>
    <xsl:call-template name="year">
     <xsl:with-param name="value" select="."/>
    </xsl:call-template>
   </tc:pub_year>
  </xsl:when>
  <xsl:when test="$item = 'music' or $item = 'game'">
   <tc:year>
    <xsl:call-template name="year">
     <xsl:with-param name="value" select="."/>
    </xsl:call-template>
   </tc:year>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="creationDate">
 <tc:cdate>
  <xsl:value-of select="substring(.,1,10)"/>
 </tc:cdate>
</xsl:template>

<xsl:template match="lastModificationDate">
 <tc:mdate>
  <xsl:value-of select="substring(.,1,10)"/>
 </tc:mdate>
</xsl:template>

<xsl:template match="pages">
 <tc:pages>
  <xsl:value-of select="."/>
 </tc:pages>
</xsl:template>

<xsl:template match="genresCompositeString">
 <!-- we don't include genresCompositeString since it's crap with no way to
      determine how to split with spaces -->
 <!-- but if it includes a newline, assume that's the delimiter -->
 <xsl:if test="contains(., '&#10;')">
  <xsl:call-template name="split">
   <xsl:with-param name="name" select="'tc:keyword'"/>
   <xsl:with-param name="value" select="."/>
   <xsl:with-param name="i18n" select="true()"/>
  </xsl:call-template>
 </xsl:if>
</xsl:template>

<xsl:template match="genre">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:keyword'"/>
  <xsl:with-param name="value" select="."/>
  <xsl:with-param name="i18n" select="true()"/>
 </xsl:call-template>
 
 <xsl:if test="$item = 'movie'">
  <xsl:choose>
   <xsl:when test="contains(., 'Region 1')">
    <tc:region i18n="true">Region 1</tc:region>
   </xsl:when>
   <xsl:when test="contains(., 'Region 2')">
    <tc:region i18n="true">Region 2</tc:region>
   </xsl:when>
   <xsl:when test="contains(., 'Region 3')">
    <tc:region i18n="true">Region 3</tc:region>
   </xsl:when>
   <xsl:when test="contains(., 'Region 4')">
    <tc:region i18n="true">Region 4</tc:region>
   </xsl:when>
   <xsl:when test="contains(., 'Region 5')">
    <tc:region i18n="true">Region 5</tc:region>
   </xsl:when>
   <xsl:when test="contains(., 'Region 6')">
    <tc:region i18n="true">Region 6</tc:region>
   </xsl:when>
   <xsl:when test="contains(., 'Region 7')">
    <tc:region i18n="true">Region 7</tc:region>
   </xsl:when>
   <xsl:when test="contains(., 'Region 8')">
    <tc:region i18n="true">Region 8</tc:region>
   </xsl:when>
  </xsl:choose>
 </xsl:if>
</xsl:template>

<xsl:template match="netrating|netRating">
 <tc:rating>
  <xsl:value-of select="."/>
 </tc:rating>
</xsl:template>

<xsl:template match="price">
 <tc:pur_price>
  <xsl:value-of select="."/>
 </tc:pur_price>
</xsl:template>

<xsl:template match="purchaseDate">
 <tc:pur_date>
  <xsl:value-of select="substring(.,1,10)"/>
 </tc:pur_date>
</xsl:template>

<xsl:template match="description">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:comments>
    <!-- RTF gets cleaned up inside of Tellico -->
    <xsl:value-of select="."/>
   </tc:comments>
  </xsl:when>
  <xsl:when test="$item = 'movie'">
   <tc:plot>
    <!-- RTF gets cleaned up inside of Tellico -->
    <xsl:value-of select="."/>
   </tc:plot>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="minutes">
 <tc:running-time>
  <xsl:value-of select="."/>
 </tc:running-time>
</xsl:template>

<xsl:template match="artist">
 <tc:artist>
  <xsl:value-of select="."/>
 </tc:artist>
</xsl:template>

<xsl:template match="isSigned">
 <tc:signed>
  <xsl:value-of select="boolean(number(.))"/>
 </tc:signed>
</xsl:template>

<!-- don't set condition new, only used -->
<xsl:template match="used[text()='1']">
 <tc:condition i18n="true">
  <xsl:text>Used</xsl:text>
 </tc:condition>
</xsl:template>

<xsl:template match="theatricalDate|publishDate">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:pub_year>
    <xsl:value-of select="substring(.,1,4)"/>
   </tc:pub_year>
  </xsl:when>
  <xsl:otherwise>
   <tc:year>
    <xsl:call-template name="year">
     <xsl:with-param name="value" select="."/>
    </xsl:call-template>
   </tc:year>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="formatSingularString|aspect">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:binding i18n="true">
    <xsl:choose>
     <xsl:when test="contains(., 'Hardcover')">
      <xsl:text>Hardback</xsl:text>
     </xsl:when>
     <xsl:when test="contains(., 'Paperback')">
      <xsl:text>Paperback</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="."/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:binding>
  </xsl:when>
  <xsl:when test="$item = 'music'">
   <tc:medium i18n="true">
    <xsl:choose>
     <xsl:when test="contains(., 'CD')">
      <xsl:text>Compact Disc</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="."/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:medium>
  </xsl:when>
  <xsl:otherwise>
   <tc:medium i18n="true">
    <xsl:choose>
     <xsl:when test="contains(., 'VHS')">
      <xsl:text>VHS</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="."/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:medium>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="actorsCompositeString">
 <xsl:choose>
  <xsl:when test="$item = 'music'">
   <tc:artists>
    <tc:artist>
     <xsl:value-of select="."/>
    </tc:artist>
   </tc:artists>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="publishersCompositeString|publisher">
 <xsl:choose>
  <xsl:when test="$item = 'book' or $item = 'game'">
   <tc:publishers>
    <tc:publisher>
     <xsl:value-of select="."/>
    </tc:publisher>
   </tc:publishers>
  </xsl:when>
  <xsl:when test="$item = 'movie'">
   <tc:studios>
    <tc:studio>
     <xsl:value-of select="."/>
    </tc:studio>
   </tc:studios>
  </xsl:when>
  <xsl:when test="$item = 'music'">
   <tc:labels>
    <tc:label>
     <xsl:value-of select="."/>
    </tc:label>
   </tc:labels>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="author">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:author'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="director">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:director'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="stars">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:cast'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="tracklisting">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:track'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="creatorsCompositeString">
 <xsl:choose>
  <xsl:when test="$item = 'book'">
   <tc:authors>
    <tc:author>
     <xsl:value-of select="."/>
    </tc:author>
   </tc:authors>
  </xsl:when>
  <xsl:when test="$item = 'movie'">
   <tc:directors>
    <tc:director>
     <xsl:value-of select="."/>
    </tc:director>
   </tc:directors>
  </xsl:when>
  <xsl:when test="$item = 'music'">
   <tc:artists>
    <tc:artist>
     <xsl:value-of select="."/>
    </tc:artist>
   </tc:artists>
  </xsl:when>
  <xsl:otherwise>
   <xsl:comment>
    <xsl:value-of select="concat('creator is', text())"/>
   </xsl:comment>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="mpaarating|audienceRecommendedAgeSingularString">
 <tc:certification i18n="true">
  <xsl:choose>
   <xsl:when test="starts-with(., 'R')">R (USA)</xsl:when>
   <xsl:when test="starts-with(., 'PG-13')">PG-13 (USA)</xsl:when>
   <xsl:when test="starts-with(., 'PG')">PG (USA)</xsl:when>
   <xsl:when test="starts-with(., 'G')">G (USA)</xsl:when>
   <xsl:when test="starts-with(., 'NR')">U (USA)</xsl:when>
  </xsl:choose>
 </tc:certification>
</xsl:template>

<xsl:template match="languagesCompositeString">
 <xsl:call-template name="split">
  <xsl:with-param name="name" select="'tc:language'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="country|countryCode">
 <tc:nationalitys>
  <tc:nationality>
   <xsl:apply-templates select="$ccodes-top">
    <xsl:with-param name="ccode-id" select="."/>
   </xsl:apply-templates>
  </tc:nationality>
 </tc:nationalitys>
</xsl:template>

<xsl:template match="features|featuresCompositeString">
 <xsl:if test="contains(., 'Widescreen')"><tc:widescreen>true</tc:widescreen></xsl:if>
 <xsl:if test='contains(., "Director&apos;s Cut")'><tc:directors-cut>true</tc:directors-cut></xsl:if>
 <xsl:if test="contains(., 'Region 1')"><tc:region i18n="true">Region 1</tc:region></xsl:if>
 <xsl:if test="contains(., 'Region 2')"><tc:region i18n="true">Region 2</tc:region></xsl:if>
 <xsl:if test="contains(., 'Region 3')"><tc:region i18n="true">Region 3</tc:region></xsl:if>
 <xsl:if test="contains(., 'Region 4')"><tc:region i18n="true">Region 4</tc:region></xsl:if>
 <xsl:if test="contains(., 'Region 5')"><tc:region i18n="true">Region 5</tc:region></xsl:if>
 <xsl:if test="contains(., 'Region 6')"><tc:region i18n="true">Region 6</tc:region></xsl:if>
 <xsl:if test="contains(., 'Region 7')"><tc:region i18n="true">Region 7</tc:region></xsl:if>
 <xsl:if test="contains(., 'Region 8')"><tc:region i18n="true">Region 8</tc:region></xsl:if>
 <xsl:if test="contains(., 'Color')"><tc:color i18n="true">Color</tc:color></xsl:if>
 <xsl:if test="contains(., 'Black &amp; White')"><tc:color i18n="true">Black &amp; White</tc:color></xsl:if>
 <xsl:if test="contains(., 'NTSC')"><tc:format i18n="true">NTSC</tc:format></xsl:if>
 <xsl:if test="contains(., 'SECAM')"><tc:format i18n="true">SECAM</tc:format></xsl:if>
 <xsl:if test="contains(., 'PAL')"><tc:format i18n="true">PAL</tc:format></xsl:if>
 <tc:aspect-ratios>
  <xsl:if test="contains(., '1.33:1')"><tc:aspect-ratio>1.33:1</tc:aspect-ratio></xsl:if>
  <xsl:if test="contains(., '1.85:1')"><tc:aspect-ratio>1.85:1</tc:aspect-ratio></xsl:if>
  <xsl:if test="contains(., '2.35:1')"><tc:aspect-ratio>2.35:1</tc:aspect-ratio></xsl:if>
 </tc:aspect-ratios>
 <tc:audio-tracks>
  <xsl:if test="contains(., 'Dolby')"><tc:audio-track i18n="true">Dolby</tc:audio-track></xsl:if>
  <xsl:if test="contains(., 'DTS')"><tc:audio-track i18n="true">DTS</tc:audio-track></xsl:if>
 </tc:audio-tracks>
</xsl:template>

<xsl:template match="platform">
 <tc:platform>
   <xsl:value-of select="."/>
  </tc:platform>
 </xsl:template>

<xsl:template match="esrbrating">
 <tc:certification>
  <xsl:value-of select="."/>
 </tc:certification>
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

<xsl:template match="cc:codes">
  <xsl:param name="ccode-id"/>
  <xsl:variable name="c" select="key('ccodes', $ccode-id)"/>
  <xsl:comment>
   <xsl:value-of select="$ccode-id"/>
  </xsl:comment>
  <xsl:comment>
   <xsl:value-of select="$c"/>
  </xsl:comment>
  <xsl:if test="$c">
    <xsl:value-of select="$c"/>
  </xsl:if>
  <xsl:if test="not($c)">
    <xsl:value-of select="$ccode-id"/>
  </xsl:if>
</xsl:template>

<!-- ignore anything not explicit -->
<xsl:template match="*|@*"/>

</xsl:stylesheet>
