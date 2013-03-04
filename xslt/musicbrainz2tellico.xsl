<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                xmlns:str="http://exslt.org/strings"
                xmlns:mb="http://musicbrainz.org/ns/mmd-1.0#"
                exclude-result-prefixes="mb"
                extension-element-prefixes="str"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing musicbrainz data

   Copyright (C) 2009 Robby Stephenson -robby@periapsis.org

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
   <xsl:value-of select="substring(mb:release-event-list/mb:event[1]/@date, 1, 4)"/>
  </year>

  <artists>
   <xsl:for-each select="mb:artist">
    <artist>
     <xsl:value-of select="mb:name"/>
    </artist>
   </xsl:for-each>
  </artists>

  <labels>
   <!-- for now, just use first release -->
   <!-- <xsl:for-each select="mb:release-event-list/mb:event/mb:label[not(.=preceding::mb:label)]"> -->
   <xsl:for-each select="mb:release-event-list/mb:event[1]/mb:label">
    <label>
     <xsl:value-of select="mb:name"/>
    </label>
   </xsl:for-each>
  </labels>

  <xsl:if test="mb:release-event-list/mb:event[1]/@format = 'CD'">
   <medium i18n='yes'>Compact Disc</medium>
  </xsl:if>
  <xsl:if test="mb:release-event-list/mb:event[1]/@format = 'Cassette'">
   <medium i18n='yes'>Cassette</medium>
  </xsl:if>
  <xsl:if test="mb:release-event-list/mb:event[1]/@format = 'Vinyl'">
   <medium i18n='yes'>Vinyl</medium>
  </xsl:if>

  <genres>
   <xsl:if test="contains(@type,'oundtrack')">
    <genre i18n='yes'>Soundtrack</genre>
   </xsl:if>
  </genres>

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
  <tracks>
   <xsl:for-each select="mb:track-list/mb:track">
    <track>
     <column>
     <xsl:value-of select="mb:title"/>
     </column>
     <column>
     <xsl:choose>
      <xsl:when test="mb:artist">
       <!-- some combinationss are separated by &,but some artists use & -->
       <!-- some combinations uses 'and' -->
       <!-- no way to accurately split, just setlle on comma for now -->
       <xsl:value-of select="translate(mb:artist/mb:name,',', ';')"/>
      </xsl:when>
      <xsl:otherwise>
       <xsl:value-of select="$release/mb:artist/mb:name"/>
      </xsl:otherwise>
     </xsl:choose>
     </column>
     <column>
     <xsl:call-template name="time">
      <xsl:with-param name="duration" select="mb:duration"/>
     </xsl:call-template>
     </column>
    </track>
   </xsl:for-each>
  </tracks>

  <cover>
   <xsl:choose>
    <xsl:when test="mb:relation-list[@target-type='Url']/mb:relation[@type='CoverArtLink']">
     <xsl:value-of select="mb:relation-list[@target-type='Url']/mb:relation[@type='CoverArtLink'][1]/@target"/>
    </xsl:when>
    <!-- most musicbrainz items have Amazon  ASIN values, but not all point got valid images
         if the AmazonAsin Url is set, then it likely does -->
    <xsl:when test="mb:relation-list[@target-type='Url']/mb:relation[@type='AmazonAsin']">
     <xsl:value-of select="concat('http://ecx.images-amazon.com/images/P/',mb:asin,'.01.MZZZZZZZ.jpg')"/>
    </xsl:when>
    <!-- TODO: when switching to ws/2, check cover-art-archive element for existence of front cover art
               before setting the value, so an extra http call is not done -->
    <xsl:otherwise>
     <xsl:value-of select="concat('http://coverartarchive.org/release/',@id,'/front')"/>
    </xsl:otherwise>
   </xsl:choose>
  </cover>

 </entry>
</xsl:template>

<xsl:template name="time">
 <xsl:param name="duration"/>
 <!-- musicbrainz uses milliseconds -->
 <xsl:value-of select="floor($duration div 1000 div 60)"/>
 <xsl:text>:</xsl:text>
 <xsl:value-of select="format-number(($duration div 1000) mod 60,'00')"/>
</xsl:template>

</xsl:stylesheet>
