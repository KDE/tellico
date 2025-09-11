<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:math="http://exslt.org/math"
                xmlns:str="http://exslt.org/strings"
                xmlns:a="uri:attribute"
                exclude-result-prefixes="tc a"
                extension-element-prefixes="math"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for exporting to GCstar

   Copyright (C) 2008-2009 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<!-- the mapping from gcstar attribute to tellico element is automated here -->
<!-- @name is the gcstar attribute name, the value is the tellico element local-name() -->
<!-- bool attributes are special, and some only apply to certain collection types -->
<a:attributes>
 <a:attribute name="isbn">isbn</a:attribute>
  <!-- titles for coins and wines are templated -->
  <a:attribute name="title" skip="GCcoins, GCwines">title</a:attribute>
  <a:attribute name="publisher" skip="GCboardgames">publisher</a:attribute>
  <a:attribute name="publishedby" type="GCboardgames">publisher</a:attribute>
  <a:attribute name="publication">pub_year</a:attribute>
  <a:attribute name="language">language</a:attribute>
  <a:attribute name="serie">series</a:attribute>
  <a:attribute name="edition">edition</a:attribute>
  <a:attribute name="pages">pages</a:attribute>
  <a:attribute name="purchasedate" format="date" type="GCwines">pur_date</a:attribute>
  <a:attribute name="acquisition" format="date" skip="GCwines">pur_date</a:attribute>
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
  <a:attribute name="web" skip="GCfilms">url</a:attribute>
  <a:attribute name="webPage" type="GCfilms">url</a:attribute>
  <a:attribute name="read" format="bool" type="GCbooks">read</a:attribute>
  <a:attribute name="seen" format="bool" type="GCfilms">seen</a:attribute>
  <a:attribute name="favourite" format="bool">favorite</a:attribute>
  <a:attribute name="label" skip="GCwines">label</a:attribute>
  <a:attribute name="release" type="GCfilms">year</a:attribute>
  <a:attribute name="composer">composer</a:attribute>
  <a:attribute name="producer">producer</a:attribute>
  <a:attribute name="platform">platform</a:attribute>
  <a:attribute name="designedby">designer</a:attribute>
  <a:attribute name="players">num-player</a:attribute>
  <a:attribute name="developer">developer</a:attribute>
  <a:attribute name="designation">appellation</a:attribute>
  <a:attribute name="vintage">vintage</a:attribute>
  <a:attribute name="type">type</a:attribute>
  <a:attribute name="country">country</a:attribute>
  <a:attribute name="purchaseprice">pur_price</a:attribute>
  <a:attribute name="quantity">quantity</a:attribute>
  <a:attribute name="soil">soil</a:attribute>
  <a:attribute name="alcohol">alcohol</a:attribute>
  <a:attribute name="volume" skip="GCcomics">volume</a:attribute>
  <a:attribute name="volume" type="GCcomics">issue</a:attribute>
  <a:attribute name="tasting">description</a:attribute>
  <a:attribute name="medal">distinction</a:attribute>
  <a:attribute name="tasted" format="bool" type="GCwines">tasted</a:attribute>
  <a:attribute name="gift" format="bool">gift</a:attribute>
  <a:attribute name="writer">writer</a:attribute>
  <a:attribute name="colourist">colorist</a:attribute>
  <a:attribute name="category">category</a:attribute>
  <a:attribute name="collection">collection</a:attribute>
  <a:attribute name="numberboards">numberboards</a:attribute>
  <a:attribute name="signing" format="bool">signed</a:attribute>
  <a:attribute name="estimate">estimate</a:attribute>
  <a:attribute name="currency">currency</a:attribute>
  <a:attribute name="diameter">diameter</a:attribute>
  <a:attribute name="value">denomination</a:attribute>
  <a:attribute name="cover" format="image" type="GCbooks, GCmusics">cover</a:attribute>
  <a:attribute name="image" format="image" type="GCfilms, GCcomics">cover</a:attribute>
  <a:attribute name="boxpic" format="image" type="GCgames, GCboardgames">cover</a:attribute>
  <a:attribute name="picture" format="image" type="GCcoins">obverse</a:attribute>
  <a:attribute name="front" format="image" type="GCcoins">obverse</a:attribute>
  <a:attribute name="back" format="image" type="GCcoins">reverse</a:attribute>
  <a:attribute name="bottlelabel" format="image" type="GCwines">label</a:attribute>
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
  <xsl:when test="tc:tellico/tc:collection/@type=6">
   <xsl:text>GCcomics</xsl:text>
  </xsl:when>
  <xsl:when test="tc:tellico/tc:collection/@type=7">
   <xsl:text>GCwines</xsl:text>
  </xsl:when>
  <xsl:when test="tc:tellico/tc:collection/@type=8">
   <xsl:text>GCcoins</xsl:text>
  </xsl:when>
  <xsl:when test="tc:tellico/tc:collection/@type=11">
   <xsl:text>GCgames</xsl:text>
  </xsl:when>
  <xsl:when test="tc:tellico/tc:collection/@type=13">
   <xsl:text>GCboardgames</xsl:text>
  </xsl:when>
 </xsl:choose>
</xsl:variable>
<!-- grab all the applicable attributes once -->
<xsl:variable name="attributes" select="document('')/*/a:attributes/a:attribute[(contains(@type, $collType) or not(@type)) and
                                                                                 not(contains(@skip, $collType))]"/>

<xsl:key name="fieldsByName" match="tc:field" use="@name"/>

<xsl:param name="imageDir"/> <!-- dir where field images are located -->
<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>

<xsl:template match="/">
 <xsl:apply-templates select="tc:tellico"/>
</xsl:template>

<xsl:template match="tc:tellico">
 <xsl:apply-templates select="tc:collection"/>
</xsl:template>

<xsl:template match="tc:collection">
 <xsl:message terminate="yes">
  <xsl:text>GCstar export is not supported for this collection type.</xsl:text>
 </xsl:message>
</xsl:template>

<xsl:template match="tc:collection[@type=2 or
                                   @type=3 or
                                   @type=4 or
                                   @type=5 or
                                   @type=6 or
                                   @type=7 or
                                   @type=8 or
                                   @type=11 or
                                   @type=13]">
 <collection items="{count(tc:entry)}" type="{$collType}">
  <information>
   <maxId>
    <xsl:value-of select="math:max(tc:entry/@id)"/>
   </maxId>
  </information>
  <xsl:apply-templates select="tc:fields"/>
  <xsl:apply-templates select="tc:entry"/>
 </collection>
</xsl:template>

<!-- output only custom fields -->
<xsl:template match="tc:fields[tc:field[substring(@name, 1, 8) = 'gcsfield']]">
 <userCollection>
  <fields>
   <xsl:for-each select="tc:field[substring(@name, 1, 8) = 'gcsfield']">
    <field group="User fields" label="{@title}" value="{@name}" 
           init="{tc:prop[@name='default']}"
           min="{tc:prop[@name='minimum']}"
           max="{tc:prop[@name='maximum']}"
           values="{translate(@allowed, ';', ',')}">
     <xsl:attribute name="type">
      <xsl:choose>
       <xsl:when test="@type = '1' and not(@flags = '32')"><xsl:text>short text</xsl:text></xsl:when>
       <xsl:when test="@type = '1' and @flags = '32'"><xsl:text>formatted</xsl:text></xsl:when>
       <xsl:when test="@type = '2'"><xsl:text>long text</xsl:text></xsl:when>
       <xsl:when test="@type = '3'"><xsl:text>options</xsl:text></xsl:when>
       <xsl:when test="@type = '4'"><xsl:text>yesno</xsl:text></xsl:when>
       <xsl:when test="@type = '6'"><xsl:text>number</xsl:text></xsl:when>
       <xsl:when test="@type = '8'"><xsl:text>single list</xsl:text></xsl:when>
       <xsl:when test="@type = '10'"><xsl:text>image</xsl:text></xsl:when>
       <xsl:when test="@type = '12'"><xsl:text>date</xsl:text></xsl:when>
       <xsl:when test="@type = '14'"><xsl:text>number</xsl:text></xsl:when>
      </xsl:choose>
     </xsl:attribute>
     <xsl:if test="@type='6' or @type='14'">
      <xsl:attribute name="displayas">
       <xsl:if test="@type='6'"><xsl:text>text</xsl:text></xsl:if>
       <xsl:if test="@type='14'"><xsl:text>graphical</xsl:text></xsl:if>
      </xsl:attribute>
     </xsl:if>
     <xsl:if test="@type = '1' and @flags = '32'">
      <xsl:attribute name="init">
       <xsl:variable name="tokens" select="str:tokenize(tc:prop[@name = 'template'], '%')"/>
       <xsl:for-each select="$tokens">
        <xsl:choose>
         <xsl:when test="starts-with(., '{')">
          <xsl:variable name="tokens2" select="str:tokenize(., '}')"/>
          <xsl:for-each select="$tokens2">
           <xsl:if test="position() = 1">
            <!-- chop-off the beginning character '{' -->
            <xsl:value-of select="concat('%', substring(., 2, string-length(.)-1), '%')"/>
           </xsl:if>
           <xsl:if test="position() &gt; 1">
            <xsl:value-of select="."/>
           </xsl:if>
          </xsl:for-each>
         </xsl:when>
         <xsl:otherwise>
          <xsl:value-of select="."/>
         </xsl:otherwise>
        </xsl:choose>
       </xsl:for-each>
      </xsl:attribute>
     </xsl:if>
    </field>
   </xsl:for-each>
  </fields>
 </userCollection>
</xsl:template>

<!-- normally, no output for fields, unless there are custom fields -->
<xsl:template match="tc:fields"/>

<!-- no output for images -->
<xsl:template match="tc:images"/>

<xsl:template match="tc:entry">
 <xsl:variable name="entry" select="."/>
 <item id="{@id}">
  <xsl:apply-templates select="*[starts-with(local-name(), 'gcsfield')]" mode="attributes"/>
  <xsl:if test="tc:rating">
   <xsl:attribute name="rating">
    <xsl:value-of select="2*tc:rating"/>
   </xsl:attribute>
  </xsl:if>
  <xsl:for-each select="$attributes">
   <xsl:call-template name="handle-attribute">
    <xsl:with-param name="att" select="."/>
    <xsl:with-param name="entry" select="$entry"/>
   </xsl:call-template>
  </xsl:for-each>

  <xsl:attribute name="added">
   <!-- gcstar uses french format, dd/mm/yy -->
   <xsl:value-of select="concat(tc:cdate/tc:day, '/',
                                tc:cdate/tc:month, '/',
                                tc:cdate/tc:year)"/>
  </xsl:attribute>

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
  <xsl:call-template name="multiline">
   <xsl:with-param name="name" select="'authors'"/>
   <xsl:with-param name="elem" select="tc:authors"/>
  </xsl:call-template>
  <xsl:call-template name="multiline">
   <xsl:with-param name="name" select="'genre'"/>
   <xsl:with-param name="elem" select="tc:genres"/>
  </xsl:call-template>
  <xsl:call-template name="multiline">
   <xsl:with-param name="name" select="'tags'"/>
   <xsl:with-param name="elem" select="tc:keywords"/>
  </xsl:call-template>

  <!-- for movies -->
  <xsl:if test="$collType = 'GCfilms' or $collType = 'GCcomics'">
   <synopsis>
    <xsl:value-of select="tc:plot"/>
   </synopsis>
  </xsl:if>
<!--
  <directors>
   <xsl:call-template name="multiline">
    <xsl:with-param name="elem" select="tc:directors"/>
   </xsl:call-template>
  </directors>
-->
  <xsl:call-template name="table">
   <xsl:with-param name="name" select="'actors'"/>
   <xsl:with-param name="elem" select="tc:casts"/>
  </xsl:call-template>
  <xsl:call-template name="multiline">
   <xsl:with-param name="name" select="'subt'"/>
   <xsl:with-param name="elem" select="tc:subtitles"/>
  </xsl:call-template>
  <xsl:choose>
   <xsl:when test="$collType = 'GCfilms' or $collType = 'GCboardgames'">
    <comment> <!-- note the lack of an 's' -->
     <xsl:value-of select="tc:comments"/>
    </comment>
   </xsl:when>
   <xsl:otherwise>
    <comments>
     <xsl:value-of select="tc:comments"/>
    </comments>
   </xsl:otherwise>
  </xsl:choose>

  <xsl:if test="$collType = 'GCfilms'">
   <xsl:apply-templates select="tc:languages"/>
  </xsl:if>

  <!-- for music -->
  <xsl:apply-templates select="tc:tracks"/>

  <!-- board games -->
  <xsl:call-template name="multiline">
   <xsl:with-param name="name" select="'mechanics'"/>
   <xsl:with-param name="elem" select="tc:mechanisms"/>
  </xsl:call-template>

  <xsl:if test="$collType = 'GCgames' or $collType = 'GCboardgames'">
   <description>
    <xsl:value-of select="tc:description"/>
   </description>
  </xsl:if>

  <!-- for wines -->
  <xsl:call-template name="multiline">
   <xsl:with-param name="name" select="'grapes'"/>
   <xsl:with-param name="elem" select="tc:varietals"/>
  </xsl:call-template>

  <!-- for coins -->
  <xsl:apply-templates select="tc:metals"/>

  <xsl:apply-templates select="*[starts-with(local-name(), 'gcsfield')]" mode="elements"/>

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

<xsl:template match="tc:metals">
 <metal>
  <xsl:for-each select="tc:metal">
   <line>
    <col>
     <xsl:value-of select="tc:column[1]"/>
    </col>
   </line>
  </xsl:for-each>
 </metal>
</xsl:template>

<xsl:template match="*[starts-with(local-name(), 'gcsfield')]" mode="attributes">
 <xsl:choose>
  <xsl:when test="key('fieldsByName', local-name())/@type = '2'"/>
  <xsl:when test="key('fieldsByName', substring(local-name(), 1, string-length(local-name())-1))/@type = '8'"/>
  <xsl:when test="key('fieldsByName', local-name())/@type = '12'">
   <xsl:attribute name="{local-name()}">
    <xsl:value-of select="concat(tc:day, '/', tc:month, '/', tc:year)"/>
   </xsl:attribute>
  </xsl:when>
  <xsl:when test="key('fieldsByName', local-name())/@type = '4'">
   <xsl:if test=". = 'true'">
    <xsl:attribute name="{local-name()}">1</xsl:attribute>
   </xsl:if>
  </xsl:when>
  <xsl:otherwise>
   <xsl:attribute name="{local-name()}">
    <xsl:value-of select="."/>
   </xsl:attribute>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="*[starts-with(local-name(), 'gcsfield')]" mode="elements">
 <xsl:variable name="shortname">
  <xsl:value-of select="substring(local-name(), 1, string-length(local-name())-1)"/>
 </xsl:variable>
 <xsl:choose>
  <xsl:when test="key('fieldsByName', local-name())/@type = '12'"/>
  <xsl:when test="key('fieldsByName', local-name())/@type = '2'">
   <xsl:element name="{local-name()}">
    <xsl:value-of select="."/>
   </xsl:element>
  </xsl:when>
  <xsl:when test="key('fieldsByName', $shortname)/@type = '8'">
   <xsl:element name="{$shortname}">
    <xsl:for-each select="child::*">
     <line>
      <col>
       <xsl:value-of select="tc:column"/>
      </col>
     </line>
    </xsl:for-each>
   </xsl:element>
  </xsl:when>
  <xsl:otherwise/>
 </xsl:choose>
</xsl:template>

<xsl:template name="multiline">
 <xsl:param name="name"/>
 <xsl:param name="elem"/>
 <xsl:if test="$elem">
  <xsl:element name="{$name}">
   <xsl:for-each select="$elem/child::*">
    <line>
     <col>
      <xsl:value-of select="."/>
     </col>
    </line>
   </xsl:for-each>
  </xsl:element>
 </xsl:if>
</xsl:template>

<xsl:template name="table">
 <xsl:param name="name"/>
 <xsl:param name="elem"/>
 <xsl:if test="$elem">
  <xsl:element name="{$name}">
   <xsl:for-each select="$elem/child::*">
    <line>
     <xsl:for-each select="child::*">
      <col>
       <xsl:value-of select="."/>
      </col>
     </xsl:for-each>
    </line>
   </xsl:for-each>
  </xsl:element>
 </xsl:if>
</xsl:template>

<xsl:template name="handle-attribute">
 <xsl:param name="att"/>
 <xsl:param name="entry"/>
 <!-- should technically check namespace, too, but unlikely to match -->
 <!-- select the direct children of the entry, or those grandchildren whose parent is equal to their name + 's' -->
 <xsl:variable name="values" select="$entry/*[local-name()=$att] |
                                     $entry/*[local-name()=concat($att,'s')]/*[local-name()=$att]"/>
 <xsl:choose>
  <xsl:when test="$att/@format='bool'">
   <xsl:attribute name="{$att/@name}">
    <xsl:value-of select="number($values[1]='true')"/>
   </xsl:attribute>
  </xsl:when>
  <xsl:when test="$att/@format='image'">
   <xsl:if test="string-length($imageDir) &gt; 0">
    <xsl:attribute name="{$att/@name}">
     <xsl:value-of select="concat($imageDir, $values[1])"/>
    </xsl:attribute>
   </xsl:if>
  </xsl:when>
  <xsl:when test="$att/@format='date'">
   <!-- gcstar uses french format, dd/mm/yy, instead of yy-mm-dd -->
   <!-- so split and reverse tokens -->
   <xsl:variable name="numbers" select="str:tokenize($values[1], '-')"/>
   <xsl:if test="count($numbers) = 3">
    <xsl:attribute name="{$att/@name}">
     <xsl:value-of select="concat($numbers[3],'/',$numbers[2],'/',$numbers[1])"/>
    </xsl:attribute>
   </xsl:if>
  </xsl:when>
  <xsl:otherwise>
   <xsl:if test="count($values)">
    <xsl:attribute name="{$att/@name}">
     <xsl:for-each select="$values">
      <xsl:value-of select="."/>
      <xsl:if test="position() &lt; last()">
       <xsl:text>, </xsl:text>
      </xsl:if>
     </xsl:for-each>
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
