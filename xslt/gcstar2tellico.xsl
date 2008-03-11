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

   Copyright (C) 2007 Robby Stephenson - robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://www.periapsis.org/tellico/

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V9.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v9/tellico.dtd"/>

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
   </xsl:choose>
</xsl:variable>

<xsl:template match="/">
 <tellico syntaxVersion="9">
  <xsl:apply-templates select="collection"/>
 </tellico>
</xsl:template>

<xsl:template match="collection">
 <tc:collection title="GCstar Import" type="{$coll}">
  <tc:fields>
   <tc:field name="_default"/>
  </tc:fields>
  <xsl:apply-templates select="item"/>
 </tc:collection>
</xsl:template>

<xsl:template match="item">
 <!-- For GCstar 1.2, the XML schema changed to use attributes for all
      the 'simple' value types. The workaround here is to parse all
      the attributes and elements. For each attribute, a dummy
      element is created with the same name as the attribute, and
      that dummy element is then parsed. This way, I can use the same
      stylesheet for either format without having to repeat a lot
      of XPath expression. -->
 <!-- simplicity, the id will either be an attribute or an element -->
 <tc:entry id="{concat(id,@id)}">
  <xsl:apply-templates select="@*|*"/>
 </tc:entry>
</xsl:template>

<xsl:template match="item/@*">
 <xsl:variable name="dummies">
  <xsl:element name="{local-name(.)}">
   <xsl:value-of select="."/>
  </xsl:element>
 </xsl:variable>
 <xsl:for-each select="exsl:node-set($dummies)/*">
  <xsl:apply-templates select="."/>
 </xsl:for-each>
</xsl:template>

<!-- the easy one matches identical local names -->
<xsl:template match="title|isbn|edition|pages|label|platform|location">
 <xsl:element name="{concat('tc:',local-name())}">
  <xsl:value-of select="."/>
 </xsl:element>
</xsl:template>

<xsl:template match="author|authors|language|genre|artist">
 <xsl:variable name="tag">
  <xsl:choose>
   <xsl:when test="local-name() = 'authors'">
    <xsl:text>author</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="local-name()"/>
   </xsl:otherwise> 
  </xsl:choose>
 </xsl:variable>

 <xsl:choose>
  <xsl:when test="line">
   <xsl:element name="{concat('tc:',$tag,'s')}">
    <xsl:for-each select="line">
     <xsl:element name="{concat('tc:',$tag)}">
      <xsl:attribute name="i18n">true</xsl:attribute>
      <xsl:value-of select="col[1]"/>
     </xsl:element>
    </xsl:for-each>
   </xsl:element>
  </xsl:when>

  <xsl:otherwise>
   <xsl:element name="{concat('tc:',$tag)}">
    <xsl:attribute name="i18n">true</xsl:attribute>
    <xsl:value-of select="."/>
   </xsl:element>
  </xsl:otherwise>

 </xsl:choose>
</xsl:template>

<xsl:template match="format">
 <xsl:if test="$coll = 2">
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
 </xsl:if>
 <xsl:if test="$coll != 2">
  <tc:medium i18n="true"><xsl:value-of select="."/></tc:medium>
 </xsl:if>
</xsl:template>

<xsl:template match="country">
 <xsl:if test="$coll != 8">
  <tc:nationalitys>
   <xsl:for-each select="str:tokenize(., ',/;')">
    <tc:nationality i18n="true">
     <xsl:value-of select="normalize-space(.)"/>
    </tc:nationality>
   </xsl:for-each>
  </tc:nationalitys>
 </xsl:if>
 <xsl:if test="$coll = 8">
  <tc:country><xsl:value-of select="."/></tc:country>
 </xsl:if>
</xsl:template>

<xsl:template match="actors">
 <tc:casts>
  <xsl:for-each select="str:tokenize(., ',/;')">
   <tc:cast>
    <tc:column>
     <xsl:value-of select="normalize-space(.)"/>
    </tc:column>
   </tc:cast>
  </xsl:for-each>
 </tc:casts>
</xsl:template>

<xsl:template match="director">
 <tc:directors>
  <tc:director>
   <xsl:value-of select="."/>
  </tc:director>
 </tc:directors>
</xsl:template>

<xsl:template match="audio">
 <tc:languages>
  <xsl:for-each select="line">
   <tc:language>
    <xsl:value-of select="col[1]"/>
   </tc:language>
  </xsl:for-each>
 </tc:languages>
</xsl:template>

<xsl:template match="synopsis">
 <tc:plot><xsl:value-of select="."/></tc:plot>
</xsl:template>

<xsl:template match="comment">
 <tc:comments><xsl:value-of select="."/></tc:comments>
</xsl:template>

<xsl:template match="image|boxpic">
 <tc:cover><xsl:value-of select="."/></tc:cover>
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
   <xsl:when test=". &lt; 18">
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

<xsl:template match="publication">
 <tc:pub_year>
  <xsl:call-template name="year">
   <xsl:with-param name="value" select="."/>
  </xsl:call-template>
 </tc:pub_year>
</xsl:template>

<xsl:template match="aquisition|added">
 <tc:pur_date><xsl:value-of select="."/></tc:pur_date>
</xsl:template>

<xsl:template match="serie">
 <tc:series><xsl:value-of select="."/></tc:series>
</xsl:template>

<xsl:template match="comments">
 <tc:comments>
  <xsl:if test="$coll = 2">
   &lt;br/&gt;
   <xsl:value-of select="../description"/>
   &lt;br/&gt;
  </xsl:if>
  <xsl:value-of select="."/>
 </tc:comments>
</xsl:template>

<xsl:template match="description">
 <xsl:if test="$coll != 2">
  <tc:description><xsl:value-of select="."/></tc:description>
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
     <xsl:value-of select="../../artist"/>
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
<xsl:template match="*"/>

<xsl:template match="editor|publisher">
 <tc:publishers>
  <tc:publisher>
   <xsl:value-of select="."/>
  </tc:publisher>
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

<xsl:template match="currency">
 <tc:type><xsl:value-of select="."/></tc:type>
</xsl:template>

<xsl:template match="value">
 <tc:denomination><xsl:value-of select="."/></tc:denomination>
</xsl:template>

<xsl:template match="estimate">
 <tc:pur_price><xsl:value-of select="."/></tc:pur_price>
</xsl:template>

<xsl:template match="front">
 <tc:obverse><xsl:value-of select="."/></tc:obverse>
</xsl:template>

<xsl:template match="back">
 <tc:reverse><xsl:value-of select="."/></tc:reverse>
</xsl:template>

<xsl:template match="publishedby">
 <tc:publishers>
  <xsl:for-each select="str:tokenize(., ',/;')">
   <tc:publisher>
    <xsl:value-of select="normalize-space(.)"/>
   </tc:publisher>
  </xsl:for-each>
 </tc:publishers>
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
