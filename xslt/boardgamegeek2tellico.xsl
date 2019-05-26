<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://periapsis.org/tellico/"
                version="1.0">

<!--
   ===================================================================
   Tellico XSLT file - used for importing data from boardgamegeek.com

   Copyright (C) 2014-2017 Robby Stephenson <robby@periapsis.org>

   This XSLT stylesheet is designed to be used with the 'Tellico'
   application, which can be found at http://tellico-project.org

   ===================================================================
-->

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"
            doctype-public="-//Robby Stephenson/DTD Tellico V11.0//EN"
            doctype-system="http://periapsis.org/tellico/dtd/v11/tellico.dtd"/>

<xsl:template match="/">
 <xsl:variable name="type">
  <xsl:choose>
   <xsl:when test="videogames|items/item[@type='videogame' or @type='videogameexpansion']">11</xsl:when>
   <xsl:when test="boardgames|items/item[@type='boardgame' or @type='boardgameexpansion']">13</xsl:when>
  </xsl:choose>
 </xsl:variable>
 <tellico syntaxVersion="11">
  <collection title="BoardGameGeek Import">
   <xsl:attribute name="type">
    <xsl:value-of select="$type"/>
   </xsl:attribute>
   <fields>
    <field name="_default"/>
    <field flags="0" title="BoardGameGeek ID" category="General" format="4" type="6" name="bggid" i18n="true"/>
    <xsl:if test="$type='11'">
     <field flags="0" title="VideoGameGeek Link" category="General" format="4" type="7" name="videogamegeek-link" i18n="true"/>
    </xsl:if>
    <xsl:if test="$type='13'">
     <field title="Artist" flags="7" category="General" format="2" type="1" name="artist"/>
     <field flags="0" title="BoardGameGeek Link" category="General" format="4" type="7" name="boardgamegeek-link" i18n="true"/>
    </xsl:if>
   </fields>
   <xsl:apply-templates select="boardgames/boardgame"/>
   <xsl:apply-templates select="videogames/videogame"/>
   <xsl:apply-templates select="items/item"/>
  </collection>
 </tellico>
</xsl:template>

<!-- skip items that don't have specific type match -->
<xsl:template match="item">
</xsl:template>

<xsl:template match="boardgame|item[@type='boardgame' or @type='boardgameexpansion']">
 <entry>

  <title>
   <xsl:value-of select="name[@type='primary']/@value"/>
  </title>

  <year>
   <xsl:value-of select="yearpublished/@value"/>
  </year>

  <bggid>
   <xsl:value-of select="@id"/>
  </bggid>

  <boardgamegeek-link>
   <xsl:value-of select="concat('http://www.boardgamegeek.com/boardgame/', @id)"/>
  </boardgamegeek-link>

  <publishers>
   <xsl:for-each select="link[@type='boardgamepublisher']">
    <publisher>
     <xsl:value-of select="@value"/>
    </publisher>
   </xsl:for-each>
  </publishers>

  <designers>
   <xsl:for-each select="link[@type='boardgamedesigner']">
    <designer>
     <xsl:value-of select="@value"/>
    </designer>
   </xsl:for-each>
  </designers>

  <artists>
   <xsl:for-each select="link[@type='boardgameartist']">
    <artist>
     <xsl:value-of select="@value"/>
    </artist>
   </xsl:for-each>
  </artists>

  <genres>
   <xsl:for-each select="link[@type='boardgamecategory']">
    <genre>
     <xsl:value-of select="@value"/>
    </genre>
   </xsl:for-each>
  </genres>

  <mechanisms>
   <xsl:for-each select="link[@type='boardgamemechanic']">
    <mechanism>
     <xsl:value-of select="@value"/>
    </mechanism>
   </xsl:for-each>
  </mechanisms>

  <cover>
   <xsl:if test="starts-with(thumbnail, '//')">
    <xsl:text>https:</xsl:text>
   </xsl:if>
   <xsl:value-of select="thumbnail"/>
  </cover>

  <description>
   <xsl:value-of select="description"/>
  </description>

  <num-players>
   <xsl:call-template name="numplayer">
    <xsl:with-param name="min" select="minplayers/@value"/>
    <xsl:with-param name="max" select="maxplayers/@value"/>
   </xsl:call-template>
  </num-players>

  <playing-time>
   <xsl:value-of select="playingtime/@value"/>
  </playing-time>

  <minimum-age>
   <xsl:value-of select="minage/@value"/>
  </minimum-age>
 </entry>
</xsl:template>

<xsl:template match="videogame|item[@type='videogame' or @type='videogameexpansion']">
 <entry>

  <title>
   <xsl:value-of select="name[@type='primary']/@value"/>
  </title>

  <year>
   <xsl:value-of select="substring(releasedate/@value,1,4)"/>
  </year>

  <bggid>
   <xsl:value-of select="@id"/>
  </bggid>

  <videogamegeek-link>
   <xsl:value-of select="concat('http://www.videogamegeek.com/videogame/', @id)"/>
  </videogamegeek-link>

  <publishers>
   <xsl:for-each select="link[@type='videogamepublisher']">
    <publisher>
     <xsl:value-of select="@value"/>
    </publisher>
   </xsl:for-each>
  </publishers>

  <developers>
   <xsl:for-each select="link[@type='videogamedeveloper']">
    <developer>
     <xsl:value-of select="@value"/>
    </developer>
   </xsl:for-each>
  </developers>

  <!-- VideoGameGeek returns multiple platforms. Bail and don't use any of them,
       allowing the user to set the platform as desired.
  <platform>
   <xsl:choose>
    <xsl:when test="link[@type='videogameplatform']/@value = 'PlayStation 4'">PlayStation4</xsl:when>
    <xsl:when test="link[@type='videogameplatform']/@value = 'PlayStation 3'">PlayStation3</xsl:when>
    <xsl:when test="link[@type='videogameplatform']/@value = 'PlayStation 2'">PlayStation2</xsl:when>
    <xsl:otherwise><xsl:value-of select="link[@type='videogameplatform']/@value"/></xsl:otherwise>
   </xsl:choose>
  </platform>
  -->

  <genres>
   <xsl:for-each select="link[@type='videogamegenre']">
    <genre>
     <xsl:value-of select="@value"/>
    </genre>
   </xsl:for-each>
  </genres>
  <!-- ignore videogametheme right now, could be keyword? -->

  <cover>
   <xsl:if test="starts-with(thumbnail, '//')">
    <xsl:text>https:</xsl:text>
   </xsl:if>
   <xsl:value-of select="thumbnail"/>
  </cover>

  <description>
   <xsl:value-of select="description"/>
  </description>

 </entry>
</xsl:template>

<xsl:template name="numplayer">
 <xsl:param name="min"/>
 <xsl:param name="max"/>
 <xsl:if test="$min &lt;= $max">
  <num-player>
   <xsl:value-of select="$min"/>
  </num-player>
 </xsl:if>
 <xsl:if test="$min &lt; $max">
  <xsl:call-template name="numplayer">
   <xsl:with-param name="min" select="$min + 1"/>
   <xsl:with-param name="max" select="$max"/>
  </xsl:call-template>
 </xsl:if>
</xsl:template>

</xsl:stylesheet>
