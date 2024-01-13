<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:mb="http://musicbrainz.org/ns/mmd-2.0#"
                exclude-result-prefixes="mb"
                extension-element-prefixes="str"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing musicbrainz data, version 2

   Copyright (C) 2009-2018 Robby Stephenson -robby@periapsis.org

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <tellico syntaxVersion="11">
  <collection title="Search Results" type="4"> <!-- 4 is music -->
   <fields>
    <field name="_default"/>
    <field flags="0" title="MusicBrainz ID" category="General" format="4" type="1" name="mbid"/>
    <field flags="0" title="Barcode" category="General" format="4" type="1" name="barcode" i18n="true"/>
    <!-- allow for multi-disc albums, with separate track fields for each disc -->
    <xsl:if test="count(//mb:release/mb:medium-list/mb:medium) &gt; 1">
     <!-- update title of first track field -->
     <field flags="1" title="Tracks (Disc 1)" name="track" format="1" type="8" i18n="true">
      <prop name="column1">Title</prop>
      <prop name="column2">Artist</prop>
      <prop name="column3">Length</prop>
      <prop name="columns">3</prop>
     </field>
     <!-- new field for each disc -->
     <xsl:for-each select="//mb:release/mb:medium-list/mb:medium[position() &gt; 1]">
      <field flags="1" title="{concat('Tracks (Disc ', position()+1, ')')}"
       name="{concat('track', position()+1)}" format="1" type="8" i18n="true">
       <prop name="column1">Title</prop>
       <prop name="column2">Artist</prop>
       <prop name="column3">Length</prop>
       <prop name="columns">3</prop>
      </field>
     </xsl:for-each>
    </xsl:if>
   </fields>
   <xsl:apply-templates select="//mb:release"/>
  </collection>
 </tellico>
</xsl:template>

<xsl:template match="mb:release">
 <entry>
  <title>
   <xsl:value-of select="mb:title"/>
  </title>

  <mbid>
   <xsl:value-of select="@id"/>
  </mbid>

  <year>
   <xsl:value-of select="substring(mb:release-event-list/mb:release-event[1]/mb:date, 1, 4)"/>
  </year>

  <artists>
   <xsl:for-each select="mb:artist-credit/mb:name-credit">
    <artist>
     <xsl:value-of select="mb:artist/mb:name"/>
    </artist>
   </xsl:for-each>
  </artists>

  <labels>
   <xsl:for-each select="mb:label-info-list/mb:label-info">
    <label>
     <xsl:value-of select="mb:label/mb:name"/>
    </label>
   </xsl:for-each>
  </labels>

  <xsl:if test="mb:medium-list/mb:medium[1]/mb:format = 'CD'">
   <medium i18n='yes'>Compact Disc</medium>
  </xsl:if>
  <xsl:if test="mb:medium-list/mb:medium[1]/mb:format = 'Cassette'">
   <medium i18n='yes'>Cassette</medium>
  </xsl:if>
  <xsl:if test="mb:medium-list/mb:medium[1]/mb:format = 'Vinyl'">
   <medium i18n='yes'>Vinyl</medium>
  </xsl:if>

  <genres>
   <xsl:if test="contains(mb:release-group/@type,'oundtrack')">
    <genre i18n='yes'>Soundtrack</genre>
   </xsl:if>
  </genres>

  <barcode>
   <xsl:value-of select="mb:barcode"/>
  </barcode>

  <!-- tags are too random, don't use them -->
  <!--
  <keywords>
   <xsl:for-each select="mb:tag-list/mb:tag">
    <keyword>
     <xsl:value-of select="."/>
    </keyword>
   </xsl:for-each>
  </keywords>
  -->

  <xsl:variable name="release" select="."/>
  <!-- take care of tracks in first disc -->
  <tracks>
   <xsl:for-each select="mb:medium-list/mb:medium[1]/mb:track-list/mb:track">
    <xsl:sort select="mb:position"/>
    <xsl:call-template name="track">
     <xsl:with-param name="release" select="$release"/>
     <xsl:with-param name="track" select="."/>
     <xsl:with-param name="fieldName" select="'track'"/>
    </xsl:call-template>
   </xsl:for-each>
  </tracks>

  <!-- account for multi-disc albums -->
  <xsl:for-each select="mb:medium-list/mb:medium[position() &gt; 1]">
   <xsl:variable name="fieldName" select="concat('track', position()+1)"/>
   <xsl:element name="{concat($fieldName, 's')}">
    <xsl:for-each select="mb:track-list/mb:track">
     <xsl:sort select="mb:position"/>
     <xsl:call-template name="track">
      <xsl:with-param name="release" select="$release"/>
      <xsl:with-param name="track" select="."/>
      <xsl:with-param name="fieldName" select="$fieldName"/>
     </xsl:call-template>
    </xsl:for-each>
   </xsl:element>
  </xsl:for-each>

  <cover>
   <xsl:choose>
    <xsl:when test="mb:cover-art-archive/mb:front='true'">
     <xsl:value-of select="concat('http://coverartarchive.org/release/',@id,'/front')"/>
    </xsl:when>
    <xsl:when test="mb:relation-list[@target-type='Url']/mb:relation[@type='CoverArtLink']">
     <xsl:value-of select="mb:relation-list[@target-type='Url']/mb:relation[@type='CoverArtLink'][1]/@target"/>
    </xsl:when>
    <!-- most musicbrainz items have Amazon ASIN values, but not all point got valid images
         if the AmazonAsin Url is set, then it likely does -->
    <xsl:when test="mb:relation-list[@target-type='url']/mb:relation[@type='amazon asin'] and mb:asin">
     <xsl:value-of select="concat('http://ecx.images-amazon.com/images/P/',mb:asin,'.01.MZZZZZZZ.jpg')"/>
    </xsl:when>
   </xsl:choose>
  </cover>

 </entry>
</xsl:template>

<xsl:template name="track">
 <xsl:param name="release"/>
 <xsl:param name="track"/>
 <xsl:param name="fieldName"/>

 <xsl:element name="{$fieldName}">
  <column>
   <xsl:value-of select="$track/mb:recording/mb:title"/>
  </column>
  <column>
   <xsl:choose>
    <xsl:when test="$track/mb:recording/mb:artist">
     <!-- some combinationss are separated by &,but some artists use & -->
     <!-- some combinations uses 'and' -->
     <!-- no way to accurately split, just settle on comma for now -->
     <xsl:value-of select="translate($track/mb:recording/mb:artist/mb:name,',', ';')"/>
    </xsl:when>
    <xsl:otherwise>
     <xsl:value-of select="$release/mb:artist-credit/mb:name-credit/mb:artist[1]/mb:name"/>
    </xsl:otherwise>
   </xsl:choose>
  </column>
  <column>
   <xsl:call-template name="time">
    <xsl:with-param name="duration" select="$track/mb:length"/>
   </xsl:call-template>
  </column>
 </xsl:element>
</xsl:template>

<xsl:template name="time">
 <xsl:param name="duration"/>
 <!-- musicbrainz uses milliseconds -->
 <xsl:value-of select="floor($duration div 1000 div 60)"/>
 <xsl:text>:</xsl:text>
 <xsl:value-of select="format-number(($duration div 1000) mod 60,'00')"/>
</xsl:template>

</xsl:stylesheet>
