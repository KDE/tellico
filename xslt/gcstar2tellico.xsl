<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:tc="http://periapsis.org/tellico/"
                xmlns:exsl="http://exslt.org/common"
                xmlns:str="http://exslt.org/strings"
                extension-element-prefixes="str exsl"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing GCstar data.

   Copyright (C) 2007 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at https://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:param name="datadir"/> <!-- dir where custom data might be located -->

<xsl:variable name="coll">
 <xsl:choose>
  <xsl:when test="/collection[1]/@type='GCbooks'">
   <xsl:text>2</xsl:text>
  </xsl:when>
  <xsl:when test="/collection[1]/@type='GCfilms'">
   <xsl:text>3</xsl:text>
  </xsl:when>
  <xsl:when test="/collection[1]/@type='GCmusics'">
   <xsl:text>4</xsl:text>
  </xsl:when>
  <xsl:when test="/collection[1]/@type='GCcomics'">
   <xsl:text>6</xsl:text>
  </xsl:when>
  <xsl:when test="/collection[1]/@type='GCwines'">
   <xsl:text>7</xsl:text>
  </xsl:when>
  <xsl:when test="/collection[1]/@type='GCcoins'">
   <xsl:text>8</xsl:text>
  </xsl:when>
  <xsl:when test="/collection[1]/@type='GCgames'">
   <xsl:text>11</xsl:text>
  </xsl:when>
  <xsl:when test="/collection[1]/@type='GCboardgames'">
   <xsl:text>13</xsl:text>
  </xsl:when>
  <xsl:otherwise>
   <xsl:text>1</xsl:text> <!-- custom collection -->
  </xsl:otherwise>
 </xsl:choose>
</xsl:variable>

<xsl:variable name="externalModel">
 <xsl:if test="$coll = 1">
  <!-- Using copy-of creates a result-tree fragment, which requires using node-set() later -->
  <xsl:copy-of select="document(concat($datadir, /collection[1]/@type, '.gcm'))/collection/fields"/>
 </xsl:if>
 <xsl:if test="$coll &gt; 1">
  <xsl:value-of select="/.."/> <!-- effectively, the null set -->
 </xsl:if>
</xsl:variable>

<xsl:template match="/">
 <tc:tellico syntaxVersion="11">
  <xsl:apply-templates select="collection" mode="top"/>
 </tc:tellico>
</xsl:template>

<xsl:template match="collection" mode="top">
 <tc:collection title="GCstar Import" type="{$coll}">
  <tc:fields>
   <xsl:if test="$coll &gt; 1">
    <!-- all default fields and favorite for any non-custom collection type -->
    <tc:field name="_default"/>
    <tc:field flags="0" title="Favorite" category="Personal" format="4" type="4" name="favorite" i18n="true"/>
   </xsl:if>
   <xsl:if test="item/@web or item/@webPage">
    <tc:field flags="0" title="URL" category="General" format="4" type="7" name="url" i18n="true"/>
   </xsl:if>
   <xsl:if test="(item/@location or item/@place) and @type != 'GCwines' and @type != 'GCcoins'">
     <tc:field flags="6" title="Location" category="Personal" format="4" type="1" name="location" i18n="true"/>
   </xsl:if>
   <xsl:if test="item/@composer">
    <tc:field flags="7" title="Composer" category="General" format="2" type="1" name="composer" i18n="true"/>
   </xsl:if>
   <xsl:if test="item/@producer and @type != 'GCwines'">
    <tc:field flags="7" title="Producer" category="General" format="2" type="1" name="producer" i18n="true"/>
   </xsl:if>
   <xsl:choose>
    <xsl:when test="@type='GCfilms'">
     <tc:field flags="8" title="Original Title" category="General" format="1" type="1" name="origtitle" i18n="true"/>
     <tc:field flags="0" title="Seen" category="Personal" format="4" type="4" name="seen" i18n="true"/>
    </xsl:when>
    <xsl:when test="@type='GCcoins'">
     <!-- gcstar includes many more coin grades than tellico -->
     <tc:field flags="2" title="Grade" category="General" format="4" type="3" name="grade"
               allowed="Proof-65;Proof-60;Mint State-70;Mint State-69;Mint State-68;Mint State-67;Mint State-66;Mint State-65;Mint State-64;Mint State-63;Mint State-62;Mint State-61;Mint State-60;Almost Uncirculated-58;Almost Uncirculated-55;Almost Uncirculated-53;Almost Uncirculated-50;Extremely Fine-45;Extremely Fine-40;Very Fine-35;Very Fine-30;Very Fine-25;Very Fine-20;Fine-15;Fine-12;Very Good-10;Very Good-8;Good-6;Good-4;Fair"/>
     <tc:field flags="6" title="Diameter" category="General" format="4" type="1" name="diameter" i18n="true"/>
     <tc:field flags="0" title="Estimate" category="Personal" format="4" type="1" name="estimate" i18n="true"/>
     <xsl:if test="item/metal">
      <tc:field title="Composition" flags="3" category="Composition" format="2" type="8" name="metal">
       <tc:prop name="column1">Metal</tc:prop>
       <tc:prop name="column2">Percentage</tc:prop>
       <tc:prop name="columns">2</tc:prop>
      </tc:field>
     </xsl:if>
    </xsl:when>
    <xsl:when test="@type='GCwines'">
     <tc:field flags="7" title="Varietal" category="General" format="0" type="1" name="varietal" i18n="true"/>
     <tc:field flags="0" title="Tasted" category="Personal" format="4" type="4" name="tasted" i18n="true"/>
     <tc:field flags="6" title="Distinction" category="General" format="4" type="1" name="distinction" i18n="true"/>
     <tc:field flags="6" title="Soil" category="General" format="4" type="1" name="soil" i18n="true"/>
     <tc:field flags="6" title="Alcohol" category="General" format="4" type="1" name="alcohol" i18n="true"/>
     <tc:field flags="6" title="Volume" category="General" format="4" type="1" name="volume" i18n="true"/>
     <tc:field flags="2" title="Type" category="General" format="4" type="3" name="type" i18n="true">
      <xsl:attribute name="allowed">
       <xsl:text>Red Wine;White Wine;Sparkling Wine</xsl:text>
       <xsl:for-each select="item[not(@type=preceding-sibling::item/@type)]/@type">
        <xsl:value-of select="concat(';',.)"/>
       </xsl:for-each>
      </xsl:attribute>
     </tc:field>
    </xsl:when>
    <xsl:when test="@type='GCcomics'">
     <tc:field flags="0" title="ISBN#" category="Publishing" format="4" description="International Standard Book Number" type="1" name="isbn" i18n="true"/>
     <tc:field flags="7" title="Colorist" category="General" format="2" type="1" name="colorist" i18n="true"/>
     <tc:field flags="6" title="Format" category="Publishing" format="4" type="1" name="format" i18n="true"/>
     <tc:field flags="6" title="Category" category="Publishing" format="4" type="1" name="category" i18n="true"/>
     <tc:field flags="6" title="Collection" category="Personal" format="4" type="1" name="collection" i18n="true"/>
     <tc:field flags="0" title="Boards" category="Publishing" format="4" type="6" name="numberboards"/>
    </xsl:when>
    <xsl:when test="$coll = 1">
     <!-- all custom fields, from a custom model -->
     <xsl:apply-templates select="exsl:node-set($externalModel)/fields/field"/>
    </xsl:when>
   </xsl:choose>
   <xsl:apply-templates select="userCollection/fields/field"/>
  </tc:fields>
  <xsl:apply-templates select="item"/>
 </tc:collection>
</xsl:template>

<xsl:template match="field">
 <tc:field name="{@value}" title="{@label}" category="{@group}" i18n="true">
  <xsl:attribute name="type">
   <xsl:choose>
    <xsl:when test="@type = 'short text'"><xsl:text>1</xsl:text></xsl:when>
    <xsl:when test="@type = 'formatted'"><xsl:text>1</xsl:text></xsl:when>
    <xsl:when test="@type = 'long text'"><xsl:text>2</xsl:text></xsl:when>
    <xsl:when test="@type = 'options'"><xsl:text>3</xsl:text></xsl:when>
    <xsl:when test="@type = 'yesno'"><xsl:text>4</xsl:text></xsl:when>
    <xsl:when test="@type = 'number' and @displayas = 'text'"><xsl:text>6</xsl:text></xsl:when>
    <xsl:when test="@type = 'number' and @displayas = 'graphical'"><xsl:text>14</xsl:text></xsl:when>
    <xsl:when test="@type = 'single list'"><xsl:text>8</xsl:text></xsl:when>
    <xsl:when test="@type = 'image'"><xsl:text>10</xsl:text></xsl:when>
    <xsl:when test="@type = 'date'"><xsl:text>12</xsl:text></xsl:when>
   </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="flags">
   <xsl:choose>
    <xsl:when test="@type = 'options'"><xsl:text>2</xsl:text></xsl:when>
   </xsl:choose>
  </xsl:attribute>
  <xsl:if test="string-length(@values)">
   <xsl:attribute name="allowed">
    <xsl:value-of select="translate(@values, ',', ';')"/>
   </xsl:attribute>
  </xsl:if>
  <xsl:if test="string-length(@init) and not(@type='formatted')">
   <tc:prop name="default">
    <xsl:value-of select="@init"/>
   </tc:prop>
  </xsl:if>
  <xsl:if test="string-length(@max)">
   <tc:prop name="maximum">
    <xsl:value-of select="@max"/>
   </tc:prop>
  </xsl:if>
  <xsl:if test="string-length(@min)">
   <tc:prop name="minimum">
    <xsl:value-of select="@min"/>
   </tc:prop>
  </xsl:if>
  <xsl:if test="@type='formatted'">
   <xsl:attribute name="flags">32</xsl:attribute>
   <xsl:variable name="tokens" select="str:tokenize(@init, '%')"/>
   <tc:prop name="template">
    <xsl:for-each select="$tokens[position() mod 2 = 1]">
     <xsl:value-of select="concat('%{', ., '}', following-sibling::*)"/>
    </xsl:for-each>
   </tc:prop>
  </xsl:if>
  <xsl:if test="@type='simple list'">
   <tc:prop name="columns">1</tc:prop>
  </xsl:if>
 </tc:field>
</xsl:template>

<xsl:template match="item">
 <!-- For GCstar 1.2, the XML schema changed to use attributes for all
      the 'simple' value types. The workaround here is to parse all
      the attributes and elements. For each attribute, a dummy
      element is created with the same name as the attribute, and
      that dummy element is then parsed. This way, I can use the same
      stylesheet for either format without having to repeat a lot
      of XPath expressions. -->
 <!-- simplicity, the id will either be an attribute or an element -->
 <tc:entry id="{(id|@id|@gcsautoid)[1]}">
  <xsl:apply-templates select="@*|*"/>
 </tc:entry>
</xsl:template>

<xsl:template match="item/@*[not(starts-with(local-name(), 'gcsfield'))]">
 <xsl:variable name="dummies">
  <xsl:element name="{local-name(.)}">
   <xsl:value-of select="."/>
  </xsl:element>
 </xsl:variable>
 <xsl:for-each select="exsl:node-set($dummies)/*">
  <xsl:apply-templates select="."/>
 </xsl:for-each>
</xsl:template>

<xsl:template match="item/@*[starts-with(local-name(), 'gcsfield')]">
 <xsl:variable name="elemName" select="local-name()"/>
 <!-- have to combine possibility of either local custom fields or external custom collection mode -->
 <xsl:variable name="fieldType" select="(/collection/userCollection/fields/field[@value = $elemName] |
                                         exsl:node-set($externalModel)/fields/field[@value = $elemName])[1]/@type"/>
 <xsl:element name="{concat('tc:',$elemName)}">
  <xsl:choose>
   <xsl:when test="$fieldType = 'date'">
    <xsl:call-template name="date">
     <xsl:with-param name="value" select="."/>
    </xsl:call-template>
   </xsl:when>
   <xsl:when test="$fieldType = 'yesno'">
    <xsl:if test=". = '1'">true</xsl:if>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="."/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:element>
</xsl:template>

<!-- ignore anything not explicit -->
<xsl:template match="*"/>

<!-- the easy one matches identical local names -->
<xsl:template match="title|isbn|edition|pages|label|platform|location|vintage|quantity|soil|alcohol|collection|series|currency|diameter|estimate">
 <xsl:element name="{concat('tc:',local-name())}">
  <xsl:value-of select="."/>
 </xsl:element>
</xsl:template>

<xsl:template match="original">
 <tc:origtitle><xsl:value-of select="."/></tc:origtitle>
</xsl:template>

<xsl:template match="author|authors|language|genre|artist|composer|producer|developer|grapes|writer|illustrator|colourist|numberboards">
 <xsl:variable name="tag">
  <xsl:choose>
   <xsl:when test="local-name() = 'authors'">
    <xsl:text>author</xsl:text>
   </xsl:when>
   <xsl:when test="local-name() = 'grapes'">
    <xsl:text>varietal</xsl:text>
   </xsl:when>
   <xsl:when test="local-name() = 'illustrator'">
    <xsl:text>artist</xsl:text>
   </xsl:when>
   <xsl:when test="local-name() = 'colourist'">
    <xsl:text>colorist</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="local-name()"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>

 <xsl:choose>
  <xsl:when test="$tag='producer' and $coll = 7">
   <tc:producer>
    <xsl:value-of select="normalize-space(.)"/>
   </tc:producer>
  </xsl:when>
  <xsl:when test="line">
   <xsl:element name="{concat('tc:',$tag,'s')}">
    <xsl:for-each select="line">
     <xsl:for-each select="str:tokenize(col[1], ',/;')">
      <xsl:element name="{concat('tc:',$tag)}">
       <xsl:attribute name="i18n">true</xsl:attribute>
       <xsl:value-of select="normalize-space(.)"/>
      </xsl:element>
     </xsl:for-each>
    </xsl:for-each>
   </xsl:element>
  </xsl:when>

  <xsl:otherwise>
   <xsl:element name="{concat('tc:',$tag,'s')}">
    <xsl:for-each select="str:tokenize(., ',/;')">
     <xsl:element name="{concat('tc:',$tag)}">
      <xsl:attribute name="i18n">true</xsl:attribute>
      <xsl:value-of select="normalize-space(.)"/>
     </xsl:element>
    </xsl:for-each>
   </xsl:element>
  </xsl:otherwise>

 </xsl:choose>
</xsl:template>

<xsl:template match="format">
 <xsl:choose>
  <xsl:when test="$coll = 2">
   <tc:binding i18n="true">
    <xsl:choose>
     <xsl:when test="contains(., 'Paperback')">
      <xsl:text>Paperback</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="."/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:binding>
  </xsl:when>
  <xsl:when test="$coll = 3">
   <tc:medium i18n="true"><xsl:value-of select="."/></tc:medium>
  </xsl:when>
  <xsl:when test="$coll = 4">
   <tc:medium i18n="true">
    <xsl:choose>
     <xsl:when test="text()='CD'">
      <xsl:text>Compact Disc</xsl:text>
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="."/>
     </xsl:otherwise>
    </xsl:choose>
   </tc:medium>
  </xsl:when>
  <xsl:when test="$coll = 6">
   <tc:format i18n="true"><xsl:value-of select="."/></tc:format>
  </xsl:when>
 </xsl:choose>
</xsl:template>

<xsl:template match="country">
 <xsl:choose>
  <xsl:when test="$coll !=6 and $coll != 7 and $coll != 8">
   <tc:nationalitys>
    <xsl:for-each select="str:tokenize(., ',/;')">
     <tc:nationality i18n="true">
      <xsl:value-of select="normalize-space(.)"/>
     </tc:nationality>
    </xsl:for-each>
   </tc:nationalitys>
  </xsl:when>
  <xsl:otherwise>
   <tc:country>
    <xsl:value-of select="."/>
   </tc:country>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>

<xsl:template match="actors">
 <tc:casts>
  <xsl:if test="line">
   <xsl:for-each select="line">
    <tc:cast>
     <tc:column>
      <xsl:value-of select="col[1]"/>
     </tc:column>
     <tc:column>
      <xsl:value-of select="col[2]"/>
     </tc:column>
    </tc:cast>
   </xsl:for-each>
  </xsl:if>
  <xsl:if test="not(line)">
   <xsl:for-each select="str:tokenize(., ',/;')">
    <tc:cast>
     <!-- could have role name in parentheses -->
     <xsl:if test="contains(., '(')">
      <tc:column>
       <xsl:value-of select="normalize-space(substring-before(., '('))"/>
      </tc:column>
      <tc:column>
       <xsl:variable name="role">
        <xsl:call-template name="substring-before-last">
         <xsl:with-param name="input" select="substring-after(., '(')"/>
         <xsl:with-param name="substr" select="')'"/>
        </xsl:call-template>
       </xsl:variable>
       <xsl:value-of select="normalize-space($role)"/>
      </tc:column>
     </xsl:if>
     <xsl:if test="not(contains(., '('))">
      <tc:column>
       <xsl:value-of select="normalize-space(.)"/>
      </tc:column>
      <tc:column/>
     </xsl:if>
    </tc:cast>
   </xsl:for-each>
  </xsl:if>
 </tc:casts>
</xsl:template>

<xsl:template match="director">
 <tc:directors>
  <tc:director>
   <xsl:value-of select="."/>
  </tc:director>
 </tc:directors>
</xsl:template>

<xsl:template match="translator">
 <tc:translators>
  <tc:translator>
   <xsl:value-of select="."/>
  </tc:translator>
 </tc:translators>
</xsl:template>

<xsl:template match="audio">
 <tc:languages>
  <xsl:for-each select="line">
   <tc:language>
    <xsl:value-of select="col[1]"/>
   </tc:language>
  </xsl:for-each>
 </tc:languages>
 <tc:audio-tracks>
  <xsl:for-each select="line">
   <tc:audio-track>
    <xsl:value-of select="col[2]"/>
   </tc:audio-track>
  </xsl:for-each>
 </tc:audio-tracks>
</xsl:template>

<xsl:template match="synopsis">
 <tc:plot><xsl:value-of select="."/></tc:plot>
</xsl:template>

<xsl:template match="comment">
 <tc:comments><xsl:value-of select="."/></tc:comments>
</xsl:template>

<xsl:template match="place">
 <tc:location><xsl:value-of select="."/></tc:location>
</xsl:template>

<xsl:template match="image|boxpic|cover">
 <xsl:call-template name="image-element">
  <xsl:with-param name="elem" select="'tc:cover'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="bottlelabel">
 <xsl:call-template name="image-element">
  <xsl:with-param name="elem" select="'tc:label'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="web|webPage">
 <tc:url><xsl:value-of select="."/></tc:url>
</xsl:template>

<xsl:template match="rating">
 <xsl:if test=". &gt; 0">
  <tc:rating><xsl:value-of select="floor(. div 2)"/></tc:rating>
 </xsl:if>
</xsl:template>

<xsl:template match="age">
 <tc:certification i18n="true">
  <xsl:choose>
   <xsl:when test=". = 1">
    <xsl:text>U (USA)</xsl:text>
   </xsl:when>
   <xsl:when test=". = 2">
    <xsl:text>G (USA)</xsl:text>
   </xsl:when>
   <xsl:when test=". &lt; 6">
    <xsl:text>PG (USA)</xsl:text>
   </xsl:when>
   <xsl:when test=". &lt; 14">
    <xsl:text>PG-13 (USA)</xsl:text>
   </xsl:when>
   <xsl:when test=". &lt; 19">
    <xsl:text>R (USA)</xsl:text>
   </xsl:when>
  </xsl:choose>
 </tc:certification>
</xsl:template>

<xsl:template match="time">
 <tc:running-time>
  <xsl:value-of select="translate(.,translate(.,'0123456789',''),'')"/>
 </tc:running-time>
</xsl:template>

<xsl:template match="video">
 <tc:format i18n="true">
  <xsl:value-of select="translate(.,'abcdefghijklmnopqrstuvwxyz',
                                    'ABCDEFGHIJKLMNOPQRSTUCWXYZ')"/>
 </tc:format>
</xsl:template>

<xsl:template match="borrower">
 <xsl:if test="string-length(.) &gt; 0 and . != 'none'">
  <tc:loaned>true</tc:loaned>
 </xsl:if>
</xsl:template>

<xsl:template match="read">
 <xsl:if test=". &gt; 0">
  <tc:read>true</tc:read>
 </xsl:if>
</xsl:template>

<xsl:template match="seen">
 <xsl:if test=". &gt; 0">
  <tc:seen>true</tc:seen>
 </xsl:if>
</xsl:template>

<xsl:template match="favourite">
 <xsl:if test=". &gt; 0">
  <tc:favorite>true</tc:favorite>
 </xsl:if>
</xsl:template>

<xsl:template match="publication|publishdate">
 <tc:pub_year>
  <xsl:call-template name="year">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </tc:pub_year>
</xsl:template>

<xsl:template match="acquisition|purchasedate">
 <xsl:variable name="numbers" select="str:tokenize(., '/-')"/>
 <xsl:if test="count($numbers) = 3">
  <tc:pur_date>
   <xsl:value-of select="concat($numbers[3],'-',$numbers[2],'-',$numbers[1])"/>
  </tc:pur_date>
 </xsl:if>
</xsl:template>

<xsl:template match="added">
 <tc:cdate calendar="gregorian">
  <xsl:call-template name="date">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </tc:cdate>
 <!-- last modified date is same as created -->
 <tc:mdate calendar="gregorian">
  <xsl:call-template name="date">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </tc:mdate>
</xsl:template>

<xsl:template match="serie">
 <tc:series><xsl:value-of select="."/></tc:series>
</xsl:template>

<xsl:template match="comments">
 <tc:comments>
  <xsl:if test="$coll = 2 and ../description">
   &lt;br/&gt;
   <xsl:value-of select="../description"/>
   &lt;br/&gt;
  </xsl:if>
  <xsl:value-of select="."/>
 </tc:comments>
</xsl:template>

<xsl:template match="description|tasting">
 <xsl:if test="$coll != 2">
  <tc:description><xsl:value-of select="."/></tc:description>
 </xsl:if>
</xsl:template>

<xsl:template match="volume">
 <xsl:if test="$coll != 6">
  <tc:volume><xsl:value-of select="."/></tc:volume>
 </xsl:if>
 <xsl:if test="$coll = 6">
  <tc:issue><xsl:value-of select="."/></tc:issue>
 </xsl:if>
</xsl:template>

<xsl:template match="year|date|release|released">
 <tc:year>
  <xsl:call-template name="year">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </tc:year>
</xsl:template>

<xsl:template match="tracks">
 <tc:tracks>
  <xsl:for-each select="line">
   <tc:track>
    <tc:column>
     <xsl:value-of select="col[2]"/>
    </tc:column>
    <tc:column>
     <xsl:value-of select="(../../artist|../../@artist)[1]"/>
    </tc:column>
    <tc:column>
     <xsl:value-of select="col[3]"/>
    </tc:column>
   </tc:track>
  </xsl:for-each>
 </tc:tracks>
</xsl:template>

<xsl:template match="name">
 <tc:title><xsl:value-of select="."/></tc:title>
</xsl:template>

<xsl:template match="completion">
 <xsl:if test=". &gt; 99">
  <tc:completed>true</tc:completed>
 </xsl:if>
</xsl:template>

<xsl:template match="editor|publisher|publishedby">
 <tc:publishers>
  <xsl:for-each select="str:tokenize(., ',/;')">
   <tc:publisher>
    <xsl:value-of select="normalize-space(.)"/>
   </tc:publisher>
  </xsl:for-each>
 </tc:publishers>
</xsl:template>

<xsl:template match="tags">
 <tc:keywords>
  <xsl:for-each select="line">
   <tc:keyword>
    <xsl:value-of select="col[1]"/>
   </tc:keyword>
  </xsl:for-each>
 </tc:keywords>
</xsl:template>

<xsl:template match="subt">
 <tc:subtitles>
  <xsl:for-each select="line">
   <tc:subtitle>
    <xsl:value-of select="col[1]"/>
   </tc:subtitle>
  </xsl:for-each>
 </tc:subtitles>
</xsl:template>

<xsl:template match="metal">
 <tc:metals>
  <xsl:for-each select="line">
   <tc:metal>
    <tc:column>
     <xsl:value-of select="col[1]"/>
    </tc:column>
   </tc:metal>
  </xsl:for-each>
 </tc:metals>
</xsl:template>

<xsl:template match="value">
 <tc:denomination><xsl:value-of select="."/></tc:denomination>
</xsl:template>

<xsl:template match="purchaseprice|cost">
 <tc:pur_price><xsl:value-of select="."/></tc:pur_price>
</xsl:template>

<xsl:template match="front|picture">
 <xsl:call-template name="image-element">
  <xsl:with-param name="elem" select="'tc:obverse'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="back">
 <xsl:call-template name="image-element">
  <xsl:with-param name="elem" select="'tc:reverse'"/>
  <xsl:with-param name="value" select="."/>
 </xsl:call-template>
</xsl:template>

<xsl:template match="designedby">
 <tc:designers>
  <xsl:for-each select="str:tokenize(., ',/;')">
   <tc:designer>
    <xsl:value-of select="normalize-space(.)"/>
   </tc:designer>
  </xsl:for-each>
 </tc:designers>
</xsl:template>

<xsl:template match="players">
 <tc:num-players>
  <!-- need to parse ranges, like 2-6 -->
  <xsl:for-each select="str:tokenize(., ',/;')">
   <tc:num-player>
    <xsl:value-of select="normalize-space(.)"/>
   </tc:num-player>
  </xsl:for-each>
 </tc:num-players>
</xsl:template>

<xsl:template match="mechanics">
 <tc:mechanisms>
  <xsl:for-each select="line">
   <tc:mechanism>
    <xsl:value-of select="col[1]"/>
   </tc:mechanism>
  </xsl:for-each>
 </tc:mechanisms>
</xsl:template>

<xsl:template match="category">
 <xsl:if test="$coll = 6">
  <tc:category><xsl:value-of select="."/></tc:category>
 </xsl:if>
 <xsl:if test="$coll != 6">
  <tc:genres>
   <xsl:for-each select="line">
    <tc:genre>
     <xsl:value-of select="col[1]"/>
    </tc:genre>
   </xsl:for-each>
  </tc:genres>
 </xsl:if>
</xsl:template>

<!-- coins -->
<xsl:template match="condition">
<!-- by default, Tellico includes
        "Proof-65,Proof-60,Mint State-65,Mint State-60,"
        "Almost Uncirculated-55,Almost Uncirculated-50,"
        "Extremely Fine-40,Very Fine-30,Very Fine-20,Fine-12,"
        "Very Good-8,Good-4,Fair" -->
<!-- GCStar doesn't appear to include proof grades -->
 <tc:grade>
  <xsl:choose>
   <xsl:when test=".='70'">Mint State-70</xsl:when>
   <xsl:when test=".='69'">Mint State-69</xsl:when>
   <xsl:when test=".='68'">Mint State-68</xsl:when>
   <xsl:when test=".='67'">Mint State-67</xsl:when>
   <xsl:when test=".='66'">Mint State-66</xsl:when>
   <xsl:when test=".='65'">Mint State-65</xsl:when>
   <xsl:when test=".='64'">Mint State-64</xsl:when>
   <xsl:when test=".='63'">Mint State-63</xsl:when>
   <xsl:when test=".='62'">Mint State-62</xsl:when>
   <xsl:when test=".='61'">Mint State-61</xsl:when>
   <xsl:when test=".='60'">Mint State-60</xsl:when>
   <xsl:when test=".='58'">Almost Uncirculated-58</xsl:when>
   <xsl:when test=".='55'">Almost Uncirculated-55</xsl:when>
   <xsl:when test=".='53'">Almost Uncirculated-53</xsl:when>
   <xsl:when test=".='50'">Almost Uncirculated-50</xsl:when>
   <xsl:when test=".='45'">Extremely Fine-45</xsl:when>
   <xsl:when test=".='40'">Extremely Fine-40</xsl:when>
   <xsl:when test=".='35'">Very Fine-35</xsl:when>
   <xsl:when test=".='30'">Very Fine-30</xsl:when>
   <xsl:when test=".='25'">Very Fine-25</xsl:when>
   <xsl:when test=".='20'">Very Fine-20</xsl:when>
   <xsl:when test=".='15'">Fine-15</xsl:when>
   <xsl:when test=".='12'">Fine-12</xsl:when>
   <xsl:when test=".='10'">Very Good-10</xsl:when>
   <xsl:when test=".='8'">Very Good-8</xsl:when>
   <xsl:when test=".='6'">Good-6</xsl:when>
   <xsl:when test=".='4'">Good-4</xsl:when>
   <xsl:when test=".&lt;4">Fair</xsl:when>
  </xsl:choose>
 </tc:grade>
 <!-- GCstar defaults to PCGS -->
 <tc:service>PCGS</tc:service>
</xsl:template>

<xsl:template match="designation">
 <tc:appellation><xsl:value-of select="."/></tc:appellation>
</xsl:template>

<xsl:template match="type">
 <tc:type i18n="true">
  <xsl:choose>
   <xsl:when test="contains(.,'red')">Red Wine</xsl:when>
   <xsl:when test="contains(.,'white')">White Wine</xsl:when>
   <xsl:when test="contains(.,'champagne') or contains(.,'sparking')">Sparkling Wine</xsl:when>
   <xsl:otherwise><xsl:value-of select="."/></xsl:otherwise>
  </xsl:choose>
 </tc:type>
</xsl:template>

<xsl:template match="medal">
 <tc:distinction><xsl:value-of select="."/></tc:distinction>
</xsl:template>

<xsl:template match="gift">
 <xsl:if test="string-length(.) &gt; 0 and . != 'false' and . != '0'">
  <tc:gift>true</tc:gift>
 </xsl:if>
</xsl:template>

<xsl:template match="tasted">
 <xsl:if test="string-length(.) &gt; 0 and . != 'false' and . != '0'">
  <tc:tasted>true</tc:tasted>
 </xsl:if>
</xsl:template>

<xsl:template match="signing">
 <xsl:if test="string-length(.) &gt; 0 and . != 'false' and . != '0'">
  <tc:signed>true</tc:signed>
 </xsl:if>
</xsl:template>

<xsl:template match="*[starts-with(local-name(), 'gcsfield')]">
 <xsl:variable name="elemName" select="local-name()"/>
 <!-- have to combine possibility of either local custom fields or external custom collection mode -->
 <xsl:variable name="fieldType" select="(/collection/userCollection/fields/field[@value = $elemName] |
                                         exsl:node-set($externalModel)/fields/field[@value = $elemName])[1]/@type"/>
 <xsl:choose>
  <xsl:when test="$fieldType = 'single list'">
   <xsl:element name="{concat('tc:', $elemName, 's')}">
    <xsl:for-each select="line">
     <xsl:element name="{concat('tc:', $elemName)}">
      <tc:column>
       <xsl:value-of select="col"/>
      </tc:column>
     </xsl:element>
    </xsl:for-each>
   </xsl:element>
  </xsl:when>
  <xsl:otherwise>
   <xsl:element name="{concat('tc:', $elemName)}">
    <xsl:value-of select="."/>
   </xsl:element>
  </xsl:otherwise>
 </xsl:choose>
</xsl:template>
 
<xsl:template name="image-element">
 <xsl:param name="elem"/>
 <xsl:param name="value"/>
 <xsl:element name="{$elem}">
  <xsl:choose>
   <!-- no.png means no image -->
   <xsl:when test="$value = 'images/no.png'">
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="$value"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:element>
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

<xsl:template name="date">
 <xsl:param name="value"/>
 <xsl:variable name="numbers" select="str:tokenize($value, '/-')"/>
 <xsl:if test="count($numbers) = 3">
  <tc:year><xsl:value-of select="$numbers[3]"/></tc:year>
  <tc:month><xsl:value-of select="$numbers[2]"/></tc:month>
  <tc:day><xsl:value-of select="$numbers[1]"/></tc:day>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
